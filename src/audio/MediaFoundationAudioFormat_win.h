#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#if JUCE_WINDOWS
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>

class MediaFoundationAudioReader final : public juce::AudioFormatReader
{
public:
    MediaFoundationAudioReader(juce::InputStream* in, const juce::File& file)
        : juce::AudioFormatReader(in, "Media Foundation")
    {
        MFStartup(MF_VERSION);

        HRESULT hr = MFCreateSourceReaderFromURL(file.getFullPathName().toWideCharPointer(),
                                                nullptr,
                                                reader.resetAndGetPointerAddress());
        if (FAILED(hr) || reader == nullptr)
            return;

        // Configure reader for uncompressed 16-bit PCM output
        juce::ComSmartPtr<IMFMediaType> partialType;
        MFCreateMediaType(partialType.resetAndGetPointerAddress());
        partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        partialType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);

        hr = reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, partialType.get());
        if (FAILED(hr))
            return;

        juce::ComSmartPtr<IMFMediaType> uncompressedType;
        hr = reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, uncompressedType.resetAndGetPointerAddress());
        if (FAILED(hr) || uncompressedType == nullptr)
            return;

        UINT32 sRate = 44100;
        UINT32 channels = 2;
        UINT32 bits = 16;
        uncompressedType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sRate);
        uncompressedType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
        uncompressedType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits);

        sampleRate = (double)sRate;
        numChannels = (unsigned int)channels;
        bitsPerSample = (unsigned int)bits;

        // Extract total duration in 100-nanosecond units
        PROPVARIANT var;
        PropVariantInit(&var);
        hr = reader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
        if (SUCCEEDED(hr))
        {
            lengthInSamples = (juce::int64)((var.hVal.QuadPart * (LONGLONG)sampleRate) / 10000000LL);
            PropVariantClear(&var);
        }

        currentPosition = 0;
    }

    ~MediaFoundationAudioReader() override
    {
        reader = nullptr;
    }

    bool readSamples(int* const* destSamples, int numDestChannels, int startOffsetInDestBuffer,
                     juce::int64 startSampleInFile, int numSamples) override
    {
        if (reader == nullptr || sampleRate <= 0.0 || numChannels == 0)
            return false;

        clearSamplesBeyondAvailableLength(destSamples, numDestChannels, startOffsetInDestBuffer,
                                          startSampleInFile, numSamples, lengthInSamples);

        // Seek if requested position is different from current position
        if (startSampleInFile != currentPosition)
        {
            PROPVARIANT var;
            PropVariantInit(&var);
            var.vt = VT_I8;
            var.hVal.QuadPart = (LONGLONG)((startSampleInFile * 10000000LL) / (juce::int64)sampleRate);
            reader->SetCurrentPosition(GUID_NULL, var);
            PropVariantClear(&var);
            currentPosition = startSampleInFile;
            residualBuffer.clear();
        }

        int samplesRead = 0;

        // Drain residual buffer first
        while (samplesRead < numSamples && residualBuffer.size() >= (int)numChannels)
        {
            for (int ch = 0; ch < numDestChannels; ++ch)
            {
                if (destSamples[ch] != nullptr)
                {
                    int srcCh = juce::jmin(ch, (int)numChannels - 1);
                    destSamples[ch][startOffsetInDestBuffer + samplesRead] = (int)residualBuffer[(size_t)srcCh] << 16;
                }
            }
            residualBuffer.erase(residualBuffer.begin(), residualBuffer.begin() + (int)numChannels);
            samplesRead++;
            currentPosition++;
        }

        // Read samples from Media Foundation
        while (samplesRead < numSamples)
        {
            DWORD streamFlags = 0;
            LONGLONG timestamp = 0;
            juce::ComSmartPtr<IMFSample> mfSample;

            HRESULT hr = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                            0, nullptr, &streamFlags, &timestamp,
                                            mfSample.resetAndGetPointerAddress());

            if (FAILED(hr) || (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM))
                break;

            if (mfSample == nullptr)
                continue;

            juce::ComSmartPtr<IMFMediaBuffer> mediaBuffer;
            hr = mfSample->ConvertToContiguousBuffer(mediaBuffer.resetAndGetPointerAddress());
            if (FAILED(hr) || mediaBuffer == nullptr)
                continue;

            BYTE* audioData = nullptr;
            DWORD maxLength = 0;
            DWORD currentLength = 0;
            hr = mediaBuffer->Lock(&audioData, &maxLength, &currentLength);
            if (FAILED(hr) || audioData == nullptr || currentLength == 0)
                continue;

            const int16_t* pcm16 = reinterpret_cast<const int16_t*>(audioData);
            int availableSamples = (int)(currentLength / (sizeof(int16_t) * numChannels));

            for (int s = 0; s < availableSamples; ++s)
            {
                if (samplesRead < numSamples)
                {
                    for (int ch = 0; ch < numDestChannels; ++ch)
                    {
                        if (destSamples[ch] != nullptr)
                        {
                            int srcCh = juce::jmin(ch, (int)numChannels - 1);
                            destSamples[ch][startOffsetInDestBuffer + samplesRead] = (int)pcm16[s * numChannels + srcCh] << 16;
                        }
                    }
                    samplesRead++;
                    currentPosition++;
                }
                else
                {
                    // Stash overflow into residual buffer
                    for (int ch = 0; ch < (int)numChannels; ++ch)
                        residualBuffer.push_back(pcm16[s * numChannels + ch]);
                }
            }

            mediaBuffer->Unlock();
        }

        return samplesRead > 0;
    }

private:
    juce::ComSmartPtr<IMFSourceReader> reader;
    juce::int64 currentPosition { 0 };
    std::vector<int16_t> residualBuffer;
};

class MediaFoundationAudioFormat final : public juce::AudioFormat
{
public:
    MediaFoundationAudioFormat()
        : juce::AudioFormat("Media Foundation Audio", juce::StringArray{ ".m4a", ".mp4", ".aac" })
    {
    }

    juce::Array<int> getPossibleSampleRates() override { return { 44100, 48000, 88200, 96000 }; }
    juce::Array<int> getPossibleBitDepths() override    { return { 16 }; }
    bool canDoStereo() override                        { return true; }
    bool canDoMono() override                          { return true; }

    juce::AudioFormatReader* createReaderFor(juce::InputStream* sourceStream, bool deleteStreamIfOpeningFails) override
    {
        if (auto* fis = dynamic_cast<juce::FileInputStream*>(sourceStream))
        {
            auto f = fis->getFile();
            auto* reader = new MediaFoundationAudioReader(sourceStream, f);
            if (reader->sampleRate > 0.0)
                return reader;

            delete reader;
        }

        if (deleteStreamIfOpeningFails)
            delete sourceStream;

        return nullptr;
    }

    juce::AudioFormatWriter* createWriterFor(juce::OutputStream*, double, unsigned int, int, const juce::StringPairArray&, int) override
    {
        return nullptr;
    }
};

#endif // JUCE_WINDOWS
