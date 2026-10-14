#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <functional>

struct SoundCloudTrackItem
{
    juce::String id;
    juce::String title;
    juce::String user;
    int durationSeconds { 0 };
    juce::String durationText { "00:00" };
    juce::String artworkUrl;
    juce::String permalinkUrl;
    juce::String progressiveStreamAuthUrl; // URL from media.transcodings with protocol=progressive (MP3)
    juce::String hlsStreamAuthUrl;         // URL from media.transcodings with protocol=hls (AAC fMP4)
    bool isHls { false };
};

class SoundCloudService
{
public:
    // Synchronous search (call from background thread)
    static std::vector<SoundCloudTrackItem> search(const juce::String& query);

    // Asynchronous search with completion dispatched to message thread
    static void searchAsync(const juce::String& query,
                            std::function<void(const std::vector<SoundCloudTrackItem>&)> onComplete);

    // Downloads the track audio (MP3 or HLS AAC .m4a) to local cache and returns the local File
    static void downloadTrackAsync(const SoundCloudTrackItem& item,
                                  std::function<void(const juce::File&)> onDownloaded);

    // Asynchronously loads or retrieves cached artwork image
    static void loadArtworkAsync(const juce::String& artworkUrl,
                                 std::function<void(const juce::Image&)> onImageLoaded);

    // Helper to get local cache folder
    static juce::File getCacheDirectory();

    // Client ID management
    static juce::String getClientId();
    static void setClientId(const juce::String& id);

private:
    static juce::File downloadHlsStream(const juce::String& hlsAuthUrl, const juce::String& trackId);
};
