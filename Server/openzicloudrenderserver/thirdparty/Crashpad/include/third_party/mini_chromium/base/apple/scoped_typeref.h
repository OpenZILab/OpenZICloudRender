// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_BASE_APPLE_SCOPED_TYPEREF_H_
#define MINI_CHROMIUM_BASE_APPLE_SCOPED_TYPEREF_H_

#include "base/logging.h"
#include "base/memory/scoped_policy.h"

namespace base {

template <typename T>
struct ScopedTypeRefTraits;

template <typename T, typename Traits = ScopedTypeRefTraits<T>>
class ScopedTypeRef {
 public:
  typedef T element_type;

  ScopedTypeRef(
      T object = Traits::InvalidValue(),
      base::scoped_policy::OwnershipPolicy policy = base::scoped_policy::ASSUME)
      : object_(object) {
    if (object_ && policy == base::scoped_policy::RETAIN)
      object_ = Traits::Retain(object_);
  }

  ScopedTypeRef(const ScopedTypeRef<T, Traits>& that) : object_(that.object_) {
    if (object_)
      object_ = Traits::Retain(object_);
  }

  ~ScopedTypeRef() {
    if (object_)
      Traits::Release(object_);
  }

  ScopedTypeRef& operator=(const ScopedTypeRef<T, Traits>& that) {
    reset(that.get(), base::scoped_policy::RETAIN);
    return *this;
  }

  [[nodiscard]] T* InitializeInto() {
    DCHECK(!object_);
    return &object_;
  }

  void reset(T object = Traits::InvalidValue(),
             base::scoped_policy::OwnershipPolicy policy =
                 base::scoped_policy::ASSUME) {
    if (object && policy == base::scoped_policy::RETAIN)
      object = Traits::Retain(object);
    if (object_)
      Traits::Release(object_);
    object_ = object;
  }

  bool operator==(T that) const { return object_ == that; }

  bool operator!=(T that) const { return object_ != that; }

  operator T() const { return object_; }

  T get() const { return object_; }

  void swap(ScopedTypeRef& that) {
    T temp = that.object_;
    that.object_ = object_;
    object_ = temp;
  }

  [[nodiscard]] T release() {
    T temp = object_;
    object_ = Traits::InvalidValue();
    return temp;
  }

 private:
  T object_;
};

}  // namespace base

#endif  // MINI_CHROMIUM_BASE_APPLE_SCOPED_TYPEREF_H_
