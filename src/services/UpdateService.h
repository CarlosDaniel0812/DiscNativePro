#pragma once

#include <juce_core/juce_core.h>
#include <functional>

// Resultado del chequeo de actualización contra GitHub Releases.
// Todo el chequeo corre en background thread; el callback llega en Message Thread.
struct UpdateInfo
{
    bool available { false };
    juce::String currentVersion;
    juce::String latestTag;      // p.ej. "v1.2.0" (tal cual viene de la API)
    juce::String latestVersion;  // sin 'v': "1.2.0"
    juce::String releaseName;
    juce::String releaseNotes;   // body del release (puede estar vacío)
    juce::String downloadUrl;    // asset de esta plataforma, o html_url como fallback
    juce::String pageUrl;        // html_url del release
};

class UpdateService
{
public:
    using UpdateCallback = std::function<void(const UpdateInfo&)>;

    // Lanza el chequeo en background y responde en Message Thread.
    // Nunca lanza: sin red, timeout, rate-limit o tag omitido => available=false.
    static void checkForUpdatesAsync(UpdateCallback onComplete);

    static juce::String getCurrentVersion();
    static bool isNewerVersion(const juce::String& latest, const juce::String& current);

    static juce::String getSkippedTag();
    static void setSkippedTag(const juce::String& tag);

private:
    static juce::File getSettingsFile();
    static int compareVersions(const juce::String& a, const juce::String& b);
};
