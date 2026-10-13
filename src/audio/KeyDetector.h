#pragma once

// Detección de tonalidad in-house (sin dependencias): cromagrama promediado
// + correlación con plantillas Krumhansl mayor/menor → notación Camelot.
// Se corre junto a BeatDetector::analyze() en loadFile() (hilo de mensaje).

#include <juce_audio_formats/juce_audio_formats.h>

struct KeyAnalysisResult
{
    juce::String camelot { "--" }; // ej. "8A", "11B"
    int rootPc { -1 };             // pitch class 0..11 (0 = C), -1 = desconocido
    bool isMinor { true };
    float confidence { 0.0f };     // 0..1 (margen entre mejor y segunda opción)
};

class KeyDetector
{
public:
    // maxSeconds acota el análisis para no alargar la carga (0 = archivo entero)
    static KeyAnalysisResult analyze(juce::AudioFormatReader* reader, double maxSeconds = 180.0);

    static juce::String camelotFor(int rootPc, bool minor);
};
