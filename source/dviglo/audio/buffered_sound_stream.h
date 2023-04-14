// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "sound_stream.h"
#include "../containers/array_ptr.h"
#include "../containers/list.h"
#include "../containers/pair.h"

#include <memory>
#include <mutex>

namespace dviglo
{

/// %Sound stream that supports manual buffering of data from the main thread.
class DV_API BufferedSoundStream : public SoundStream
{
public:
    /// Construct.
    BufferedSoundStream();
    /// Destruct.
    ~BufferedSoundStream() override;

    /// Produce sound data into destination. Return number of bytes produced. Called by SoundSource from the mixing thread.
    unsigned GetData(signed char* dest, unsigned numBytes) override;

    /// Buffer sound data. Makes a copy of it.
    void AddData(void* data, unsigned numBytes);
    /// Buffer sound data by taking ownership of it.
    void AddData(const std::shared_ptr<signed char[]>& data, unsigned numBytes);
    /// Buffer sound data by taking ownership of it.
    void AddData(const std::shared_ptr<signed short[]>& data, unsigned numBytes);
    /// Remove all buffered audio data.
    void Clear();

    /// Return amount of buffered (unplayed) sound data in bytes.
    unsigned GetBufferNumBytes() const;
    /// Return length of buffered (unplayed) sound data in seconds.
    float GetBufferLength() const;

private:
    /// Buffers and their sizes.
    List<Pair<std::shared_ptr<signed char[]>, unsigned>> buffers_;
    /// Byte position in the front most buffer.
    unsigned position_;
    /// Mutex for buffer data.
    mutable std::mutex buffer_mutex_;
};

}
