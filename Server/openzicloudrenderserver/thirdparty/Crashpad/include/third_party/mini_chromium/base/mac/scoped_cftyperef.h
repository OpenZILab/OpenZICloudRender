// Copyright 2006-2008 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_BASE_MAC_SCOPED_CFTYPEREF_H_
#define MINI_CHROMIUM_BASE_MAC_SCOPED_CFTYPEREF_H_

#include <CoreFoundation/CoreFoundation.h>

#include "base/apple/scoped_typeref.h"

namespace base {

namespace internal {

template <typename CFT>
struct ScopedCFTypeRefTraits {
  static CFT InvalidValue() { return nullptr; }
  static CFT Retain(CFT object) {
    CFRetain(object);
    return object;
  }
  static void Release(CFT object) { CFRelease(object); }
};

}  // namespace internal

template <typename CFT>
using ScopedCFTypeRef =
    ScopedTypeRef<CFT, internal::ScopedCFTypeRefTraits<CFT>>;

}  // namespace base

#endif  // MINI_CHROMIUM_BASE_MAC_SCOPED_CFTYPEREF_H_
