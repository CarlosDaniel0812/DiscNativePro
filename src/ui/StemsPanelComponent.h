#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/DjMixer.h"
#include "../services/LanguageManager.h"
#include "DjButton.h"

// Tira STEMS por cortes de EQ (no separación neuronal): 3 kills por deck
// (BAJO=low, VOZ=mid, AGUDO=high) sobre el EQ Isolator existente.
// Al activar guarda el valor del knob y pone Kill (0.0); al soltar restaura.
class StemsPanelComponent : public juce::Component,
                            public LanguageManager::Listener
{
public:
    StemsPanelComponent(DjMixer& djMixer);
    ~StemsPanelComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void languageChanged() override;
    void refreshLanguage();

private:
    // band: 0 = Low, 1 = Mid, 2 = High
    void toggleKill(int deck, int band);
    std::atomic<float>& eqParam(int deck, int band);
    void updateKillButtons();

    DjMixer& mixer;

    DjButton lowKill1, midKill1, highKill1;
    DjButton lowKill2, midKill2, highKill2;
    juce::Label stemsTitleLabel;
    juce::Label stemsSubLabel;

    bool killed[2][3] { { false, false, false }, { false, false, false } };
    float savedEq[2][3] { { 0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemsPanelComponent)
};
