//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/10/20 16:50
//

#if PLATFORM_WINDOWS

#include "D3D11DynamicRHI.h"
#include "Config.h"
#include "Logger/CommandLogger.h"
#include "dxgi1_3.h"
#include "dxgi1_4.h"
#include "dxgi1_6.h"
#include <d3d11_4.h>
#include <evntprov.h>

#pragma comment(lib, "Rpcrt4.lib")
#define Log(...)

namespace OpenZI::CloudRender {
    /**
     * Attempts to create a D3D11 device for the adapter using at most MaxFeatureLevel.
     * If creation is successful, true is returned and the supported feature level is set in
     * OutFeatureLevel.
     */
    static bool SafeTestD3D11CreateDevice(IDXGIAdapter* Adapter, D3D_FEATURE_LEVEL MinFeatureLevel,
                                          D3D_FEATURE_LEVEL MaxFeatureLevel,
                                          D3D_FEATURE_LEVEL* OutFeatureLevel) {
        ID3D11Device* D3DDevice = nullptr;
        ID3D11DeviceContext* D3DDeviceContext = nullptr;
        uint32 DeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

        // @MIXEDREALITY_CHANGE : BEGIN - Add BGRA flag for Windows Mixed Reality HMD's
        DeviceFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        // @MIXEDREALITY_CHANGE : END

        D3D_FEATURE_LEVEL RequestedFeatureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        // Trim to allowed feature levels
        int32 FirstAllowedFeatureLevel = 0;
        int32 NumAllowedFeatureLevels = 2;
        int32 LastAllowedFeatureLevel = NumAllowedFeatureLevels - 1;

        while (FirstAllowedFeatureLevel < NumAllowedFeatureLevels) {
            if (RequestedFeatureLevels[FirstAllowedFeatureLevel] == MaxFeatureLevel) {
                break;
            }
            FirstAllowedFeatureLevel++;
        }

        while (LastAllowedFeatureLevel > 0) {
            if (RequestedFeatureLevels[LastAllowedFeatureLevel] >= MinFeatureLevel) {
                break;
            }
            LastAllowedFeatureLevel--;
        }

        NumAllowedFeatureLevels = LastAllowedFeatureLevel - FirstAllowedFeatureLevel + 1;
        if (MaxFeatureLevel < MinFeatureLevel || NumAllowedFeatureLevels <= 0) {
            return false;
        }

        // We don't want software renderer. Ideally we specify D3D_DRIVER_TYPE_HARDWARE on
        // creation but when we specify an adapter we need to specify D3D_DRIVER_TYPE_UNKNOWN
        // (otherwise the call fails). We cannot check the device type later (seems this is
        // missing functionality in D3D).
        HRESULT Result = D3D11CreateDevice(Adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, DeviceFlags,
                                           &RequestedFeatureLevels[FirstAllowedFeatureLevel],
                                           NumAllowedFeatureLevels, D3D11_SDK_VERSION, &D3DDevice,
                                           OutFeatureLevel, &D3DDeviceContext);

        if (SUCCEEDED(Result)) {
            D3DDevice->Release();
            D3DDeviceContext->Release();
            return true;
        }

        return false;
    }

    FD3D11DynamicRHI::FD3D11DynamicRHI() : Name("D3D11") { InitD3DDevice(); }
    FD3D11DynamicRHI::~FD3D11DynamicRHI() {}

    bool FD3D11DynamicRHI::InitD3DDevice() {
        // Try to create the DXGIFactory1.  This will fail if we're not running Vista SP2 or higher.
        TRefCountPtr<IDXGIFactory1> DXGIFactory1;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(DXGIFactory1.GetInitReference())))) {
            Log("Failed to create DXGIFactory1!");
            return false;
        } else if (FAILED(DXGIFactory1->QueryInterface(
                       IID_PPV_ARGS(DXGIFactory.GetInitReference())))) {
            Log("Failed to get DXGIFactory interface!");
            return false;
        }
        if (!DXGIFactory.IsValid())
            return false;

        TRefCountPtr<IDXGIFactory6> DXGIFactory6;
        DXGIFactory1->QueryInterface(__uuidof(IDXGIFactory6),
                                     (void**)DXGIFactory6.GetInitReference());

        // TODO: Shipping包要将该值改为false
        bool bAllowPerfHUD = true;

        // Allow HMD to override which graphics adapter is chosen, so we pick the adapter where the
        // HMD is connected
        uint64 HmdGraphicsAdapterLuid = 0;
        const bool bFavorNonIntegrated = FAppConfig::Get().GpuIndex == -1;

        TRefCountPtr<IDXGIAdapter> TempAdapter;
        D3D_FEATURE_LEVEL MinAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
        D3D_FEATURE_LEVEL MaxAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        TRefCountPtr<IDXGIAdapter> ChosenAdapter;
        TRefCountPtr<IDXGIAdapter> FirstAdapter;
        TRefCountPtr<IDXGIAdapter> FirstWithoutIntegratedAdapter;

        bool bIsAnyAMD = false;
        bool bIsAnyIntel = false;
        bool bIsAnyNVIDIA = false;

        int PreferredVendor = -1;
        bool bAllowSoftwareFallback = false;

        int GpuPreferenceInt = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
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

        auto LocalEnumAdapters = [&DXGIFactory6, &DXGIFactory1, GpuPreference](
                                     UINT AdapterIndex, IDXGIAdapter** Adapter) -> HRESULT {
            if (!DXGIFactory6 || GpuPreference == DXGI_GPU_PREFERENCE_UNSPECIFIED) {
                return DXGIFactory1->EnumAdapters(AdapterIndex, Adapter);
            } else {
                return DXGIFactory6->EnumAdapterByGpuPreference(
                    AdapterIndex, GpuPreference, __uuidof(IDXGIAdapter), (void**)Adapter);
            }
        };

        for (uint32 AdapterIndex = 0;
             LocalEnumAdapters(AdapterIndex, TempAdapter.GetInitReference()) !=
             DXGI_ERROR_NOT_FOUND;
             ++AdapterIndex) {
            if (TempAdapter) {
                D3D_FEATURE_LEVEL ActualFeatureLevel = (D3D_FEATURE_LEVEL)0;
                if (SafeTestD3D11CreateDevice(TempAdapter, MinAllowedFeatureLevel,
                                              MaxAllowedFeatureLevel, &ActualFeatureLevel)) {
                    DXGI_ADAPTER_DESC AdapterDesc;
                    TempAdapter->GetDesc(&AdapterDesc);
                    bool bIsAMD = AdapterDesc.VendorId == 0x1002;
                    bool bIsIntel = AdapterDesc.VendorId == 0x8086;
                    bool bIsNVIDIA = AdapterDesc.VendorId == 0x10DE;
                    bool bIsMicrosoft = AdapterDesc.VendorId == 0x1414;
                    if (bIsAMD)
                        bIsAnyAMD = true;
                    if (bIsIntel)
                        bIsAnyIntel = true;
                    if (bIsNVIDIA)
                        bIsAnyNVIDIA = true;
                    // Simple heuristic but without profiling it's hard to do better
                    bool bIsNonLocalMemoryPresent = false;
                    if (bIsIntel) {
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
                    // Add special check to support HMDs, which do not have associated outputs.
                    // To reject the software emulation, unless the cvar wants it.
                    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#WARP_new_for_Win8
                    // Before we tested for no output devices but that failed where a laptop had a
                    // Intel (with output) and NVidia (with no output)
                    const bool bSkipSoftwareAdapter = bIsMicrosoft && !bAllowSoftwareFallback &&
                                                      FAppConfig::Get().GpuIndex < 0 &&
                                                      HmdGraphicsAdapterLuid == 0;

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
                    const bool bSkipExplicitAdapter =
                        FAppConfig::Get().AdapterLuidLowPart !=
                            (uint32)AdapterDesc.AdapterLuid.LowPart ||
                        FAppConfig::Get().AdapterLuidHighPart !=
                            (uint32)AdapterDesc.AdapterLuid.HighPart ||
                        FAppConfig::Get().AdapterDeviceId != (uint32)AdapterDesc.DeviceId;

                    const bool bSkipAdapter = bSkipSoftwareAdapter || bSkipPerfHUDAdapter ||
                                              bSkipHmdGraphicsAdapter || bSkipExplicitAdapter;

                    auto CurrentAdapter = TempAdapter;
                    if (!bSkipAdapter) {
                        if (!bIsIntegrated && !FirstWithoutIntegratedAdapter.IsValid()) {
                            FirstWithoutIntegratedAdapter = CurrentAdapter;
                        } else if (PreferredVendor == AdapterDesc.VendorId &&
                                   FirstWithoutIntegratedAdapter.IsValid()) {
                            FirstWithoutIntegratedAdapter = CurrentAdapter;
                        }

                        if (!FirstAdapter.IsValid()) {
                            FirstAdapter = CurrentAdapter;
                        } else if (PreferredVendor == AdapterDesc.VendorId &&
                                   FirstAdapter.IsValid()) {
                            FirstAdapter = CurrentAdapter;
                        }
                    }
                } else {
                    ML_LOG(Main, Error, "  %2u. Unknown, failed to create test device.",
                           AdapterIndex);
                }
            } else {
                ML_LOG(Main, Error, "  %2u. Unknown, failed to create adapter.", AdapterIndex);
            }
        }

        if (bFavorNonIntegrated) {
            ChosenAdapter = FirstWithoutIntegratedAdapter;

            // We assume Intel is integrated graphics (slower than discrete) than NVIDIA or AMD
            // cards and rather take a different one
            if (!ChosenAdapter.IsValid()) {
                ChosenAdapter = FirstAdapter;
            }
        } else {
            ChosenAdapter = FirstAdapter;
        }

        if (ChosenAdapter.IsValid()) {
            DXGI_ADAPTER_DESC AdapterDesc;
            ChosenAdapter->GetDesc(&AdapterDesc);
            ML_LOG(Main, Log, "Chosen D3D11 Adapter:");
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

            bool bSuccess = CreateDevice(ChosenAdapter.GetReference(), Device.GetInitReference(),
                                         DeviceContext.GetInitReference());

            return bSuccess;
        } else {
            ML_LOG(Main, Error, "Failed to choose a D3D11 Adapter.");
            return false;
        }
    }

    FTexture2DRHIRef FD3D11DynamicRHI::GetSharedTexture(HANDLE SharedTextureHandle) {
        FTexture2DRHIRef Texture2DRHIRef;
        if (!SharedTextureHandle)
            return Texture2DRHIRef;

        TRefCountPtr<ID3D11Texture2D> pTexture;
        if (SUCCEEDED(Device->OpenSharedResource(SharedTextureHandle,
                                                 IID_PPV_ARGS(pTexture.GetInitReference())))) {
            
            DeviceContext->Flush();
        } else {
            // int iGpu = 2;
            // // Initialize DXGI
            // // Need to use DXGI 1.1 for shared texture support.
            // TRefCountPtr<IDXGIFactory1> pDXGIFactory1;
            // TRefCountPtr<IDXGIFactory> TempDXGIFactory;
            // if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(pDXGIFactory1.GetInitReference())))) {
            //     ML_LOG(Main, Error, "Failed to create DXGIFactory1!");
            //     return Texture2DRHIRef;
            // } else if (FAILED(pDXGIFactory1->QueryInterface(
            //                IID_PPV_ARGS(TempDXGIFactory.GetInitReference())))) {
            //     ML_LOG(Main, Error, "Failed to get DXGIFactory interface!");
            //     return Texture2DRHIRef;
            // }
            // if (!TempDXGIFactory.IsValid())
            //     ML_LOG(Main, Error, "TempDXGIFactory is not valid");
            //     return Texture2DRHIRef;

            // TRefCountPtr<IDXGIAdapter> DXGIAdapter;
            // if (FAILED(TempDXGIFactory->EnumAdapters(iGpu, DXGIAdapter.GetInitReference()))) {
            //     ML_LOG(Main, Error, "Failed to enum adapter GpuIndex %d device", iGpu);
            //     return Texture2DRHIRef;
            // }
            // TRefCountPtr<ID3D11Device> TempDevice;
            // TRefCountPtr<ID3D11DeviceContext> TempDeviceContext;
            // if (CreateDevice(DXGIAdapter.GetReference(), TempDevice.GetInitReference(),
            //                  TempDeviceContext.GetInitReference())) {
            //     if (SUCCEEDED(TempDevice->OpenSharedResource(
            //             SharedTextureHandle, IID_PPV_ARGS(pTexture.GetInitReference())))) {
            //         ML_LOG(Main, Error, "Open success");
            //     } else {

            //         ML_LOG(Main, Error, "Open failed");
            //     }
            // }
            return Texture2DRHIRef;
        }
        FRHITexture2D* Texture2D = new FD3D11Texture2D(pTexture.GetReference());
        Texture2DRHIRef = Texture2D;
        return std::move(Texture2DRHIRef);
        // return new FD3D11Texture2D(pTexture.GetReference());
    }

    bool FD3D11DynamicRHI::CreateDevice(IDXGIAdapter* DXGIAdapter, ID3D11Device** pD3D11Device,
                                        ID3D11DeviceContext** pD3D11Context) {
        UINT creationFlags = 0;
#if _DEBUG
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL eFeatureLevel;

        HRESULT hRes =
            D3D11CreateDevice(DXGIAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, creationFlags, NULL, 0,
                              D3D11_SDK_VERSION, pD3D11Device, &eFeatureLevel, pD3D11Context);
#if _DEBUG
        // CreateDevice fails on Win10 in debug if the Win10 SDK isn't installed.
        if (pD3D11Device == NULL) {
            hRes =
                D3D11CreateDevice(DXGIAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0,
                                  D3D11_SDK_VERSION, pD3D11Device, &eFeatureLevel, pD3D11Context);
        }
#endif
        if (FAILED(hRes)) {
            Log("Failed to create D3D11 device! (err=%u)", hRes);
            return false;
        }

        if (eFeatureLevel < D3D_FEATURE_LEVEL_11_0) {
            Log("DX11 level hardware required!");
            return false;
        }

        TRefCountPtr<ID3D11Multithread> D3D11Multithread;
        HRESULT hr =
            (*pD3D11Context)->QueryInterface(IID_PPV_ARGS(D3D11Multithread.GetInitReference()));
        if (SUCCEEDED(hr)) {
            Log(L"Successfully get ID3D11Multithread interface. We set "
                L"SetMultithreadProtected(TRUE)");
            D3D11Multithread->SetMultithreadProtected(TRUE);
            D3D11Multithread->Release();
        } else {
            Log(L"Failed to get ID3D11Multithread interface. Ignore.");
        }

        return true;
    }

    bool FD3D11DynamicRHI::RegenerateD3DDevice() {
        // Try to create the DXGIFactory1.  This will fail if we're not running Vista SP2 or higher.
        TRefCountPtr<IDXGIFactory1> DXGIFactory1;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(DXGIFactory1.GetInitReference())))) {
            Log("Failed to create DXGIFactory1!");
            return false;
        } else if (FAILED(DXGIFactory1->QueryInterface(
                       IID_PPV_ARGS(DXGIFactory.GetInitReference())))) {
            Log("Failed to get DXGIFactory interface!");
            return false;
        }
        if (!DXGIFactory.IsValid())
            return false;

        TRefCountPtr<IDXGIFactory6> DXGIFactory6;
        DXGIFactory1->QueryInterface(__uuidof(IDXGIFactory6),
                                     (void**)DXGIFactory6.GetInitReference());

        // TODO: Shipping包要将该值改为false
        bool bAllowPerfHUD = true;

        // Allow HMD to override which graphics adapter is chosen, so we pick the adapter where the
        // HMD is connected
        uint64 HmdGraphicsAdapterLuid = 0;
        const bool bFavorNonIntegrated = FAppConfig::Get().GpuIndex == -1;

        TRefCountPtr<IDXGIAdapter> TempAdapter;
        D3D_FEATURE_LEVEL MinAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
        D3D_FEATURE_LEVEL MaxAllowedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        TRefCountPtr<IDXGIAdapter> ChosenAdapter;
        TRefCountPtr<IDXGIAdapter> FirstAdapter;
        TRefCountPtr<IDXGIAdapter> FirstWithoutIntegratedAdapter;

        bool bIsAnyAMD = false;
        bool bIsAnyIntel = false;
        bool bIsAnyNVIDIA = false;

        int PreferredVendor = -1;
        bool bAllowSoftwareFallback = false;

        int GpuPreferenceInt = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
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

        auto LocalEnumAdapters = [&DXGIFactory6, &DXGIFactory1, GpuPreference](
                                     UINT AdapterIndex, IDXGIAdapter** Adapter) -> HRESULT {
            if (!DXGIFactory6 || GpuPreference == DXGI_GPU_PREFERENCE_UNSPECIFIED) {
                return DXGIFactory1->EnumAdapters(AdapterIndex, Adapter);
            } else {
                return DXGIFactory6->EnumAdapterByGpuPreference(
                    AdapterIndex, GpuPreference, __uuidof(IDXGIAdapter), (void**)Adapter);
            }
        };

        for (uint32 AdapterIndex = 0;
             LocalEnumAdapters(AdapterIndex, TempAdapter.GetInitReference()) !=
             DXGI_ERROR_NOT_FOUND;
             ++AdapterIndex) {
            if (TempAdapter) {
                D3D_FEATURE_LEVEL ActualFeatureLevel = (D3D_FEATURE_LEVEL)0;
                if (SafeTestD3D11CreateDevice(TempAdapter, MinAllowedFeatureLevel,
                                              MaxAllowedFeatureLevel, &ActualFeatureLevel)) {
                    DXGI_ADAPTER_DESC AdapterDesc;
                    TempAdapter->GetDesc(&AdapterDesc);
                    const bool bSkipSoftwareAdapter = !bAllowSoftwareFallback &&
                                                      FAppConfig::Get().GpuIndex < 0 &&
                                                      HmdGraphicsAdapterLuid == 0;

                    const bool bSkipAdapter =
                        (uint32)AdapterDesc.AdapterLuid.LowPart !=
                            FAppConfig::Get().AdapterLuidLowPart ||
                        (uint32)AdapterDesc.AdapterLuid.HighPart !=
                            FAppConfig::Get().AdapterLuidHighPart ||
                        (uint32)AdapterDesc.DeviceId != FAppConfig::Get().AdapterDeviceId;

                    if (!bSkipAdapter) {
                        ChosenAdapter = TempAdapter;
                    }
                } else {
                    ML_LOG(Main, Error, "  %2u. Unknown, failed to create test device.",
                           AdapterIndex);
                }
            } else {
                ML_LOG(Main, Error, "  %2u. Unknown, failed to create adapter.", AdapterIndex);
            }
        }

        if (bFavorNonIntegrated) {
            ChosenAdapter = FirstWithoutIntegratedAdapter;

            // We assume Intel is integrated graphics (slower than discrete) than NVIDIA or AMD
            // cards and rather take a different one
            if (!ChosenAdapter.IsValid()) {
                ChosenAdapter = FirstAdapter;
            }
        } else {
            ChosenAdapter = FirstAdapter;
        }

        if (ChosenAdapter.IsValid()) {
            DXGI_ADAPTER_DESC AdapterDesc;
            ChosenAdapter->GetDesc(&AdapterDesc);
            ML_LOG(Main, Log, "Chosen D3D11 Adapter:");
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

            bool bSuccess = CreateDevice(ChosenAdapter.GetReference(), Device.GetInitReference(),
                                         DeviceContext.GetInitReference());

            return bSuccess;
        } else {
            ML_LOG(Main, Error, "Failed to choose a D3D11 Adapter.");
            return false;
        }
    }
} // namespace OpenZI::CloudRender

#endif