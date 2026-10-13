#include "WaveformTheme.h"

juce::File WaveformTheme::getSettingsFile()
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("DiscNativePro");
    if (!appData.exists())
        appData.createDirectory();
    return appData.getChildFile("settings.json");
}

WaveformTheme::Theme& WaveformTheme::cached()
{
    static Theme theme = loadFromDisk();
    return theme;
}

WaveformTheme::Theme WaveformTheme::loadFromDisk()
{
    Theme theme;
    auto file = getSettingsFile();
    if (file.existsAsFile())
    {
        auto json = juce::JSON::parse(file);
        if (json.isObject())
        {
            // ARGB int round-trip (sin parseo de strings, sin riesgo de formato)
            if (json.hasProperty("waveLow"))
                theme.low = juce::Colour((juce::uint32)(int)json["waveLow"]);
            if (json.hasProperty("waveMid"))
                theme.mid = juce::Colour((juce::uint32)(int)json["waveMid"]);
            if (json.hasProperty("waveHigh"))
                theme.high = juce::Colour((juce::uint32)(int)json["waveHigh"]);
        }
    }
    return theme;
}

void WaveformTheme::saveToDisk(const Theme& theme)
{
    auto file = getSettingsFile();
    juce::var json;
    if (file.existsAsFile())
        json = juce::JSON::parse(file);
    if (!json.isObject())
        json = juce::var(new juce::DynamicObject());

    if (auto* obj = json.getDynamicObject())
    {
        obj->setProperty("waveLow", (int)theme.low.getARGB());
        obj->setProperty("waveMid", (int)theme.mid.getARGB());
        obj->setProperty("waveHigh", (int)theme.high.getARGB());
    }
    file.replaceWithText(juce::JSON::toString(json));
}

void WaveformTheme::applyPreset(int preset)
{
    Theme theme;
    if (preset == NeonBlue)
    {
        theme.low = juce::Colour::fromRGB(21, 101, 192);
        theme.mid = juce::Colour::fromRGB(41, 182, 246);
        theme.high = juce::Colour::fromRGB(0, 229, 255);
    }
    else if (preset == SilverMono)
    {
        theme.low = juce::Colour::fromRGB(85, 85, 85);
        theme.mid = juce::Colour::fromRGB(153, 153, 153);
        theme.high = juce::Colour::fromRGB(238, 238, 238);
    }
    // RgbMulticolor = defaults del struct
    cached() = theme;
    saveToDisk(cached());
}
