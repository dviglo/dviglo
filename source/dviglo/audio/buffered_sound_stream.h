// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../audio/sound_stream.h"
#include "../container/array_ptr.h"
#include "../container/list.h"
#include "../core/mutex.h"
#include "../container/pair.h"

namespace dviglo
{

/// %Sound stream that supports manual buffering of data from the main thread.
class URHO3D_API BufferedSoundStream : public SoundStream
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
    void AddData(const SharedArrayPtr<signed char>& data, unsigned numBytes);
    /// Buffer sound data by taking ownership of it.
    void AddData(const SharedArrayPtr<signed short>& data, unsigned numBytes);
    /// Remove all buffered audio data.
    void Clear();

    /// Return amount of buffered (unplayed) sound data in bytes.
    unsigned GetBufferNumBytes() const;
    /// Return length of buffered (unplayed) sound data in seconds.
    float GetBufferLength() const;

private:
    /// Buffers and their sizes.
    List<Pair<SharedArrayPtr<signed char>, unsigned>> buffers_;
    /// Byte position in the front most buffer.
    unsigned position_;
    /// Mutex for buffer data.
    mutable Mutex bufferMutex_;
};

}
