//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/05 09:26
//

#pragma once

#if PLATFORM_WINDOWS

#include <Windows.h>
#include <assert.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace OpenZI::CloudRender {
   using uint32 = uint32_t;
   using int64 = int64_t;
   using uint64 = uint64_t;
   using schar = signed char;
   using uchar = unsigned char;
   using ushort = unsigned short;
#ifndef MLCES_CV_BIG_INT
#define MLCES_CV_BIG_INT(n) n##LL
#endif
#ifndef MLCES_CV_BIG_UINT
#define MLCES_CV_BIG_UINT(n) n##ULL
#endif

   using FShareMemoryData = uchar;
   using FShareMemoryTail = uchar;

   // A named mutex for synchronization between processes.
   class FIPCMutex {
  public:
      FIPCMutex(const char* MutexName, bool bInitialOwner = false);
      ~FIPCMutex();

      bool Wait(uint32 TimeoutMs = INFINITE);
      void Release();
      bool AlreadyExist();

  private:
      HANDLE Mutex;
      bool bExist;
   };

   struct FShareMemoryHeader {
      uint32 MemorySize = 0;
      uint32 HeadSize = 0;
      uint32 ContentSize = 0;
      uint32 TailSize = 0;
      uint32 Width = 0;
      uint32 Height = 0;
      HANDLE SharedHandle = nullptr;
      uint64 CrcCheck = 0;
      uint32 VarReserved = 0;

      uint32 GetMaxContentSize() { return MemorySize - HeadSize - TailSize; }
   };

   __interface IShareMemoryCallback {
  public:
      virtual FShareMemoryData* Alloc(uint32 Size) = 0;
      virtual void Free(FShareMemoryData * Data) = 0;
   };

   class FShareMemoryCallback : public IShareMemoryCallback {
  public:
      virtual FShareMemoryData* Alloc(uint32 Size);
      virtual void Free(FShareMemoryData* Data);
   };

   class FShareMemory {
  public:
      FShareMemory(LPCSTR InShareName, bool bInWrite);
      virtual ~FShareMemory();

      int Read(FShareMemoryData*& Data, IShareMemoryCallback* Callback);

      int ReadInternal(FShareMemoryData*& Data, IShareMemoryCallback* Callback);

      static uint64 Crc64(const uchar* Data, size_t Size, uint64 Crcx);

      bool Check();

      uint32 GetHeadSize() const;
      uint32 GetTailSize() const;
      uint32 GetMemorySize() const;
      uint32 GetContentSize() const;
      HANDLE GetSharedHandle() const;
      FShareMemoryHeader* GetHeader() const;
      FShareMemoryData* GetContent() const;
      FShareMemoryTail* GetTail() const;
      void GetSizeXY(uint32& SizeX, uint32& SizeY);

  protected:
      std::string MutexName;
      std::string MemoryName;
      bool bWrite;
      FIPCMutex* Mutex;
      HANDLE MapFile;
      FShareMemoryData* Buffer;
   };

   class FShareMemoryReader : public FShareMemory {
  public:
      FShareMemoryReader(LPCSTR InShareName);
      ~FShareMemoryReader();
   };

   class FShareMemoryWriter : public FShareMemory {
  public:
      FShareMemoryWriter(LPCSTR InShareName, uint32 Size);
      ~FShareMemoryWriter();

      int Write(FShareMemoryData* Data, uint32 Size, uint32 Width, uint32 Height,
                HANDLE SharedHandle);
      int WriteInternal(FShareMemoryData* Data, uint32 Size, uint32 Width, uint32 Height,
                        HANDLE SharedHandle);
   };
} // namespace OpenZI::CloudRender

#elif PLATFORM_LINUX

#include <stdint.h>
#include <cstring>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

namespace OpenZI::CloudRender {
   using uint32 = uint32_t;
   using int64 = int64_t;
   using uint64 = uint64_t;
   using schar = signed char;
   using uchar = unsigned char;
   using ushort = unsigned short;
#ifndef MLCES_CV_BIG_INT
#define MLCES_CV_BIG_INT(n) n##LL
#endif
#ifndef MLCES_CV_BIG_UINT
#define MLCES_CV_BIG_UINT(n) n##ULL
#endif

   using FShareMemoryData = uchar;
   using FShareMemoryTail = uchar;

   struct FShareMemoryHeader {
      uint32 MemorySize = 0;
      uint32 HeadSize = 0;
      uint32 ContentSize = 0;
      uint32 TailSize = 0;
      uint32 Width = 0;
      uint32 Height = 0;
      int SharedHandle = 0;
      uint64 CrcCheck = 0;
      uint32 VarReserved = 0;

      uint32 GetMaxContentSize() { return MemorySize - HeadSize - TailSize; }
   };

   class IShareMemoryCallback {
  public:
      virtual FShareMemoryData* Alloc(uint32 Size) = 0;
      virtual void Free(FShareMemoryData* Data) = 0;
   };

   class FShareMemoryCallback : public IShareMemoryCallback {
  public:
      virtual FShareMemoryData* Alloc(uint32 Size);
      virtual void Free(FShareMemoryData* Data);
   };

   class FShareMemory {
  public:
      FShareMemory(key_t ShmKey, uint32 InShmSize);
      virtual ~FShareMemory();

      int Read(FShareMemoryData*& Data, IShareMemoryCallback* Callback);
      int ReadInternal(FShareMemoryData*& Data, IShareMemoryCallback* Callback);
      int Write(FShareMemoryData* Data, uint32 Size, uint32 Width, uint32 Height, int SharedHandle);
      int WriteInternal(FShareMemoryData* Data, uint32 Size, uint32 Width, uint32 Height,
                        int SharedHandle);

      static uint64 Crc64(const uchar* Data, size_t Size, uint64 Crcx);

      bool Check();

      uint32 GetHeadSize() const;
      uint32 GetTailSize() const;
      uint32 GetMemorySize() const;
      uint32 GetContentSize() const;
      int GetSharedHandle() const;
      FShareMemoryHeader* GetHeader() const;
      FShareMemoryData* GetContent() const;
      FShareMemoryTail* GetTail() const;
      void GetSizeXY(uint32& SizeX, uint32& SizeY);

  protected:
      pthread_mutexattr_t attr;
      pthread_mutex_t mutex;
      int ShmId;
      int SemId;
      uint32 ShmSize;
      sembuf SemOperation;
      FShareMemoryData* Buffer;
   };
   using FShareMemoryReader = FShareMemory;
   using FShareMemoryWriter = FShareMemory;
} // namespace OpenZI::CloudRender
#endif