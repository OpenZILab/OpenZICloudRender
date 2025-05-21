//
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/31 09:21
//
#pragma once
#include "CoreMinimal.h"

namespace OpenZI
{

    /** A virtual interface for ref counted objects to implement. */
    class IRefCountedObject
    {
    public:
        virtual ~IRefCountedObject() {}
        virtual uint32 AddRef() const = 0;
        virtual uint32 Release() const = 0;
        virtual uint32 GetRefCount() const = 0;
    };

    /**
     * A smart pointer to an object which implements AddRef/Release.
     */
    template <typename ReferencedType>
    class TRefCountPtr
    {
        typedef ReferencedType *ReferenceType;

    public:
        inline TRefCountPtr() : Reference(nullptr)
        {
        }

        TRefCountPtr(ReferencedType *InReference, bool bAddRef = true)
        {
            Reference = InReference;
            if (Reference && bAddRef)
            {
                Reference->AddRef();
            }
        }

        TRefCountPtr(const TRefCountPtr &Copy)
        {
            Reference = Copy.Reference;
            if (Reference)
            {
                Reference->AddRef();
            }
        }

        template <typename CopyReferencedType>
        explicit TRefCountPtr(const TRefCountPtr<CopyReferencedType> &Copy)
        {
            Reference = static_cast<ReferencedType *>(Copy.GetReference());
            if (Reference)
            {
                Reference->AddRef();
            }
        }

        inline TRefCountPtr(TRefCountPtr &&Move)
        {
            Reference = Move.Reference;
            Move.Reference = nullptr;
        }

        template <typename MoveReferencedType>
        explicit TRefCountPtr(TRefCountPtr<MoveReferencedType> &&Move)
        {
            Reference = static_cast<ReferencedType *>(Move.GetReference());
            Move.Reference = nullptr;
        }

        ~TRefCountPtr()
        {
            if (Reference)
            {
                Reference->Release();
            }
        }

        TRefCountPtr &operator=(ReferencedType *InReference)
        {
            if (Reference != InReference)
            {
                // Call AddRef before Release, in case the new reference is the same as the old reference.
                ReferencedType *OldReference = Reference;
                Reference = InReference;
                if (Reference)
                {
                    Reference->AddRef();
                }
                if (OldReference)
                {
                    OldReference->Release();
                }
            }
            return *this;
        }

        inline TRefCountPtr &operator=(const TRefCountPtr &InPtr)
        {
            return *this = InPtr.Reference;
        }

        template <typename CopyReferencedType>
        inline TRefCountPtr &operator=(const TRefCountPtr<CopyReferencedType> &InPtr)
        {
            return *this = InPtr.GetReference();
        }

        TRefCountPtr &operator=(TRefCountPtr &&InPtr)
        {
            if (this != &InPtr)
            {
                ReferencedType *OldReference = Reference;
                Reference = InPtr.Reference;
                InPtr.Reference = nullptr;
                if (OldReference)
                {
                    OldReference->Release();
                }
            }
            return *this;
        }

        template <typename MoveReferencedType>
        TRefCountPtr &operator=(TRefCountPtr<MoveReferencedType> &&InPtr)
        {
            // InPtr is a different type (or we would have called the other operator), so we need not test &InPtr != this
            ReferencedType *OldReference = Reference;
            Reference = InPtr.Reference;
            InPtr.Reference = nullptr;
            if (OldReference)
            {
                OldReference->Release();
            }
            return *this;
        }

        inline ReferencedType *operator->() const
        {
            return Reference;
        }

        inline operator ReferenceType() const
        {
            return Reference;
        }

        inline ReferencedType **GetInitReference()
        {
            *this = nullptr;
            return &Reference;
        }

        inline ReferencedType *GetReference() const
        {
            return Reference;
        }

        inline friend bool IsValidRef(const TRefCountPtr &InReference)
        {
            return InReference.Reference != nullptr;
        }

        inline bool IsValid() const
        {
            return Reference != nullptr;
        }

        inline void SafeRelease()
        {
            *this = nullptr;
        }

        uint32 GetRefCount()
        {
            uint32 Result = 0;
            if (Reference)
            {
                Result = Reference->GetRefCount();
#if PLATFORM_WINDOWS
                // check(Result > 0); // you should never have a zero ref count if there is a live ref counted pointer (*this is live)
#elif PLATFORM_LINUX

#endif
            }
            return Result;
        }

        inline void Swap(TRefCountPtr &InPtr) // this does not change the reference count, and so is faster
        {
            ReferencedType *OldReference = Reference;
            Reference = InPtr.Reference;
            InPtr.Reference = OldReference;
        }

    private:
        ReferencedType *Reference;

        template <typename OtherType>
        friend class TRefCountPtr;
    };

    template <typename ReferencedType>
    inline bool operator==(const TRefCountPtr<ReferencedType> &A, const TRefCountPtr<ReferencedType> &B)
    {
        return A.GetReference() == B.GetReference();
    }

    template <typename ReferencedType>
    inline bool operator==(const TRefCountPtr<ReferencedType> &A, ReferencedType *B)
    {
        return A.GetReference() == B;
    }

    template <typename ReferencedType>
    inline bool operator==(ReferencedType *A, const TRefCountPtr<ReferencedType> &B)
    {
        return A == B.GetReference();
    }
} // namespace OpenZI