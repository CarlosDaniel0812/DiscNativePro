#pragma once

// Tema global de colores de waveforms (Graves/Medios/Agudos) para los 3
// painters (overview, scrolling, vertical). Caché en memoria + persistencia
// en settings.json. Los painters repintan a 60 Hz: cambios en vivo, cero I/O.

#include <juce_gui_basics/juce_gui_basics.h>

class WaveformTheme
{
public:
    enum Preset { RgbMulticolor = 0, NeonBlue = 1, SilverMono = 2 };

    static juce::Colour low()  { return cached().low; }
    static juce::Colour mid()  { return cached().mid; }
    static juce::Colour high() { return cached().high; }

    static void setLow(juce::Colour c)  { cached().low = c; saveToDisk(cached()); }
    static void setMid(juce::Colour c)  { cached().mid = c; saveToDisk(cached()); }
    static void setHigh(juce::Colour c) { cached().high = c; saveToDisk(cached()); }

    static void applyPreset(int preset);

private:
    struct Theme
    {
        juce::Colour low  { juce::Colour::fromRGB(230, 57, 70) };
        juce::Colour mid  { juce::Colour::fromRGB(255, 159, 28) };
        juce::Colour high { juce::Colour::fromRGB(0, 229, 255) };
    };

    static Theme& cached();
    static Theme loadFromDisk();
    static void saveToDisk(const Theme& theme);
    static juce::File getSettingsFile();
};
