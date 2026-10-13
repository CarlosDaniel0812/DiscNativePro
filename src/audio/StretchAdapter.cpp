#include "StretchAdapter.h"
#include <cmath>

StretchAdapter::StretchAdapter()
{
    tmp.setSize(2, tmpSize);
    tmp.clear();
    tmpOut.setSize(2, maxBlockSamples);
    tmpOut.clear();
}

void StretchAdapter::prepare(double sr)
{
    if (sr <= 0.0)
        return;
    sampleRate = sr;
    stretch.presetCheaper(2, (float)sr);
    latencySec.store((double)(stretch.inputLatency() + stretch.outputLatency()) / sr);
    prepared = true;
}

void StretchAdapter::reset()
{
    stretch.reset();
}

void StretchAdapter::render(juce::AudioSource* src, int srcChannels,
                            float* const* dst, int dstChannels, int numSamples)
{
    if (src == nullptr || dst == nullptr || numSamples <= 0 || !prepared)
        return;

    double rate = tempoRate.load();
    if (rate < 0.02) rate = 0.02;
    if (rate > 4.0)  rate = 4.0;

    int M = juce::jlimit(1, tmpSize, (int)std::round((double)numSamples * rate));

    juce::AudioSourceChannelInfo readInfo(&tmp, 0, M);
    src->getNextAudioBlock(readInfo);

    float* inPtrs[2] = { tmp.getWritePointer(0),
                         srcChannels > 1 ? tmp.getWritePointer(1) : tmp.getWritePointer(0) };

    if (dstChannels >= 2)
    {
        stretch.process(inPtrs, M, dst, numSamples);
        return;
    }

    // Destino mono (no ocurre en el pipeline actual): procesa a temporal y mezcla.
    if (numSamples > maxBlockSamples)
        return;
    float* tmpOutPtrs[2] = { tmpOut.getWritePointer(0), tmpOut.getWritePointer(1) };
    stretch.process(inPtrs, M, tmpOutPtrs, numSamples);
    const float* a = tmpOut.getReadPointer(0);
    const float* b = tmpOut.getReadPointer(1);
    for (int i = 0; i < numSamples; ++i)
        dst[0][i] = 0.5f * (a[i] + b[i]);
}
