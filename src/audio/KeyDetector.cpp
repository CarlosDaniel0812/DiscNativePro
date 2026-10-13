#include "KeyDetector.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

namespace
{
// Plantillas Krumhansl-Kessler (12 pitch classes desde C)
constexpr float kMajorTemplate[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
                                       2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
constexpr float kMinorTemplate[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f,
                                       2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

// Camelot por pitch class (0 = C)
const char* const kCamelotMinor[12] = { "5A", "12A", "7A", "2A", "9A", "4A",
                                        "11A", "6A", "1A", "8A", "3A", "10A" };
const char* const kCamelotMajor[12] = { "8B", "3B", "10B", "5B", "12B", "7B",
                                        "2B", "9B", "4B", "11B", "6B", "1B" };

constexpr int kFftOrder = 12;
constexpr int kFftSize = 1 << kFftOrder;
constexpr int kHop = 4096;
constexpr double kMinFreq = 55.0;
constexpr double kMaxFreq = 2100.0;
} // namespace

juce::String KeyDetector::camelotFor(int rootPc, bool minor)
{
    if (rootPc < 0 || rootPc > 11)
        return "--";
    return minor ? kCamelotMinor[rootPc] : kCamelotMajor[rootPc];
}

KeyAnalysisResult KeyDetector::analyze(juce::AudioFormatReader* reader, double maxSeconds)
{
    KeyAnalysisResult result;
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
        return result;

    const double sr = reader->sampleRate;
    const double fileSec = (double)reader->lengthInSamples / sr;
    double span = (maxSeconds > 0.0) ? juce::jmin(fileSec, maxSeconds) : fileSec;
    // Ventana centrada (evita intros/outros atípicos cuando el tema es largo)
    double startSec = (fileSec > span) ? (fileSec - span) * 0.5 : 0.0;
    juce::int64 startSample = (juce::int64)(startSec * sr);
    juce::int64 totalSamples = juce::jmin(reader->lengthInSamples - startSample,
                                          (juce::int64)(span * sr));
    if (totalSamples < kFftSize)
        return result;

    const int numCh = (int)reader->numChannels;
    juce::AudioBuffer<float> window;
    window.setSize(numCh, kFftSize);

    juce::dsp::FFT fft(kFftOrder);
    std::vector<float> fftData((size_t)kFftSize * 2, 0.0f);

    // Hann precalculada
    std::vector<float> hann((size_t)kFftSize);
    for (int i = 0; i < kFftSize; ++i)
        hann[(size_t)i] = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * (float)i / (float)(kFftSize - 1));

    double chroma[12] = { 0.0 };
    juce::int64 pos = startSample;
    const juce::int64 end = startSample + totalSamples;
    int windows = 0;

    while (pos + kFftSize <= end)
    {
        if (!reader->read(&window, 0, kFftSize, pos, true, true))
            break;

        for (int i = 0; i < kFftSize; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                s += window.getReadPointer(ch)[i];
            fftData[(size_t)i] = (s / (float)juce::jmax(1, numCh)) * hann[(size_t)i];
        }

        fft.performFrequencyOnlyForwardTransform(fftData.data(), true);

        for (int k = 1; k <= kFftSize / 2; ++k)
        {
            double freq = (double)k * sr / (double)kFftSize;
            if (freq < kMinFreq || freq > kMaxFreq)
                continue;
            int pc = (int)std::lround(12.0 * std::log2(freq / 440.0)) % 12;
            if (pc < 0) pc += 12;
            chroma[pc] += std::sqrt((double)fftData[(size_t)k]);
        }

        ++windows;
        pos += kHop;
    }

    if (windows < 4)
        return result;

    // Normaliza el cromagrama
    double maxC = 0.0;
    for (int i = 0; i < 12; ++i)
        maxC = juce::jmax(maxC, chroma[i]);
    if (maxC <= 0.0)
        return result;
    for (int i = 0; i < 12; ++i)
        chroma[i] /= maxC;

    // Correlación con plantillas rotadas (12 raíces x mayor/menor)
    double best = -2.0, second = -2.0;
    int bestRoot = -1;
    bool bestMinor = true;
    for (int root = 0; root < 12; ++root)
    {
        for (int m = 0; m < 2; ++m)
        {
            const float* tmpl = (m == 0) ? kMajorTemplate : kMinorTemplate;
            double corr = 0.0, sumC = 0.0, sumT = 0.0, sumCC = 0.0, sumTT = 0.0;
            for (int i = 0; i < 12; ++i)
            {
                double c = chroma[i];
                double t = (double)tmpl[(i - root + 12) % 12];
                corr += c * t;
                sumC += c;
                sumT += t;
                sumCC += c * c;
                sumTT += t * t;
            }
            double denom = std::sqrt((sumCC - sumC * sumC / 12.0) * (sumTT - sumT * sumT / 12.0));
            double r = (denom > 1e-9) ? (corr - sumC * sumT / 12.0) / denom : -1.0;
            if (r > best)
            {
                second = best;
                best = r;
                bestRoot = root;
                bestMinor = (m == 1);
            }
            else if (r > second)
            {
                second = r;
            }
        }
    }

    if (bestRoot < 0)
        return result;

    result.rootPc = bestRoot;
    result.isMinor = bestMinor;
    result.camelot = camelotFor(bestRoot, bestMinor);
    result.confidence = (float)juce::jlimit(0.0, 1.0, (best - second) / juce::jmax(0.15, best));
    return result;
}
