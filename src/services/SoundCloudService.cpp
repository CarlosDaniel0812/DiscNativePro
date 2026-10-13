#include "SoundCloudService.h"
#include <juce_events/juce_events.h>
#include <thread>
#include <mutex>
#include <map>

namespace
{
    std::mutex scImageCacheMutex;
    std::map<juce::String, juce::Image> scArtworkCache;
    juce::String customClientId;
    const char* defaultClientId = "O4KAW11rMMnH5modEfDHOdzmnuvsck9X";
}

juce::String SoundCloudService::getClientId()
{
    return customClientId.isNotEmpty() ? customClientId : juce::String(defaultClientId);
}

void SoundCloudService::setClientId(const juce::String& id)
{
    customClientId = id.trim();
}

juce::File SoundCloudService::getCacheDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("DiscNativePro")
                   .getChildFile("soundcloud_cache");
    if (!dir.exists())
        dir.createDirectory();
    return dir;
}

std::vector<SoundCloudTrackItem> SoundCloudService::search(const juce::String& query)
{
    std::vector<SoundCloudTrackItem> results;
    auto q = query.trim();
    if (q.isEmpty())
        return results;

    juce::String encoded = juce::URL::addEscapeChars(q, true);
    juce::String urlStr = "https://api-v2.soundcloud.com/search/tracks?q=" + encoded
                        + "&client_id=" + getClientId()
                        + "&limit=40";

    juce::URL searchUrl(urlStr);
    auto stream = searchUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                  .withConnectionTimeoutMs(8000)
                                                  .withExtraHeaders("User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)\r\n"));
    if (stream == nullptr)
        return results;

    juce::String jsonText = stream->readEntireStreamAsString();
    auto parsed = juce::JSON::parse(jsonText);
    if (!parsed.isObject() || !parsed.hasProperty("collection"))
        return results;

    auto collection = parsed["collection"];
    if (!collection.isArray())
        return results;

    for (int i = 0; i < collection.size(); ++i)
    {
        auto item = collection[i];
        if (!item.isObject())
            continue;

        SoundCloudTrackItem track;
        track.id = item["id"].toString();
        track.title = item["title"].toString();

        if (item.hasProperty("user") && item["user"].isObject())
            track.user = item["user"]["username"].toString();

        int durMs = item["duration"].toString().getIntValue();
        track.durationSeconds = durMs / 1000;
        int mins = track.durationSeconds / 60;
        int secs = track.durationSeconds % 60;
        track.durationText = juce::String::formatted("%02d:%02d", mins, secs);

        track.artworkUrl = item["artwork_url"].toString();
        if (track.artworkUrl.isEmpty() && item.hasProperty("user") && item["user"].isObject())
            track.artworkUrl = item["user"]["avatar_url"].toString();

        track.permalinkUrl = item["permalink_url"].toString();

        // Extract progressive MP3 stream auth URL
        if (item.hasProperty("media") && item["media"].isObject() && item["media"].hasProperty("transcodings"))
        {
            auto transcodings = item["media"]["transcodings"];
            if (transcodings.isArray())
            {
                for (int t = 0; t < transcodings.size(); ++t)
                {
                    auto fmt = transcodings[t];
                    if (fmt.isObject() && fmt.hasProperty("format") && fmt["format"].isObject())
                    {
                        auto proto = fmt["format"]["protocol"].toString();
                        if (proto.equalsIgnoreCase("progressive"))
                        {
                            track.progressiveStreamAuthUrl = fmt["url"].toString();
                            break;
                        }
                    }
                }
            }
        }

        if (track.title.isNotEmpty())
            results.push_back(track);
    }

    return results;
}

void SoundCloudService::searchAsync(const juce::String& query,
                                    std::function<void(const std::vector<SoundCloudTrackItem>&)> onComplete)
{
    std::thread([query, cb = std::move(onComplete)]() {
        auto res = search(query);
        juce::MessageManager::callAsync([cb, res]() {
            if (cb)
                cb(res);
        });
    }).detach();
}

void SoundCloudService::downloadTrackAsync(const SoundCloudTrackItem& item,
                                           std::function<void(const juce::File&)> onDownloaded)
{
    if (item.id.isEmpty() || item.progressiveStreamAuthUrl.isEmpty())
    {
        if (onDownloaded)
            onDownloaded(juce::File());
        return;
    }

    auto cacheDir = getCacheDirectory();
    auto targetFile = cacheDir.getChildFile("sc_" + item.id + ".mp3");

    if (targetFile.existsAsFile() && targetFile.getSize() > 10000)
    {
        juce::MessageManager::callAsync([onDownloaded, targetFile]() {
            if (onDownloaded)
                onDownloaded(targetFile);
        });
        return;
    }

    std::thread([item, targetFile, cb = std::move(onDownloaded)]() {
        // Step 1: Resolve progressive stream auth URL
        juce::String authUrlStr = item.progressiveStreamAuthUrl;
        if (!authUrlStr.contains("client_id="))
            authUrlStr += (authUrlStr.contains("?") ? "&" : "?") + juce::String("client_id=") + getClientId();

        juce::URL authUrl(authUrlStr);
        auto authStream = authUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                        .withConnectionTimeoutMs(8000)
                                                        .withExtraHeaders("User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)\r\n"));

        juce::String directStreamUrl;
        if (authStream != nullptr)
        {
            auto parsed = juce::JSON::parse(authStream->readEntireStreamAsString());
            if (parsed.isObject() && parsed.hasProperty("url"))
                directStreamUrl = parsed["url"].toString();
        }

        if (directStreamUrl.isEmpty())
        {
            juce::MessageManager::callAsync([cb]() {
                if (cb)
                    cb(juce::File());
            });
            return;
        }

        // Step 2: Download MP3 stream
        juce::URL downloadUrl(directStreamUrl);
        auto dlStream = downloadUrl.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                          .withConnectionTimeoutMs(15000)
                                                          .withExtraHeaders("User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)\r\n"));
        if (dlStream != nullptr)
        {
            auto tempFile = targetFile.getSiblingFile(targetFile.getFileName() + ".tmp");
            tempFile.deleteFile();
            {
                juce::FileOutputStream fos(tempFile);
                if (fos.openedOk())
                    fos.writeFromInputStream(*dlStream, -1);
            }

            if (tempFile.existsAsFile() && tempFile.getSize() > 10000)
            {
                tempFile.moveFileTo(targetFile);
                juce::MessageManager::callAsync([cb, targetFile]() {
                    if (cb)
                        cb(targetFile);
                });
                return;
            }
        }

        juce::MessageManager::callAsync([cb]() {
            if (cb)
                cb(juce::File());
        });
    }).detach();
}

void SoundCloudService::loadArtworkAsync(const juce::String& artworkUrl,
                                         std::function<void(const juce::Image&)> onImageLoaded)
{
    if (artworkUrl.isEmpty())
        return;

    {
        std::lock_guard<std::mutex> lock(scImageCacheMutex);
        auto it = scArtworkCache.find(artworkUrl);
        if (it != scArtworkCache.end() && it->second.isValid())
        {
            juce::MessageManager::callAsync([onImageLoaded, img = it->second]() {
                if (onImageLoaded)
                    onImageLoaded(img);
            });
            return;
        }
    }

    std::thread([artworkUrl, cb = std::move(onImageLoaded)]() {
        juce::URL url(artworkUrl);
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                .withConnectionTimeoutMs(6000));
        juce::Image img;
        if (stream != nullptr)
        {
            img = juce::ImageFileFormat::loadFrom(*stream);
        }

        if (img.isValid())
        {
            std::lock_guard<std::mutex> lock(scImageCacheMutex);
            scArtworkCache[artworkUrl] = img;
        }

        juce::MessageManager::callAsync([cb, img]() {
            if (cb)
                cb(img);
        });
    }).detach();
}
