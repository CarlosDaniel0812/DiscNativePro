#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct UsbDrive
{
    juce::String id;
    juce::String name;
    juce::File root;
    juce::String sizeDesc;
    bool isRemovable { true };
    bool isCustom { false };
};

class UsbDriveScanner
{
public:
    static std::vector<UsbDrive> getConnectedDrives();
    static bool isValidAudioFile(const juce::File& file);
    static juce::String formatBytes(juce::int64 bytes);
};
