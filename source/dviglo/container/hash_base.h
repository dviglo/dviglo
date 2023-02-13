// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../dviglo_config.h"

#include "../container/allocator.h"
#include "../container/hash.h"
#include "../container/swap.h"

namespace dviglo
{

/// Hash set/map node base class.
struct HashNodeBase
{
    /// Construct.
    HashNodeBase() :
        down_(nullptr),
        prev_(nullptr),
        next_(nullptr)
    {
    }

    /// Next node in the bucket.
    HashNodeBase* down_;
    /// Previous node.
    HashNodeBase* prev_;
    /// Next node.
    HashNodeBase* next_;
};

/// Hash set/map iterator base class.
struct HashIteratorBase
{
    /// Construct.
    HashIteratorBase() :
        ptr_(nullptr)
    {
    }

    /// Construct with a node pointer.
    explicit HashIteratorBase(HashNodeBase* ptr) :
        ptr_(ptr)
    {
    }

    /// Test for equality with another iterator.
    bool operator ==(const HashIteratorBase& rhs) const { return ptr_ == rhs.ptr_; }

    /// Test for inequality with another iterator.
    bool operator !=(const HashIteratorBase& rhs) const { return ptr_ != rhs.ptr_; }

    /// Go to the next node.
    void GotoNext()
    {
        if (ptr_)
            ptr_ = ptr_->next_;
    }

    /// Go to the previous node.
    void GotoPrev()
    {
        if (ptr_)
            ptr_ = ptr_->prev_;
    }

    /// Node pointer.
    HashNodeBase* ptr_;
};

/// Hash set/map base class.
/** Note that to prevent extra memory use due to vtable pointer, %HashBase intentionally does not declare a virtual destructor
    and therefore %HashBase pointers should never be used.
  */
class URHO3D_API HashBase
{
public:
    /// Initial amount of buckets.
    static inline constexpr i32 MIN_BUCKETS = 8;

    /// Maximum load factor.
    static inline constexpr i32 MAX_LOAD_FACTOR = 4;

    /// Construct.
    HashBase() :
        head_(nullptr),
        tail_(nullptr),
        ptrs_(nullptr),
        allocator_(nullptr)
    {
    }

    /// Swap with another hash set or map.
    void Swap(HashBase& rhs)
    {
        dviglo::Swap(head_, rhs.head_);
        dviglo::Swap(tail_, rhs.tail_);
        dviglo::Swap(ptrs_, rhs.ptrs_);
        dviglo::Swap(allocator_, rhs.allocator_);
    }

    /// Return number of elements.
    i32 Size() const { return ptrs_ ? (reinterpret_cast<i32*>(ptrs_))[0] : 0; }

    /// Return number of buckets.
    i32 NumBuckets() const { return ptrs_ ? (reinterpret_cast<i32*>(ptrs_))[1] : 0; }

    /// Return whether has no elements.
    bool Empty() const { return Size() == 0; }

protected:
    /// Allocate bucket head pointers + room for size and bucket count variables.
    void AllocateBuckets(i32 size, i32 numBuckets);

    /// Reset bucket head pointers.
    void ResetPtrs();

    /// Set new size.
    void SetSize(i32 size);

    /// Return bucket head pointers.
    HashNodeBase** Ptrs() const { return ptrs_ ? ptrs_ + 2 : nullptr; }

    /// List head node pointer.
    HashNodeBase* head_;
    /// List tail node pointer.
    HashNodeBase* tail_;
    /// Bucket head pointers.
    HashNodeBase** ptrs_;
    /// Node allocator.
    AllocatorBlock* allocator_;
};

}
