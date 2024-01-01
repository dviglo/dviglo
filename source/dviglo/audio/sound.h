// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2024 the Dviglo project
// License: MIT

#pragma once

#include "../resource/resource.h"

#include <memory>

namespace dviglo
{

class SoundStream;

/// %Sound resource.
class DV_API Sound : public ResourceWithMetadata
{
    DV_OBJECT(Sound);

public:
    /// Construct.
    explicit Sound();
    /// Destruct and free sound data.
    ~Sound() override;
    /// Register object factory.
    static void register_object();

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool begin_load(Deserializer& source) override;

    /// Load raw sound data.
    bool load_raw(Deserializer& source);
    /// Load WAV format sound data.
    bool load_wav(Deserializer& source);
    /// Load Ogg Vorbis format sound data. Does not decode at load, but will rather be decoded while playing.
    bool LoadOggVorbis(Deserializer& source);
    /// Set sound size in bytes. Also resets the sound to be uncompressed and one-shot.
    void SetSize(unsigned dataSize);
    /// Set uncompressed sound data.
    void SetData(const void* data, unsigned dataSize);
    /// Set uncompressed sound data format.
    void SetFormat(unsigned frequency, bool sixteenBit, bool stereo);
    /// Set loop on/off. If loop is enabled, sets the full sound as loop range.
    void SetLooped(bool enable);
    /// Define loop.
    void SetLoop(unsigned repeatOffset, unsigned endOffset);

    /// Return a new instance of a decoder sound stream. Used by compressed sounds.
    SharedPtr<SoundStream> GetDecoderStream() const;

    /// Return shared sound data.
    std::shared_ptr<signed char[]> GetData() const { return data_; }

    /// Return sound data start.
    signed char* GetStart() const { return data_.get(); }

    /// Return loop start.
    signed char* GetRepeat() const { return repeat_; }

    /// Return sound data end.
    signed char* GetEnd() const { return end_; }

    /// Return length in seconds.
    float GetLength() const;

    /// Return total sound data size.
    unsigned GetDataSize() const { return dataSize_; }

    /// Return sample size.
    unsigned GetSampleSize() const;

    /// Return default frequency as a float.
    float GetFrequency() const { return (float)frequency_; }

    /// Return default frequency as an integer.
    unsigned GetIntFrequency() const { return frequency_; }

    /// Return whether is looped.
    bool IsLooped() const { return looped_; }

    /// Return whether data is sixteen bit.
    bool IsSixteenBit() const { return sixteenBit_; }

    /// Return whether data is stereo.
    bool IsStereo() const { return stereo_; }

    /// Return whether is compressed.
    bool IsCompressed() const { return compressed_; }

    /// Fix interpolation by copying data from loop start to loop end (looped), or adding silence (oneshot). Called internally, does not normally need to be called, unless the sound data is modified manually on the fly.
    void FixInterpolation();

private:
    /// Load optional parameters from an XML file.
    void LoadParameters();

    /// Sound data.
    std::shared_ptr<signed char[]> data_;
    /// Loop start.
    signed char* repeat_;
    /// Sound data end.
    signed char* end_;
    /// Sound data size in bytes.
    unsigned dataSize_;
    /// Default frequency.
    unsigned frequency_;
    /// Looped flag.
    bool looped_;
    /// Sixteen bit flag.
    bool sixteenBit_;
    /// Stereo flag.
    bool stereo_;
    /// Compressed flag.
    bool compressed_;
    /// Compressed sound length.
    float compressedLength_;
};

} // namespace dviglo
