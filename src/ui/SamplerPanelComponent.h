#pragma once

// Tira de sampler one-shot (8 pads): click vacío = cargar, click = trigger,
// click-derecho = menú (trigger/unload). Sin persistencia en v1.

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../audio/SamplerBank.h"
#include "../services/LanguageManager.h"
#include "DjButton.h"
#include "DjKnob.h"
#include <functional>

class SamplerPanelComponent : public juce::Component,
                              public LanguageManager::Listener
{
public:
    SamplerPanelComponent(SamplerBank& bank,
                          juce::AudioFormatManager& formatManager,
                          std::function<double()> sampleRateProvider);
    ~SamplerPanelComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void languageChanged() override;
    void refreshLanguage();
    void mouseDown(const juce::MouseEvent& e) override;

private:
    void refreshPad(int index);
    void refreshAllPads();
    void chooseSampleFor(int index);
    void showPadMenu(int index);

    SamplerBank& sampler;
    juce::AudioFormatManager& formats;
    std::function<double()> getSampleRate;

    juce::Label titleLabel;
    DjButton stopAllBtn;
    DjKnob volKnob { "VOL" };
    DjButton pads[SamplerBank::numSlots];

    std::unique_ptr<juce::FileChooser> fileChooser;
    int pendingSlot { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerPanelComponent)
};
