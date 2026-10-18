#pragma once

// Analizador Armónico Profesional para DiscNativePro (C++20 / JUCE 8)
// Pipeline de vanguardia: Detección de afinación (detuning cents), Spectral Whitening,
// Dual-Band Chroma (Bajo vs Armonía), Perfiles Sha'ath/EDMA con ponderación de tónica en graves.
// Notación dual: Camelot + Musical (ej. "8A / Am") con codificación de color armónico.

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_graphics/juce_graphics.h>

struct KeyAnalysisResult
{
    juce::String camelot { "--" };      // ej. "8A", "11B"
    juce::String musicalKey { "--" };   // ej. "Am", "C"
    juce::String formattedKey { "--" }; // ej. "8A / Am", "11B / Bb"
    int rootPc { -1 };                  // pitch class 0..11 (0 = C), -1 = desconocido
    bool isMinor { true };
    float confidence { 0.0f };          // 0..1 (margen relativo de certeza)
    float tuningCents { 0.0f };         // Desviación en cents respecto a A=440Hz
};

class KeyDetector
{
public:
    // Analiza un archivo de audio con el algoritmo armónico profesional
    static KeyAnalysisResult analyze(juce::AudioFormatReader* reader, double maxSeconds = 60.0);

    // Helpers de notación y conversión
    static juce::String camelotFor(int rootPc, bool minor);
    static juce::String musicalKeyFor(int rootPc, bool minor);
    static juce::String formatFullKey(int rootPc, bool minor);
    static juce::String formatKeyString(const juce::String& rawKey);

    // Rueda Camelot: Color oficial para visualización en Decks y Biblioteca
    static juce::Colour getCamelotColour(const juce::String& keyStr);

    // Compatibilidad armónica (Camelot Wheel: misma clave, +/-1 número, o cambio A<->B)
    static bool areKeysCompatible(const juce::String& keyA, const juce::String& keyB);
};
