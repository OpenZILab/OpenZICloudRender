/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/

#include "IPCTypes.h"

#if PLATFORM_WINDOWS

namespace OpenZI::CloudRender {
    FIPCMutex::FIPCMutex(const char* MutexName, bool bInitialOwner) {
        Mutex = CreateMutexA(NULL, bInitialOwner, MutexName);
        bExist = GetLastError() == ERROR_ALREADY_EXISTS;
    }

    FIPCMutex::~FIPCMutex() {
        if (Mutex)
            CloseHandle(Mutex);
    }

    bool FIPCMutex::Wait(uint32 TimeoutMs) {
        return WaitForSingleObject(Mutex, TimeoutMs) == WAIT_OBJECT_0;
    }

    void FIPCMutex::Release() {
        if (Mutex)
            ReleaseMutex(Mutex);
    }

    bool FIPCMutex::AlreadyExist() { return bExist; }

    FShareMemoryData* FShareMemoryCallback::Alloc(uint32 Size) {
        auto ShareMemoryData = new FShareMemoryData[Size + 1];
        ShareMemoryData[Size] = '$';
        return ShareMemoryData;
    }

    void FShareMemoryCallback::Free(FShareMemoryData* Data) { delete[] Data; }

    FShareMemory::FShareMemory(LPCSTR InShareName, bool bInWrite)
        : MutexName("ShareMemoryMutex-"), MemoryName("ShareMemoryMap-"), bWrite(false),
          Mutex(nullptr), MapFile(nullptr), Buffer(nullptr) {
        assert(InShareName);
        if (InShareName) {
            MemoryName.append(InShareName);
            MutexName.append(InShareName);
        }
        const char* TempName = MutexName.c_str();
        Mutex = new FIPCMutex(TempName);
    }

    FShareMemory::~FShareMemory() {
        if (Buffer) {
            ::UnmapViewOfFile(Buffer);
            Buffer = nullptr;
        }
        if (MapFile) {
            ::CloseHandle(MapFile);
            MapFile = nullptr;
        }
        if (Mutex) {
            delete Mutex;
            Mutex = nullptr;
        }
    }

    int FShareMemory::Read(FShareMemoryData*& Data, IShareMemoryCallback* Callback) {
        if (!Buffer || !Callback) {
            return -1;
        }
        Mutex->Wait();
        int Flag = ReadInternal(Data, Callback);
        Mutex->Release();
        return Flag;
    }

    int FShareMemory::ReadInternal(FShareMemoryData*& Data, IShareMemoryCallback* Callback) {
        // assert(Data == nullptr);
        if (!Buffer || !Callback) {
            return -1;
        }
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return -1;
        }
        uint32 ContentSize = Header->ContentSize;
        if (Data == nullptr)
            Data = Callback->Alloc(ContentSize + 1);
        memcpy(Data, GetContent(), ContentSize);
        Data[ContentSize] = 0;
        auto CrcCheck = Crc64(Data, ContentSize, 0);
        if (CrcCheck != Header->CrcCheck) {
            return -1;
        }
        return ContentSize;
    }

    uint64 FShareMemory::Crc64(const uchar* Data, size_t Size, uint64 Crcx) {
        static uint64 Table[256];
        static bool bInitialized = false;
        if (!bInitialized) {
            for (int i = 0; i < 256; ++i) {
                uint64 c = i;
                for (int j = 0; j < 8; ++j) {
                    c = ((c & 1) ? MLCES_CV_BIG_UINT(0xc96c5795d7870f42) : 0) ^ (c >> 1);
                }
                Table[i] = c;
            }
            bInitialized = true;
        }

        uint64 Crc = ~Crcx;
        for (size_t idx = 0; idx < Size; ++idx) {
            Crc = Table[(uchar)Crc ^ Data[idx]] ^ (Crc >> 8);
        }
        return ~Crc;
    }

    bool FShareMemory::Check() { return Mutex && MapFile && Buffer; }

    uint32 FShareMemory::GetHeadSize() const { return sizeof(FShareMemoryHeader); }

    uint32 FShareMemory::GetTailSize() const { return sizeof(FShareMemoryTail); }

    uint32 FShareMemory::GetMemorySize() const {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return 0;
        }
        return Header->MemorySize;
    }

    uint32 FShareMemory::GetContentSize() const {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return 0;
        }
        return Header->ContentSize;
    }

    HANDLE FShareMemory::GetSharedHandle() const {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return 0;
        }
        return Header->SharedHandle;
    }

    FShareMemoryHeader* FShareMemory::GetHeader() const {
        if (!Buffer) {
            return nullptr;
        }
        FShareMemoryHeader* Header = (FShareMemoryHeader*)Buffer;
        if (Header->HeadSize != GetHeadSize() || Header->TailSize != GetTailSize()) {
            return nullptr;
        }
        return Header;
    }

    FShareMemoryData* FShareMemory::GetContent() const {
        if (!Buffer) {
            return nullptr;
        }
        return Buffer + GetHeadSize();
    }

    FShareMemoryTail* FShareMemory::GetTail() const {
        if (!Buffer) {
            return nullptr;
        }
        return Buffer + GetHeadSize() + GetContentSize();
    }

    void FShareMemory::GetSizeXY(uint32& SizeX, uint32& SizeY) {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return;
        }
        SizeX = Header->Width;
        SizeY = Header->Height;
    }

    FShareMemoryReader::FShareMemoryReader(LPCSTR InShareName) : FShareMemory(InShareName, false) {
        MapFile = ::OpenFileMappingA(FILE_MAP_READ, 0, MemoryName.c_str());
        Buffer = (FShareMemoryData*)::MapViewOfFile(MapFile, FILE_MAP_READ, 0, 0, 0);
    }

    FShareMemoryReader::~FShareMemoryReader() {}

    FShareMemoryWriter::FShareMemoryWriter(LPCSTR InShareName, uint32 Size)
        : FShareMemory(InShareName, true) {
        FShareMemoryHeader Header;
        Header.HeadSize = GetHeadSize();
        Header.ContentSize = 0;
        Header.TailSize = GetTailSize();
        Header.MemorySize = Size + GetHeadSize() + GetTailSize();

        MapFile = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                       Header.MemorySize, MemoryName.c_str());

        if (!MapFile) {
            MapFile = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, MemoryName.c_str());
        }

        Buffer = (FShareMemoryData*)::MapViewOfFile(MapFile, FILE_MAP_ALL_ACCESS, 0, 0,
                                                    Header.MemorySize);

        assert(Buffer);
        if (Buffer) {
            memcpy(Buffer, &Header, sizeof(Header));
            FShareMemoryTail Tail;
            Tail = 0;
            memcpy(GetContent(), &Tail, sizeof(Tail));
        }
    }

    FShareMemoryWriter::~FShareMemoryWriter() { int a = 0; }

    int FShareMemoryWriter::Write(FShareMemoryData* Data, uint32 Size, uint32 Width, uint32 Height,
                                  HANDLE SharedHandle) {
        if (!Buffer || !Data) {
            return -1;
        }
        Mutex->Wait();
        int Flag = WriteInternal(Data, Size, Width, Height, SharedHandle);
        Mutex->Release();
        return Flag;
    }

    int FShareMemoryWriter::WriteInternal(FShareMemoryData* Data, uint32 Size, uint32 Width,
                                          uint32 Height, HANDLE SharedHandle) {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return -1;
        }
        if (Size > Header->GetMaxContentSize()) {
            Size = Header->GetMaxContentSize();
        }

        assert(Header == (FShareMemoryHeader*)Buffer);
        Header->ContentSize = Size;
        Header->Width = Width;
        Header->Height = Height;
        Header->SharedHandle = SharedHandle;
        Header->CrcCheck = Crc64(Data, Size, 0);
        memcpy(Buffer, Header, GetHeadSize());
        memcpy(GetContent(), Data, Size);
        FShareMemoryTail Tail;
        Tail = 0;
        memcpy(GetContent() + Size, &Tail, sizeof(Tail));
        return Size;
    }

} // namespace OpenZI::CloudRender

#elif PLATFORM_LINUX

#include <cassert>

namespace OpenZI::CloudRender {
    FShareMemoryData* FShareMemoryCallback::Alloc(uint32 Size) {
        auto ShareMemoryData = new FShareMemoryData[Size + 1];
        ShareMemoryData[Size] = '$';
        return ShareMemoryData;
    }

    void FShareMemoryCallback::Free(FShareMemoryData* Data) { delete[] Data; }

    FShareMemory::FShareMemory(key_t ShmKey, uint32 InShmSize)
        : ShmSize(InShmSize), Buffer(nullptr) {
        FShareMemoryHeader Header;
        Header.HeadSize = GetHeadSize();
        Header.ContentSize = 0;
        Header.TailSize = GetTailSize();
        Header.MemorySize = ShmSize - GetHeadSize() - GetTailSize();

        ShmId = shmget(ShmKey, ShmSize, IPC_CREAT | 0666);
        if (ShmId == -1) {
            // exit(1);
        }
        Buffer = (unsigned char*)shmat(ShmId, nullptr, 0);
        if (Buffer == (unsigned char*)-1) {
            // exit(1);
        }
        if (Buffer) {
            memcpy(Buffer, &Header, sizeof(Header));
            FShareMemoryTail Tail;
            Tail = 0;
            memcpy(GetContent(), &Tail, sizeof(Tail));
        }
        SemId = semget(ShmKey, 1, IPC_CREAT | 0666);
        if (SemId == -1) {
            // exit(1);
        }
        SemOperation.sem_num = 0;
        SemOperation.sem_op = 1;
        SemOperation.sem_flg = 0;
        semop(SemId, &SemOperation, 1);

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mutex, &attr);
    }

    FShareMemory::~FShareMemory() {
        // 分离共享内存
        shmdt(Buffer);

        // 删除共享内存和信号量
        shmctl(ShmId, IPC_RMID, nullptr);
        semctl(SemId, 0, IPC_RMID);
        pthread_mutexattr_destroy(&attr);
    }

    int FShareMemory::Read(FShareMemoryData*& Data, IShareMemoryCallback* Callback) {
        if (!Buffer || !Callback) {
            return -1;
        }
        SemOperation.sem_op = -1;
        // semop(SemId, &SemOperation, 1);
        pthread_mutex_lock(&mutex);
        int Flag = ReadInternal(Data, Callback);
        pthread_mutex_unlock(&mutex);
        // SemOperation.sem_op = 1;
        // semop(SemId, &SemOperation, 1);
        return Flag;
    }

    int FShareMemory::ReadInternal(FShareMemoryData*& Data, IShareMemoryCallback* Callback) {
        // assert(Data == nullptr);
        if (!Buffer || !Callback) {
            return -1;
        }
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return -1;
        }
        uint32 ContentSize = Header->ContentSize;
        if (Data == nullptr)
            Data = Callback->Alloc(ContentSize + 1);
        memcpy(Data, GetContent(), ContentSize);
        Data[ContentSize] = 0;
        auto CrcCheck = Crc64(Data, ContentSize, 0);
        if (CrcCheck != Header->CrcCheck) {
            return -1;
        }
        return ContentSize;
    }

    uint64 FShareMemory::Crc64(const uchar* Data, size_t Size, uint64 Crcx) {
        static uint64 Table[256];
        static bool bInitialized = false;
        if (!bInitialized) {
            for (int i = 0; i < 256; ++i) {
                uint64 c = i;
                for (int j = 0; j < 8; ++j) {
                    c = ((c & 1) ? MLCES_CV_BIG_UINT(0xc96c5795d7870f42) : 0) ^ (c >> 1);
                }
                Table[i] = c;
            }
            bInitialized = true;
        }

        uint64 Crc = ~Crcx;
        for (size_t idx = 0; idx < Size; ++idx) {
            Crc = Table[(uchar)Crc ^ Data[idx]] ^ (Crc >> 8);
        }
        return ~Crc;
    }

    bool FShareMemory::Check() { return ShmId != -1 && SemId != -1 && Buffer; }

    uint32 FShareMemory::GetHeadSize() const { return sizeof(FShareMemoryHeader); }

    uint32 FShareMemory::GetTailSize() const { return sizeof(FShareMemoryTail); }

    uint32 FShareMemory::GetMemorySize() const {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return 0;
        }
        return Header->MemorySize;
    }

    uint32 FShareMemory::GetContentSize() const {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return 0;
        }
        return Header->ContentSize;
    }

    int FShareMemory::GetSharedHandle() const {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return 0;
        }
        return Header->SharedHandle;
    }

    FShareMemoryHeader* FShareMemory::GetHeader() const {
        if (!Buffer) {
            return nullptr;
        }
        FShareMemoryHeader* Header = (FShareMemoryHeader*)Buffer;
        if (Header->HeadSize != GetHeadSize() || Header->TailSize != GetTailSize()) {
            return nullptr;
        }
        return Header;
    }

    FShareMemoryData* FShareMemory::GetContent() const {
        if (!Buffer) {
            return nullptr;
        }
        return Buffer + GetHeadSize();
    }

    FShareMemoryTail* FShareMemory::GetTail() const {
        if (!Buffer) {
            return nullptr;
        }
        return Buffer + GetHeadSize() + GetContentSize();
    }

    void FShareMemory::GetSizeXY(uint32& SizeX, uint32& SizeY) {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return;
        }
        SizeX = Header->Width;
        SizeY = Header->Height;
    }

    int FShareMemory::Write(FShareMemoryData* Data, uint32 Size, uint32 Width, uint32 Height,
                            int SharedHandle) {
        if (!Buffer || !Data) {
            return -1;
        }
        // SemOperation.sem_op = -1;
        // semop(SemId, &SemOperation, 1);
        pthread_mutex_lock(&mutex);
        int Flag = WriteInternal(Data, Size, Width, Height, SharedHandle);
        pthread_mutex_unlock(&mutex);
        // SemOperation.sem_op = 1;
        // semop(SemId, &SemOperation, 1);
        return Flag;
    }

    int FShareMemory::WriteInternal(FShareMemoryData* Data, uint32 Size, uint32 Width,
                                    uint32 Height, int SharedHandle) {
        FShareMemoryHeader* Header = GetHeader();
        if (!Header) {
            return -1;
        }
        if (Size > Header->GetMaxContentSize()) {
            Size = Header->GetMaxContentSize();
        }

        assert(Header == (FShareMemoryHeader*)Buffer);
        Header->ContentSize = Size;
        Header->Width = Width;
        Header->Height = Height;
        Header->SharedHandle = SharedHandle;
        Header->CrcCheck = Crc64(Data, Size, 0);
        memcpy(Buffer, Header, GetHeadSize());
        memcpy(GetContent(), Data, Size);
        FShareMemoryTail Tail;
        Tail = 0;
        memcpy(GetContent() + Size, &Tail, sizeof(Tail));
        return Size;
    }

} // namespace OpenZI::CloudRender
#endif