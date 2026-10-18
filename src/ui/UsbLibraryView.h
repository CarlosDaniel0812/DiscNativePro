#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../services/UsbDriveScanner.h"
#include "../services/LanguageManager.h"
#include "DjButton.h"
#include "LucideIcons.h"
#include "LibraryComponent.h"
#include <vector>
#include <functional>

class UsbLibraryView : public juce::Component,
                       public juce::TableListBoxModel,
                       public juce::Timer,
                       public LanguageManager::Listener
{
public:
    UsbLibraryView(juce::AudioFormatManager& formatManager);
    ~UsbLibraryView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void languageChanged() override;
    void refreshLanguage();

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>& currentlySelectedRows) override;

    // Callbacks
    std::function<void(int deckIndex, const juce::File& file)> onLoadTrack;

    void refreshDrives();
    void selectDrive(int index);
    const std::vector<TrackItem>& getAllTracks() const { return allUsbTracks; }
    void selectFolder(const juce::String& folderName);
    void openManualFolder();

private:
    void filterTracks(const juce::String& query);
    void updateSyncStatusLabel(int count, bool isScanning, const juce::String& currentFolder = {});

    juce::AudioFormatManager& formatManager;
    std::unique_ptr<class AsyncFolderScanner> asyncScanner;

    std::vector<UsbDrive> drives;
    int selectedDriveIndex { -1 };
    juce::String selectedFolderName { "Todas las pistas" };
    std::vector<juce::String> availableFolders;
    std::set<juce::String> discoveredFolderSet;
    std::unordered_set<juce::String> knownUsbPaths;

    std::vector<TrackItem> allUsbTracks;
    std::vector<TrackItem> filteredUsbTracks;

    bool isSyncing { false };
    juce::String currentlyScannedDriveId;

    // Left Sidebar controls
    juce::Label sidebarTitle;
    DjButton refreshBtn;
    DjButton addFolderBtn;

    // Sidebar Folder / Drive buttons
    juce::Viewport sidebarViewport;
    juce::Component sidebarContent;
    std::vector<std::unique_ptr<DjButton>> driveButtons;
    std::vector<std::unique_ptr<DjButton>> folderButtons;

    // Center controls (when connected)
    juce::Label headerTitle;
    juce::TextEditor searchBox;
    juce::Label trackCountLabel;
    juce::TableListBox table;

    // Center Empty State controls (when no drive)
    DjButton manualExploreBtn;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UsbLibraryView)
};
