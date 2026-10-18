#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../ui/LibraryComponent.h"
#include "LibraryCacheManager.h"
#include <vector>
#include <functional>
#include <atomic>

class AsyncFolderScanner : public juce::Thread
{
public:
    AsyncFolderScanner(juce::AudioFormatManager& formatManager);
    ~AsyncFolderScanner() override;

    // Callbacks dispatched on the JUCE Message Thread
    std::function<void(const std::vector<TrackItem>& batch, uint64_t scanId)> onTracksDiscovered;
    std::function<void(int totalFound, const juce::String& currentFolderName, uint64_t scanId)> onProgress;
    std::function<void(int totalFound, uint64_t scanId)> onScanFinished;

    void startScan(const juce::File& rootFolder, const juce::String& baseFolderTag = {});
    void cancelScan();
    bool isScanning() const { return isThreadRunning(); }
    uint64_t getScanId() const { return currentScanId.load(); }

    void run() override;

    static bool isValidAudioFile(const juce::File& file);

private:
    void scanDirectoryRecursive(const juce::File& dir, const juce::File& rootDir, const juce::String& baseFolderTag, uint64_t scanId);
    bool processAudioFile(const juce::File& file, const juce::File& rootDir, const juce::String& baseFolderTag, TrackItem& outItem);
    void flushBatch(uint64_t scanId, bool isFinal = false);

    juce::AudioFormatManager& formatManager;
    juce::File rootToScan;
    juce::String rootBaseTag;

    std::atomic<uint64_t> currentScanId { 0 };
    std::atomic<bool> cancelRequested { false };
    std::atomic<int> tracksFoundCount { 0 };

    std::vector<TrackItem> batchBuffer;
    juce::uint32 lastBatchDispatchTick { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsyncFolderScanner)
};
