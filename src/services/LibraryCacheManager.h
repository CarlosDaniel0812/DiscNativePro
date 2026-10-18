#pragma once

#include <juce_core/juce_core.h>
#include <unordered_map>
#include <mutex>

struct CachedTrackMetadata
{
    juce::String title;
    juce::String artist;
    juce::String album;
    double duration { 0.0 };
    double bpm { 0.0 };
    juce::String key;
    juce::int64 lastModifiedTime { 0 };
    juce::int64 fileSizeBytes { 0 };
};

class LibraryCacheManager
{
public:
    static LibraryCacheManager& getInstance();

    bool getTrack(const juce::File& file, CachedTrackMetadata& outMeta);
    void storeTrack(const juce::File& file, const CachedTrackMetadata& meta, bool autoSave = false);
    void updateBpmAndKey(const juce::File& file, double bpm, const juce::String& key);

    void loadCache();
    void saveCache();

    size_t getCachedCount();
    void clear();

private:
    LibraryCacheManager();
    ~LibraryCacheManager();

    juce::File getCacheFile() const;

    std::mutex cacheMutex;
    std::unordered_map<juce::String, CachedTrackMetadata> cache; // Key: Canonical Full Path
    bool isDirty { false };
    juce::uint32 lastSaveTick { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryCacheManager)
};
