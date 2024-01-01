// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#include "buffered_sound_stream.h"

#include <cstring>
#include <iterator>

#include "../common/debug_new.h"

using namespace std;

namespace dviglo
{

BufferedSoundStream::BufferedSoundStream() :
    position_(0)
{
}

BufferedSoundStream::~BufferedSoundStream() = default;

unsigned BufferedSoundStream::GetData(signed char* dest, unsigned numBytes)
{
    scoped_lock lock(buffer_mutex_);

    unsigned outBytes = 0;

    while (numBytes && buffers_.Size())
    {
        // Copy as much from the front buffer as possible, then discard it and move to the next
        List<Pair<shared_ptr<signed char[]>, unsigned>>::Iterator front = buffers_.Begin();

        unsigned copySize = front->second_ - position_;
        if (copySize > numBytes)
            copySize = numBytes;

        memcpy(dest, front->first_.get() + position_, copySize);
        position_ += copySize;
        if (position_ >= front->second_)
        {
            buffers_.PopFront();
            position_ = 0;
        }

        dest += copySize;
        outBytes += copySize;
        numBytes -= copySize;
    }

    return outBytes;
}

void BufferedSoundStream::AddData(void* data, unsigned numBytes)
{
    if (data && numBytes)
    {
        scoped_lock lock(buffer_mutex_);

        shared_ptr<signed char[]> newBuffer = make_shared<signed char[]>(numBytes);
        memcpy(newBuffer.get(), data, numBytes);
        buffers_.Push(MakePair(newBuffer, numBytes));
    }
}

void BufferedSoundStream::AddData(const shared_ptr<signed char[]>& data, unsigned numBytes)
{
    if (data && numBytes)
    {
        scoped_lock lock(buffer_mutex_);

        buffers_.Push(MakePair(data, numBytes));
    }
}

void BufferedSoundStream::AddData(const shared_ptr<signed short[]>& data, unsigned numBytes)
{
    if (data && numBytes)
    {
        scoped_lock lock(buffer_mutex_);

        buffers_.Push(MakePair(reinterpret_pointer_cast<signed char[]>(data), numBytes));
    }
}

void BufferedSoundStream::Clear()
{
    scoped_lock lock(buffer_mutex_);

    buffers_.Clear();
    position_ = 0;
}

unsigned BufferedSoundStream::GetBufferNumBytes() const
{
    scoped_lock lock(buffer_mutex_);

    unsigned ret = 0;
    for (List<Pair<shared_ptr<signed char[]>, unsigned>>::ConstIterator i = buffers_.Begin(); i != buffers_.End(); ++i)
        ret += i->second_;
    // Subtract amount of sound data played from the front buffer
    ret -= position_;

    return ret;
}

float BufferedSoundStream::GetBufferLength() const
{
    return (float)GetBufferNumBytes() / (GetFrequency() * (float)GetSampleSize());
}

}
