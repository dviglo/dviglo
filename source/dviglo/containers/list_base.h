// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../common/config.h"

#include "allocator.h"

namespace dviglo
{

/// Базовый класс узла двусвязного списка
struct ListNodeBase
{
    /// Предыдущий узел
    ListNodeBase* prev = nullptr;

    /// Следующий узел
    ListNodeBase* next = nullptr;
};

/// Doubly-linked list iterator base class.
struct ListIteratorBase
{
    /// Construct.
    ListIteratorBase() :
        ptr_(nullptr)
    {
    }

    /// Construct with a node pointer.
    explicit ListIteratorBase(ListNodeBase* ptr) :
        ptr_(ptr)
    {
    }

    /// Test for equality with another iterator.
    bool operator ==(const ListIteratorBase& rhs) const { return ptr_ == rhs.ptr_; }

    /// Test for inequality with another iterator.
    bool operator !=(const ListIteratorBase& rhs) const { return ptr_ != rhs.ptr_; }

    /// Go to the next node.
    void GotoNext()
    {
        if (ptr_)
            ptr_ = ptr_->next;
    }

    /// Go to the previous node.
    void GotoPrev()
    {
        if (ptr_)
            ptr_ = ptr_->prev;
    }

    /// Node pointer.
    ListNodeBase* ptr_;
};

/// Doubly-linked list base class.
class DV_API ListBase
{
public:
    /// Construct.
    ListBase() :
        head_(nullptr),
        tail_(nullptr),
        allocator_(nullptr),
        size_(0)
    {
    }

protected:
    /// Head node pointer.
    ListNodeBase* head_;
    /// Tail node pointer.
    ListNodeBase* tail_;
    /// Node allocator.
    AllocatorBlock* allocator_;
    /// Number of nodes.
    i32 size_;
};

}
