#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/DjMixer.h"
#include "FxSlotComponent.h"
#include "DjButton.h"

class FxPanelComponent : public juce::Component
{
public:
    FxPanelComponent(DjMixer& djMixer);
    ~FxPanelComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Deck 1 FX slots
    FxSlotComponent slot1_1;
    FxSlotComponent slot1_2;
    FxSlotComponent slot1_3;

    // Center divider: Mode selector
    juce::Label fxTitleLabel;
    DjButton manualBtn;
    DjButton instantBtn;
    bool isInstantMode { false };

    // Deck 2 FX slots
    FxSlotComponent slot2_1;
    FxSlotComponent slot2_2;
    FxSlotComponent slot2_3;

    void updateModeButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPanelComponent)
};
