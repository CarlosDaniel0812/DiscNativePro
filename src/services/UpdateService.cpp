#include "UpdateService.h"
#include <juce_events/juce_events.h>

namespace
{
constexpr const char* kLatestReleaseApi =
    "https://api.github.com/repos/axel-molina/DiscNativePro/releases/latest";

#if JUCE_MAC
constexpr const char* kAssetName = "DiscNativePro-macOS-Universal.zip";
#elif JUCE_WINDOWS
constexpr const char* kAssetName = "DiscNativePro-Windows-x64.zip";
#else
constexpr const char* kAssetName = "DiscNativePro-Linux-x64.tar.gz";
#endif
} // namespace

juce::String UpdateService::getCurrentVersion()
{
#ifdef DISCNATIVEPRO_VERSION
    return DISCNATIVEPRO_VERSION;
#else
    return "1.0.0-dev";
#endif
}

int UpdateService::compareVersions(const juce::String& a, const juce::String& b)
{
    // Compara solo componentes numéricos ("1.2.0" vs "1.10.0" => -1).
    // Los sufijos (-dev, -rc1) se ignoran aquí; se resuelven en isNewerVersion.
    auto numParts = [](const juce::String& v) {
        juce::StringArray parts;
        parts.addTokens(v.upToFirstOccurrenceOf("-", false, false), ".", "");
        return parts;
    };

    auto pa = numParts(a.trim());
    auto pb = numParts(b.trim());
    int n = juce::jmax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i)
    {
        int va = i < pa.size() ? pa[i].getIntValue() : 0;
        int vb = i < pb.size() ? pb[i].getIntValue() : 0;
        if (va != vb)
            return va < vb ? -1 : 1;
    }
    return 0;
}

bool UpdateService::isNewerVersion(const juce::String& latest, const juce::String& current)
{
    juce::String l = latest.trim();
    juce::String c = current.trim();
    if (l.startsWithChar('v') || l.startsWithChar('V'))
        l = l.substring(1);
    if (c.startsWithChar('v') || c.startsWithChar('V'))
        c = c.substring(1);

    int cmp = compareVersions(l, c);
    // Comparación solo numérica: un empate ("1.0.0-3-gabc123" vs "1.0.0")
    // NO es update. Solo una versión numéricamente mayor avisa.
    // (Los builds locales ya no reportan un "-dev" genérico: CMake estampa
    // git describe, así que no hacen falta reglas especiales de sufijos.)
    return cmp > 0;
}

juce::File UpdateService::getSettingsFile()
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("DiscNativePro");
    if (!appData.exists())
        appData.createDirectory();
    return appData.getChildFile("settings.json");
}

juce::String UpdateService::getSkippedTag()
{
    auto file = getSettingsFile();
    if (!file.existsAsFile())
        return {};
    auto json = juce::JSON::parse(file);
    if (json.isObject() && json.hasProperty("skippedUpdateTag"))
        return json["skippedUpdateTag"].toString();
    return {};
}

void UpdateService::setSkippedTag(const juce::String& tag)
{
    auto file = getSettingsFile();
    juce::var json;
    if (file.existsAsFile())
        json = juce::JSON::parse(file);
    if (!json.isObject())
        json = juce::var(new juce::DynamicObject());

    if (auto* obj = json.getDynamicObject())
    {
        if (tag.isEmpty())
            obj->removeProperty("skippedUpdateTag");
        else
            obj->setProperty("skippedUpdateTag", tag);
    }
    file.replaceWithText(juce::JSON::toString(json));
}

void UpdateService::checkForUpdatesAsync(UpdateCallback onComplete)
{
    juce::Thread::launch([onComplete]() {
        UpdateInfo info;
        info.currentVersion = getCurrentVersion();

        juce::URL url(kLatestReleaseApi);
        std::unique_ptr<juce::InputStream> stream(url.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(10000)
                .withExtraHeaders("User-Agent: DiscNativePro-Updater\r\n"
                                  "Accept: application/vnd.github+json\r\n")));

        if (stream != nullptr)
        {
            auto json = juce::JSON::parse(stream->readEntireStreamAsString());
            if (json.isObject())
            {
                info.latestTag = json["tag_name"].toString().trim();
                info.latestVersion = info.latestTag;
                if (info.latestVersion.startsWithChar('v') || info.latestVersion.startsWithChar('V'))
                    info.latestVersion = info.latestVersion.substring(1);
                info.releaseName = json["name"].toString();
                info.releaseNotes = json["body"].toString();
                info.pageUrl = json["html_url"].toString();
                info.downloadUrl = info.pageUrl;

                if (auto* assets = json["assets"].getArray())
                {
                    for (auto& a : *assets)
                    {
                        auto name = a["name"].toString();
                        auto dl = a["browser_download_url"].toString();
                        if (name == kAssetName && dl.isNotEmpty())
                        {
                            info.downloadUrl = dl;
                            break;
                        }
                    }
                }

                if (info.latestVersion.isNotEmpty()
                    && info.latestTag != getSkippedTag()
                    && isNewerVersion(info.latestVersion, info.currentVersion))
                {
                    info.available = true;
                }
            }
        }

        juce::MessageManager::callAsync([onComplete, info]() { onComplete(info); });
    });
}
