#ifndef _SIMPLEPB_STACK_HPP_
#define _SIMPLEPB_STACK_HPP_

// This is an adaptation of RapidJson's very well implemented internal/stack.
// simplified without allocator and only for uint8_t

// Tencent is pleased to support the open source community by making RapidJSON available.
//
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved.
//
// Licensed under the MIT License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef _ATTRINLINE_
#if defined(_MSC_VER) && defined(NDEBUG)
#define _ATTRINLINE_ __forceinline
#elif defined(__GNUC__) && __GNUC__ >= 4 && defined(NDEBUG)
#define _ATTRINLINE_ __attribute__((always_inline))
#else
#define _ATTRINLINE_
#endif
#endif // _ATTRINLINE_

//! Compiler branching hint for expression with high probability to be true.
#ifndef BRANCH_LIKELY_
#if defined(__GNUC__) || defined(__clang__)
#define BRANCH_LIKELY_(x) __builtin_expect(!!(x), 1)
#else
#define BRANCH_LIKELY_(x) (x)
#endif
#endif

//! Compiler branching hint for expression with low probability to be true.
#ifndef BRANCH_UNLIKELY_
#if defined(__GNUC__) || defined(__clang__)
#define BRANCH_UNLIKELY_(x) __builtin_expect(!!(x), 0)
#else
#define BRANCH_UNLIKELY_(x) (x)
#endif
#endif

#include <stdint.h>
#include <assert.h>

namespace crow {
  class Stack {
  public:
    // Optimization note: Do not allocate memory for stack_ in constructor.
    // Do it lazily when first Push() -> Expand() -> Resize().
    Stack(size_t stackCapacity) : stack_(0), stackTop_(0), stackEnd_(0), initialCapacity_(stackCapacity) {
    }
    ~Stack() { if (stack_ != nullptr) free(stack_); }

    // Optimization note: try to minimize the size of this function for force inline.
    // Expansion is run very infrequently, so it is moved to another (probably non-inline) function.
    _ATTRINLINE_ void Reserve(size_t count = 1) {
         // Expand the stack if needed
        if (BRANCH_UNLIKELY_((stackTop_ + count) > stackEnd_))
            Expand(count);
    }

    _ATTRINLINE_ uint8_t* Push(size_t count = 1) {
        Reserve(count);
        return PushUnsafe(count);
    }

    _ATTRINLINE_ uint8_t* PushUnsafe(size_t count = 1) {
        assert((stackTop_ + count) <= stackEnd_);
        uint8_t* ret = reinterpret_cast<uint8_t*>(stackTop_);
        stackTop_ += count;
        return ret;
    }

    void Clear() { stackTop_ = stack_; }

    uint8_t* Bottom() { return reinterpret_cast<uint8_t*>(stack_); }

    const uint8_t* Bottom() const { return reinterpret_cast<uint8_t*>(stack_); }

    bool Empty() const { return stackTop_ == stack_; }
    size_t GetSize() const { return static_cast<size_t>(stackTop_ - stack_); }
    size_t GetCapacity() const { return static_cast<size_t>(stackEnd_ - stack_); }

  private:

    void Expand(size_t count) {
        // Only expand the capacity if the current stack exists. Otherwise just create a stack with initial capacity.
        size_t newCapacity;
        if (stack_ == 0) {
            newCapacity = initialCapacity_;
        } else {
            newCapacity = GetCapacity();
            newCapacity += (newCapacity + 1) / 2;
        }
        size_t newSize = GetSize() + count;
        if (newCapacity < newSize)
            newCapacity = newSize;

        Resize(newCapacity);
    }

    void Resize(size_t newCapacity) {
        const size_t size = GetSize();  // Backup the current size

        uint8_t* pOld = stack_;
        //printf("Resize %lu -> %lu\n", size, newCapacity);
        stack_ = static_cast<uint8_t*>(malloc(newCapacity));
        memcpy(stack_, pOld, size);

        stackTop_ = stack_ + size;
        stackEnd_ = stack_ + newCapacity;
    }

    uint8_t *stack_;
    uint8_t *stackTop_;
    uint8_t *stackEnd_;
    size_t initialCapacity_;
  };
}

#endif // _SIMPLEPB_STACK_HPP_
