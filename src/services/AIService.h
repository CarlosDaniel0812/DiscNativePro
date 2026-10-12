#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <vector>

enum class AIProvider
{
    Gemini,
    OpenAI,
    Claude,
    DeepSeek
};

struct AIRecommendation
{
    juce::String title;
    juce::String artist;
    double bpm { 0.0 };
    juce::String key;
    juce::String genre;
    juce::String reason;

    // Hybrid library resolution
    bool isLocal { false };
    juce::File localFile;
};

struct CurrentTrackContext
{
    juce::String title;
    juce::String artist;
    double bpm { 0.0 };
    juce::String key;
    bool isVideo { false };
    int activeDeck { 1 };
};

class AIService
{
public:
    static juce::File getSettingsFile();

    // Provider & Configuration
    static AIProvider getProvider();
    static void setProvider(AIProvider provider);
    static juce::String getProviderName(AIProvider provider);

    static juce::String getApiKey(AIProvider provider);
    static void setApiKey(AIProvider provider, const juce::String& key);

    static bool isEnabled();
    static void setEnabled(bool enabled);

    static bool isConfigured();

    // Asynchronous Recommendation Generation
    // Returns results on the JUCE Message Thread
    static void requestRecommendationsAsync(
        const CurrentTrackContext& context,
        std::function<void(const std::vector<AIRecommendation>& recommendations, const juce::String& errorMsg)> onComplete);

    // Helper to resolve whether a recommendation matches any track in local library
    static void resolveLocalMatches(std::vector<AIRecommendation>& recs,
                                    const std::vector<juce::File>& localLibraryFiles);

private:
    static juce::String getProviderKeyName(AIProvider provider);
    static juce::String buildPrompt(const CurrentTrackContext& context);
    static std::vector<AIRecommendation> parseResponse(AIProvider provider, const juce::String& rawJson, juce::String& outError);
};
