#include "DeckFxChain.h"
#include <cmath>
#include <algorithm>

void FxSlot::prepare(double sampleRate, int blockSize)
{
    delayBufferL.assign(maxDelaySamples, 0.0f);
    delayBufferR.assign(maxDelaySamples, 0.0f);
    delayWritePos = 0;

    reverb.setSampleRate(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 2;

    chorus.prepare(spec);
    phaser.prepare(spec);

    crushHoldCount = 0;
    crushHeldL = 0.0f;
    crushHeldR = 0.0f;
    gateEnv = 0.0f;
}

void FxSlot::reset()
{
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    delayWritePos = 0;

    reverb.reset();
    chorus.reset();
    phaser.reset();

    crushHoldCount = 0;
    crushHeldL = 0.0f;
    crushHeldR = 0.0f;
    gateEnv = 0.0f;
}

void FxSlot::process(juce::AudioBuffer<float>& buffer,
                     juce::AudioBuffer<float>& scratch,
                     int numSamples,
                     double sampleRate,
                     double currentBpm)
{
    if (!enabled.load())
        return;

    float wet = wetDry.load();
    if (wet <= 0.001f)
        return;

    int numChannels = juce::jmin(2, buffer.getNumChannels());

    // 1. Copy dry input into pre-allocated scratch buffer
    for (int ch = 0; ch < numChannels; ++ch)
    {
        scratch.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    float param = parameter.load();
    auto fx = static_cast<FxType>(type.load());

    // 2. Process DSP into buffer
    switch (fx)
    {
        case FxType::Echo:
        {
            float delaySec = 0.25f;
            if (currentBpm > 30.0)
            {
                float beatSec = static_cast<float>(60.0 / currentBpm);
                if (param < 0.25f)
                    delaySec = beatSec * 0.25f;       // 1/4 beat
                else if (param < 0.50f)
                    delaySec = beatSec * 0.50f;       // 1/2 beat
                else if (param < 0.75f)
                    delaySec = beatSec * 0.75f;       // 3/4 beat
                else
                    delaySec = beatSec * 1.0f;        // 1/1 beat
            }
            else
            {
                delaySec = 0.125f + param * 0.625f;
            }

            int delaySamples = juce::jlimit(1, maxDelaySamples - 1, static_cast<int>(delaySec * sampleRate));
            float feedback = 0.48f;

            auto* l = buffer.getWritePointer(0);
            auto* r = numChannels > 1 ? buffer.getWritePointer(1) : l;

            for (int i = 0; i < numSamples; ++i)
            {
                float dryL = l[i];
                float dryR = r[i];

                int readPos = (delayWritePos - delaySamples + maxDelaySamples) % maxDelaySamples;
                float delayedL = delayBufferL[static_cast<size_t>(readPos)];
                float delayedR = delayBufferR[static_cast<size_t>(readPos)];

                delayBufferL[static_cast<size_t>(delayWritePos)] = dryL + delayedL * feedback;
                delayBufferR[static_cast<size_t>(delayWritePos)] = dryR + delayedR * feedback;

                delayWritePos = (delayWritePos + 1) % maxDelaySamples;

                l[i] = delayedL;
                if (numChannels > 1)
                    r[i] = delayedR;
            }
            break;
        }

        case FxType::Reverb:
        {
            juce::Reverb::Parameters p;
            p.roomSize = juce::jlimit(0.15f, 0.95f, param);
            p.damping = 0.4f;
            p.wetLevel = 1.0f;
            p.dryLevel = 0.0f;
            p.width = 1.0f;
            reverb.setParameters(p);

            float* l = buffer.getWritePointer(0);
            float* r = numChannels > 1 ? buffer.getWritePointer(1) : l;
            reverb.processStereo(l, r, numSamples);
            break;
        }

        case FxType::Flanger:
        {
            float rate = 0.2f + param * 2.8f;
            chorus.setRate(rate);
            chorus.setDepth(0.85f);
            chorus.setCentreDelay(3.5f);
            chorus.setFeedback(0.55f);
            chorus.setMix(1.0f);

            juce::dsp::AudioBlock<float> block(buffer);
            auto subBlock = block.getSubBlock(0, static_cast<size_t>(numSamples));
            juce::dsp::ProcessContextReplacing<float> context(subBlock);
            chorus.process(context);
            break;
        }

        case FxType::Phaser:
        {
            float rate = 0.2f + param * 3.5f;
            phaser.setRate(rate);
            phaser.setDepth(0.9f);
            phaser.setFeedback(0.5f);
            phaser.setMix(1.0f);

            juce::dsp::AudioBlock<float> block(buffer);
            auto subBlock = block.getSubBlock(0, static_cast<size_t>(numSamples));
            juce::dsp::ProcessContextReplacing<float> context(subBlock);
            phaser.process(context);
            break;
        }

        case FxType::BitCrusher:
        {
            float bits = juce::jmap(param, 0.0f, 1.0f, 12.0f, 3.0f);
            float steps = std::pow(2.0f, bits);
            int holdSamples = 1 + static_cast<int>(param * 7.0f);

            auto* l = buffer.getWritePointer(0);
            auto* r = numChannels > 1 ? buffer.getWritePointer(1) : l;

            for (int i = 0; i < numSamples; ++i)
            {
                crushHoldCount++;
                if (crushHoldCount >= holdSamples)
                {
                    crushHoldCount = 0;
                    crushHeldL = std::round(l[i] * steps) / steps;
                    crushHeldR = std::round(r[i] * steps) / steps;
                }
                l[i] = crushHeldL;
                if (numChannels > 1)
                    r[i] = crushHeldR;
            }
            break;
        }

        case FxType::Gate:
        {
            // Rhythmic pattern gate synced to BPM or rate
            double rateHz = (currentBpm > 30.0) ? (currentBpm / 60.0) * (param < 0.5f ? 2.0 : 4.0)
                                                : (2.0 + param * 6.0);
            double phaseStep = (rateHz * juce::MathConstants<double>::twoPi) / sampleRate;

            auto* l = buffer.getWritePointer(0);
            auto* r = numChannels > 1 ? buffer.getWritePointer(1) : l;

            for (int i = 0; i < numSamples; ++i)
            {
                gateEnv += static_cast<float>(phaseStep);
                if (gateEnv >= juce::MathConstants<float>::twoPi)
                    gateEnv -= juce::MathConstants<float>::twoPi;

                // Square pulse with soft cosine transitions to eliminate clicks
                float rawPulse = std::sin(gateEnv) > 0.0f ? 1.0f : 0.0f;
                float mod = rawPulse * 0.95f + 0.05f;

                l[i] *= mod;
                if (numChannels > 1)
                    r[i] *= mod;
            }
            break;
        }
    }

    // 3. Dry/Wet Crossfade: buffer = dry * (1 - wet) + wetSample * wet
    float dryGain = 1.0f - wet;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        const auto* inDry = scratch.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = (inDry[i] * dryGain) + (out[i] * wet);
        }
    }
}

DeckFxChain::DeckFxChain()
{
    // Default FX assignments matching DJ software standard
    slots[0].type.store(static_cast<int>(FxType::Echo));
    slots[1].type.store(static_cast<int>(FxType::Flanger));
    slots[2].type.store(static_cast<int>(FxType::Gate));
}

void DeckFxChain::prepare(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;

    scratchBuffer.setSize(2, blockSize, false, false, true);

    for (auto& slot : slots)
    {
        slot.prepare(sampleRate, blockSize);
    }
}

void DeckFxChain::reset()
{
    scratchBuffer.clear();
    for (auto& slot : slots)
    {
        slot.reset();
    }
}

void DeckFxChain::process(juce::AudioBuffer<float>& buffer, int numSamples, double currentBpm)
{
    if (scratchBuffer.getNumSamples() < numSamples)
        scratchBuffer.setSize(2, numSamples, false, false, true);

    for (auto& slot : slots)
    {
        if (slot.enabled.load())
        {
            slot.process(buffer, scratchBuffer, numSamples, currentSampleRate, currentBpm);
        }
    }
}
