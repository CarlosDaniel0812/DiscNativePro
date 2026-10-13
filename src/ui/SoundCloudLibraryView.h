#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../services/SoundCloudService.h"
#include "DjButton.h"
#include "LucideIcons.h"
#include "../services/LanguageManager.h"
#include <vector>
#include <functional>

class SoundCloudLibraryView : public juce::Component,
                             public juce::TableListBoxModel,
                             public LanguageManager::Listener
{
public:
    SoundCloudLibraryView();
    ~SoundCloudLibraryView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void languageChanged() override;
    void refreshLanguage();

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>& currentlySelectedRows) override;

    void performSearch(const juce::String& query);

    std::function<void(int deckIndex, const juce::File& file, const juce::String& title, const juce::String& artist)> onLoadTrack;

private:
    juce::Label headerTitle;
    juce::TextEditor searchBox;
    DjButton searchBtn;
    juce::Label statusLabel;

    juce::TableListBox table;
    std::vector<SoundCloudTrackItem> searchResults;
    bool isSearching { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundCloudLibraryView)
};
