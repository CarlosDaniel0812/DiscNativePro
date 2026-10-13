#pragma once

#include <juce_core/juce_core.h>
#include <vector>

enum class Language { Spanish, English };
enum class LanguageSetting { System, Spanish, English };

class LanguageManager
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void languageChanged() = 0;
    };

    static LanguageManager& getInstance()
    {
        static LanguageManager instance;
        return instance;
    }

    Language getCurrentLanguage() const { return currentLang; }
    LanguageSetting getLanguageSetting() const { return langSetting; }
    bool isSpanish() const { return currentLang == Language::Spanish; }
    bool isEnglish() const { return currentLang == Language::English; }

    void setLanguageSetting(LanguageSetting setting);

    // Core translation helper: returns es or en depending on current language
    static juce::String tr(const juce::String& es, const juce::String& en)
    {
        return getInstance().isSpanish() ? es : en;
    }

    static juce::String trUtf8(const char* es, const char* en)
    {
        return getInstance().isSpanish() ? juce::String::fromUTF8(es) : juce::String::fromUTF8(en);
    }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    LanguageManager();
    ~LanguageManager() = default;

    Language resolveSystemLanguage() const;
    void loadFromSettings();
    void saveToSettings();
    void notifyListeners();

    Language currentLang { Language::Spanish };
    LanguageSetting langSetting { LanguageSetting::System };
    std::vector<Listener*> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LanguageManager)
};

// Convenience macro for inline translation
#define TR(es, en) LanguageManager::tr(es, en)
