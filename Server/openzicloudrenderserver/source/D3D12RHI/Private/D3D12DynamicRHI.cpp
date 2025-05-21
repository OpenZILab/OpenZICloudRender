//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/21 11:39
//
#if PLATFORM_WINDOWS

#include "D3D12DynamicRHI.h"
#include "Config.h"
#include "Containers/FormatString.h"
#include "Logger/CommandLogger.h"
#include "NvCodecUtils.h"
#include "NvEncoder.h"
#include <dxgi.h>
#include <dxgi1_6.h>

namespace OpenZI::CloudRender {
    static HRESULT EnumAdapters(int32 AdapterIndex, DXGI_GPU_PREFERENCE GpuPreference,
                                IDXGIFactory* DxgiFactory, IDXGIFactory6* DxgiFactory6,
                                IDXGIAdapter** TempAdapter) {
        if (!DxgiFactory6 || GpuPreference == DXGI_GPU_PREFERENCE_UNSPECIFIED) {
            return DxgiFactory->EnumAdapters(AdapterIndex, TempAdapter);
        } else {
            return DxgiFactory6->EnumAdapterByGpuPreference(AdapterIndex, GpuPreference,
                                                            IID_PPV_ARGS(TempAdapter));
        }
    }

    /**
     * Since CreateDXGIFactory is a delay loaded import from the DXGI DLL, if the user
     * doesn't have Vista/DX10, calling CreateDXGIFactory will throw an exception.
     * We use SEH to detect that case and fail gracefully.
     */
    static void SafeCreateDXGIFactory(IDXGIFactory4** DXGIFactory) {
        // bIsQuadBufferStereoEnabled =
        //     FParse::Param(FCommandLine::Get(), TEXT("quad_buffer_stereo"));
        CreateDXGIFactory(__uuidof(IDXGIFactory4), (void**)DXGIFactory);
    }

    /**
     * Returns the minimum D3D feature level required to create based on
     * command line parameters.
     */
    static D3D_FEATURE_LEVEL GetRequiredD3DFeatureLevel() { return D3D_FEATURE_LEVEL_11_0; }

    static D3D_SHADER_MODEL FindHighestShaderModel(ID3D12Device* Device) {
        // Because we can't guarantee older Windows versions will know about newer shader models, we
        // need to check them all in descending order and return the first result that succeeds.
        const D3D_SHADER_MODEL ShaderModelsToCheck[] = {
            D3D_SHADER_MODEL_6_6, D3D_SHADER_MODEL_6_5, D3D_SHADER_MODEL_6_4, D3D_SHADER_MODEL_6_3,
            D3D_SHADER_MODEL_6_2, D3D_SHADER_MODEL_6_1, D3D_SHADER_MODEL_6_0,
        };

        D3D12_FEATURE_DATA_SHADER_MODEL FeatureShaderModel{};
        for (const D3D_SHADER_MODEL ShaderModelToCheck : ShaderModelsToCheck) {
            FeatureShaderModel.HighestShaderModel = ShaderModelToCheck;

            if (SUCCEEDED(Device->CheckFeatureSupport(
                    D3D12_FEATURE_SHADER_MODEL, &FeatureShaderModel, sizeof(FeatureShaderModel)))) {
                return FeatureShaderModel.HighestShaderModel;
            }
        }

        // Last ditch effort, the minimum requirement for DX12 is 5.1
        return D3D_SHADER_MODEL_5_1;
    }

    /**
     * Attempts to create a D3D12 device for the adapter using at minimum MinFeatureLevel.
     * If creation is successful, true is returned and the max supported feature level is set in
     * OutMaxFeatureLevel.
     */
    static bool SafeTestD3D12CreateDevice(IDXGIAdapter* Adapter, D3D_FEATURE_LEVEL MinFeatureLevel,
                                          D3D_FEATURE_LEVEL& OutMaxFeatureLevel,
                                          D3D_SHADER_MODEL& OutMaxShaderModel,
                                          uint32& OutNumDeviceNodes) {
        const D3D_FEATURE_LEVEL FeatureLevels[] = {
            // Add new feature levels that the app supports here.
            D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0};

        ID3D12Device* pDevice = nullptr;
        if (SUCCEEDED(D3D12CreateDevice(Adapter, MinFeatureLevel, IID_PPV_ARGS(&pDevice)))) {
            // Determine the max feature level supported by the driver and hardware.
            D3D_FEATURE_LEVEL MaxFeatureLevel = MinFeatureLevel;
            D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevelCaps = {};
            FeatureLevelCaps.pFeatureLevelsRequested = FeatureLevels;
            FeatureLevelCaps.NumFeatureLevels = 4;
            if (SUCCEEDED(pDevice->CheckFeatureSupport(
                    D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevelCaps, sizeof(FeatureLevelCaps)))) {
                MaxFeatureLevel = FeatureLevelCaps.MaxSupportedFeatureLevel;
            }

            OutMaxShaderModel = FindHighestShaderModel(pDevice);

            OutMaxFeatureLevel = MaxFeatureLevel;
            OutNumDeviceNodes = pDevice->GetNodeCount();

            pDevice->Release();
            return true;
        }

        return false;
    }

    FD3D12DynamicRHI::FD3D12DynamicRHI() : Name("D3D12") {
        HandlePrefix = FAppConfig::Get().ShareMemorySuffix;
        // Try to create the DXGIFactory.  This will fail if we're not running Vista.
        TRefCountPtr<IDXGIFactory4> DXGIFactory4;
        SafeCreateDXGIFactory(DXGIFactory4.GetInitReference());
        if (!DXGIFactory4) {
            return;
        }

        TRefCountPtr<IDXGIFactory6> DXGIFactory6;
        DXGIFactory4->QueryInterface(__uuidof(IDXGIFactory6),
                                     (void**)DXGIFactory6.GetInitReference());

        bool bAllowPerfHUD = false;

        // Allow HMD to override which graphics adapter is chosen, so we pick the adapter where
        // the HMD is connected
        uint64 HmdGraphicsAdapterLuid = 0;
        const bool bFavorDiscreteAdapter = FAppConfig::Get().GpuIndex == -1;

        TRefCountPtr<IDXGIAdapter> TempAdapter;
        const D3D_FEATURE_LEVEL MinRequiredFeatureLevel = GetRequiredD3DFeatureLevel();

        TRefCountPtr<IDXGIAdapter> ChosenAdapter;
        TRefCountPtr<IDXGIAdapter> FirstDiscreteAdapter;
        TRefCountPtr<IDXGIAdapter> FirstAdapter;

        bool bIsAnyAMD = false;
        bool bIsAnyIntel = false;
        bool bIsAnyNVIDIA = false;
        bool bRequestedWARP = false;
        bool bAllowSoftwareRendering = false;

        int GpuPreferenceInt = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        // FParse::Value(FCommandLine::Get(), TEXT("-gpupreference="), GpuPreferenceInt);
        DXGI_GPU_PREFERENCE GpuPreference;
        switch (GpuPreferenceInt) {
        case 1:
            GpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            break;
        case 2:
            GpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            break;
        default:
            GpuPreference = DXGI_GPU_PREFERENCE_UNSPECIFIED;
            break;
        }

        int PreferredVendor = -1;

        // Enumerate the DXGIFactory's adapters.
        for (uint32 AdapterIndex = 0;
             EnumAdapters(AdapterIndex, GpuPreference, DXGIFactory4, DXGIFactory6,
                          TempAdapter.GetInitReference()) != DXGI_ERROR_NOT_FOUND;
             ++AdapterIndex) {
            // Check that if adapter supports D3D12.
            if (TempAdapter) {
                D3D_FEATURE_LEVEL MaxSupportedFeatureLevel = static_cast<D3D_FEATURE_LEVEL>(0);
                D3D_SHADER_MODEL MaxSupportedShaderModel = D3D_SHADER_MODEL_5_1;
                uint32 NumNodes = 0;

                if (SafeTestD3D12CreateDevice(TempAdapter, MinRequiredFeatureLevel,
                                              MaxSupportedFeatureLevel, MaxSupportedShaderModel,
                                              NumNodes)) {
                    ck(NumNodes > 0);
                    // Log some information about the available D3D12 adapters.
                    DXGI_ADAPTER_DESC AdapterDesc;
                    TempAdapter->GetDesc(&AdapterDesc);
                    bool bIsAMD = AdapterDesc.VendorId == 0x1002;
                    bool bIsIntel = AdapterDesc.VendorId == 0x8086;
                    bool bIsNVIDIA = AdapterDesc.VendorId == 0x10DE;
                    bool bIsWARP = AdapterDesc.VendorId == 0x1414;

                    if (bIsAMD)
                        bIsAnyAMD = true;
                    if (bIsIntel)
                        bIsAnyIntel = true;
                    if (bIsNVIDIA)
                        bIsAnyNVIDIA = true;

                    // Simple heuristic but without profiling it's hard to do better
                    bool bIsNonLocalMemoryPresent = false;
                    if (bIsIntel) {
                        ML_LOG(Main, Verbose, "D3D12DynamicRHI IsIntel");
                        TRefCountPtr<IDXGIAdapter3> TempDxgiAdapter3;
                        DXGI_QUERY_VIDEO_MEMORY_INFO NonLocalVideoMemoryInfo;
                        if (SUCCEEDED(TempAdapter->QueryInterface(
                                _uuidof(IDXGIAdapter3),
                                (void**)TempDxgiAdapter3.GetInitReference())) &&
                            TempDxgiAdapter3.IsValid() &&
                            SUCCEEDED(TempDxgiAdapter3->QueryVideoMemoryInfo(
                                0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
                                &NonLocalVideoMemoryInfo))) {
                            bIsNonLocalMemoryPresent = NonLocalVideoMemoryInfo.Budget != 0;
                        }
                    }
                    const bool bIsIntegrated = bIsIntel && !bIsNonLocalMemoryPresent;

                    // PerfHUD is for performance profiling
                    const bool bIsPerfHUD = false;

                    auto CurrentAdapter = TempAdapter;

                    // If requested WARP, then reject all other adapters. If WARP not requested,
                    // then reject the WARP device if software rendering support is disallowed
                    const bool bSkipWARP = false;

                    // we don't allow the PerfHUD adapter
                    const bool bSkipPerfHUDAdapter = bIsPerfHUD && !bAllowPerfHUD;

                    // the HMD wants a specific adapter, not this one
                    const bool bSkipHmdGraphicsAdapter =
                        HmdGraphicsAdapterLuid != 0 &&
                        memcmp(&HmdGraphicsAdapterLuid, &AdapterDesc.AdapterLuid, sizeof(LUID)) !=
                            0;

                    // the user wants a specific adapter, not this one
                    // const bool bSkipExplicitAdapter = FAppConfig::Get().GpuIndex >= 0 &&
                    //                                   AdapterIndex != FAppConfig::Get().GpuIndex;
                    const bool bSkipExplicitAdapter = FAppConfig::Get().AdapterLuidLowPart != (uint32)AdapterDesc.AdapterLuid.LowPart 
                                                      || FAppConfig::Get().AdapterLuidHighPart != (uint32)AdapterDesc.AdapterLuid.HighPart
                                                      || FAppConfig::Get().AdapterDeviceId != (uint32)AdapterDesc.DeviceId;

                    const bool bSkipAdapter = bSkipWARP || bSkipPerfHUDAdapter ||
                                              bSkipHmdGraphicsAdapter || bSkipExplicitAdapter;

                    if (!bSkipAdapter) {
                        if (!bIsWARP && !bIsIntegrated && !FirstDiscreteAdapter.IsValid()) {
                            FirstDiscreteAdapter = CurrentAdapter;
                        } else if (PreferredVendor == AdapterDesc.VendorId &&
                                   FirstDiscreteAdapter.IsValid()) {
                            FirstDiscreteAdapter = CurrentAdapter;
                        }

                        if (!FirstAdapter.IsValid()) {
                            FirstAdapter = CurrentAdapter;
                        } else if (PreferredVendor == AdapterDesc.VendorId &&
                                   FirstAdapter.IsValid()) {
                            FirstAdapter = CurrentAdapter;
                        }
                    }
                }
            }
        }

        if (bFavorDiscreteAdapter) {
            // We assume Intel is integrated graphics (slower than discrete) than NVIDIA or AMD
            // cards and rather take a different one
            if (FirstDiscreteAdapter.IsValid()) {
                ChosenAdapter = FirstDiscreteAdapter;
            } else {
                ChosenAdapter = FirstAdapter;
            }
        } else {
            ChosenAdapter = FirstAdapter;
        }

        if (ChosenAdapter.IsValid()) {
            DXGI_ADAPTER_DESC AdapterDesc;
            ChosenAdapter->GetDesc(&AdapterDesc);
            ML_LOG(Main, Log, "Chosen D3D12 Adapter:");
            ML_LOGW(Main, Log, L"    Description : %s", AdapterDesc.Description);
            ML_LOG(Main, Log, "    VendorId    : %04x", AdapterDesc.VendorId);
            ML_LOG(Main, Log, "    DeviceId    : %04x", AdapterDesc.DeviceId);
            ML_LOG(Main, Log, "    SubSysId    : %04x", AdapterDesc.SubSysId);
            ML_LOG(Main, Log, "    Revision    : %04x", AdapterDesc.Revision);
            ML_LOG(Main, Log, "    DedicatedVideoMemory : %zu bytes",
                   AdapterDesc.DedicatedVideoMemory);
            ML_LOG(Main, Log, "    DedicatedSystemMemory : %zu bytes",
                   AdapterDesc.DedicatedSystemMemory);
            ML_LOG(Main, Log, "    SharedSystemMemory : %zu bytes", AdapterDesc.SharedSystemMemory);
            ML_LOG(Main, Log, "    AdapterLuid : %lu %lu", AdapterDesc.AdapterLuid.HighPart,
                   AdapterDesc.AdapterLuid.LowPart);

            ck(D3D12CreateDevice(ChosenAdapter.GetReference(), D3D_FEATURE_LEVEL_11_0,
                                 IID_PPV_ARGS(Device.GetInitReference())));
        } else {
            ML_LOG(Main, Error, "Failed to choose a D3D12 Adapter.");
        }
    }

    FD3D12DynamicRHI::~FD3D12DynamicRHI() {}

    FTexture2DRHIRef FD3D12DynamicRHI::GetSharedTexture(HANDLE SharedTextureHandle) {
        FTexture2DRHIRef Texture2DRHIRef;
        if (!SharedTextureHandle)
            return Texture2DRHIRef;

        TRefCountPtr<ID3D12Resource> SharedTexture;
        HRESULT result = Device->OpenSharedHandle(SharedTextureHandle,
                                                  IID_PPV_ARGS(SharedTexture.GetInitReference()));
        if (SUCCEEDED(result)) {
            // ML_LOG(LogD3D12DynamicRHI, Success, "open shared handle success");
        } else {
            // ML_LOG(LogD3D12DynamicRHI, Error, "Failed to open shared handle");
            return Texture2DRHIRef;
        }
        FRHITexture2D* Texture2D = new FD3D12Texture2D(SharedTexture.GetReference(), 0, nullptr, 0);
        Texture2DRHIRef = Texture2D;
        return Texture2DRHIRef;
    }

    FTexture2DRHIRef FD3D12DynamicRHI::GetSharedTextureBySuffix(uint32 Suffix,
                                                                HANDLE SharedTextureHandle,
                                                                uint64 TextureMemorySize) {
        FTexture2DRHIRef Texture2DRHIRef;

        TRefCountPtr<ID3D12Resource> SharedTexture;
        HANDLE SH = nullptr;
        auto Name = PrintfW(L"%hs_%d", HandlePrefix.c_str(), Suffix);
        HRESULT result = Device->OpenSharedHandleByName(Name.c_str(), GENERIC_ALL, &SH);
        result = Device->OpenSharedHandle(SH, IID_PPV_ARGS(SharedTexture.GetInitReference()));
        FRHITexture2D* Texture2D =
            new FD3D12Texture2D(SharedTexture.GetReference(), TextureMemorySize, SH, Suffix);
        Texture2DRHIRef = Texture2D;
        return Texture2DRHIRef;
    }

} // namespace OpenZI::CloudRender

#endif