#include "LibraryCacheManager.h"

LibraryCacheManager& LibraryCacheManager::getInstance()
{
    static LibraryCacheManager instance;
    return instance;
}

LibraryCacheManager::LibraryCacheManager()
{
    loadCache();
}

LibraryCacheManager::~LibraryCacheManager()
{
    saveCache();
}

juce::File LibraryCacheManager::getCacheFile() const
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("DiscNativePro");
    if (!appData.isDirectory())
        appData.createDirectory();
    return appData.getChildFile("library_cache.json");
}

size_t LibraryCacheManager::getCachedCount()
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.size();
}

void LibraryCacheManager::clear()
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.clear();
    isDirty = true;
    saveCache();
}

bool LibraryCacheManager::getTrack(const juce::File& file, CachedTrackMetadata& outMeta)
{
    auto path = file.getFullPathName();
    auto modTime = file.getLastModificationTime().toMilliseconds();
    auto fileSize = file.getSize();

    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cache.find(path);
    if (it != cache.end())
    {
        // Check if file has changed on disk
        if (it->second.lastModifiedTime == modTime && it->second.fileSizeBytes == fileSize)
        {
            outMeta = it->second;
            return true;
        }
    }
    return false;
}

void LibraryCacheManager::storeTrack(const juce::File& file, const CachedTrackMetadata& meta, bool autoSave)
{
    auto path = file.getFullPathName();

    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cache[path] = meta;
        isDirty = true;
    }

    if (autoSave)
    {
        auto now = juce::Time::getMillisecondCounter();
        if (now - lastSaveTick > 5000)
        {
            saveCache();
            lastSaveTick = now;
        }
    }
}

void LibraryCacheManager::updateBpmAndKey(const juce::File& file, double bpm, const juce::String& key)
{
    auto path = file.getFullPathName();
    bool updated = false;

    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cache.find(path);
        if (it != cache.end())
        {
            if (bpm > 0.0) it->second.bpm = bpm;
            if (key.isNotEmpty() && key != "--") it->second.key = key;
            isDirty = true;
            updated = true;
        }
    }

    if (updated)
    {
        auto now = juce::Time::getMillisecondCounter();
        if (now - lastSaveTick > 3000)
        {
            saveCache();
            lastSaveTick = now;
        }
    }
}

void LibraryCacheManager::loadCache()
{
    auto file = getCacheFile();
    if (!file.existsAsFile())
        return;

    auto jsonContent = file.loadFileAsString();
    if (jsonContent.isEmpty())
        return;

    auto parsed = juce::JSON::parse(jsonContent);
    if (!parsed.isObject())
        return;

    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return;

    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.clear();

    for (const auto& prop : obj->getProperties())
    {
        auto path = prop.name.toString();
        const auto& val = prop.value;
        if (val.isObject())
        {
            auto* itemObj = val.getDynamicObject();
            if (itemObj != nullptr)
            {
                CachedTrackMetadata meta;
                meta.title = itemObj->getProperty("title").toString();
                meta.artist = itemObj->getProperty("artist").toString();
                meta.album = itemObj->getProperty("album").toString();
                meta.duration = static_cast<double>(itemObj->getProperty("duration"));
                meta.bpm = static_cast<double>(itemObj->getProperty("bpm"));
                meta.key = itemObj->getProperty("key").toString();
                meta.lastModifiedTime = static_cast<juce::int64>(itemObj->getProperty("modTime"));
                meta.fileSizeBytes = static_cast<juce::int64>(itemObj->getProperty("size"));

                cache[path] = meta;
            }
        }
    }
    isDirty = false;
}

void LibraryCacheManager::saveCache()
{
    std::unordered_map<juce::String, CachedTrackMetadata> snapshot;
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (!isDirty && getCacheFile().existsAsFile())
            return;
        snapshot = cache;
        isDirty = false;
        lastSaveTick = juce::Time::getMillisecondCounter();
    }

    auto rootObj = std::make_unique<juce::DynamicObject>();
    for (const auto& [path, meta] : snapshot)
    {
        auto itemObj = std::make_unique<juce::DynamicObject>();
        itemObj->setProperty("title", meta.title);
        itemObj->setProperty("artist", meta.artist);
        itemObj->setProperty("album", meta.album);
        itemObj->setProperty("duration", meta.duration);
        itemObj->setProperty("bpm", meta.bpm);
        itemObj->setProperty("key", meta.key);
        itemObj->setProperty("modTime", meta.lastModifiedTime);
        itemObj->setProperty("size", meta.fileSizeBytes);

        rootObj->setProperty(path, juce::var(itemObj.release()));
    }

    auto jsonString = juce::JSON::toString(juce::var(rootObj.release()), true);
    auto file = getCacheFile();
    juce::TemporaryFile tempFile(file);
    if (tempFile.getFile().replaceWithText(jsonString))
    {
        tempFile.overwriteTargetFileWithTemporary();
    }
}
