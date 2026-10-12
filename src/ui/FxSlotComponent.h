#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/DeckFxChain.h"
#include "DjButton.h"

class FxSlotComponent : public juce::Component
{
public:
    FxSlotComponent(FxSlot& fxSlot);
    ~FxSlotComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateUiFromSlot();

private:
    FxSlot& slot;

    // Top row: Effect selection
    DjButton prevBtn;
    DjButton fxNameBtn;
    DjButton nextBtn;

    // Bottom row: Controls
    DjButton onBtn;
    DjButton paramBtn;
    juce::Label dryLabel;
    juce::Slider wetDrySlider;
    juce::Label wetLabel;

    void showFxMenu();
    void cycleEffect(int delta);
    void cycleParam();
    juce::String getParamText() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxSlotComponent)
};
