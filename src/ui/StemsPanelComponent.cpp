#include "StemsPanelComponent.h"

StemsPanelComponent::StemsPanelComponent(DjMixer& djMixer)
    : mixer(djMixer)
{
    auto setupKill = [this](DjButton& btn, int deck, int band) {
        btn.setFontSize(9.5f);
        btn.setCornerRadius(4.0f);
        btn.onClick = [this, deck, band]() { toggleKill(deck, band); };
        addAndMakeVisible(btn);
    };

    setupKill(lowKill1, 0, 0);
    setupKill(midKill1, 0, 1);
    setupKill(highKill1, 0, 2);
    setupKill(lowKill2, 1, 0);
    setupKill(midKill2, 1, 1);
    setupKill(highKill2, 1, 2);

    stemsTitleLabel.setText("STEMS", juce::dontSendNotification);
    stemsTitleLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    stemsTitleLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(148, 163, 184));
    stemsTitleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stemsTitleLabel);

    stemsSubLabel.setFont(juce::FontOptions(8.0f, juce::Font::plain));
    stemsSubLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(100, 116, 139));
    stemsSubLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(stemsSubLabel);

    LanguageManager::getInstance().addListener(this);
    refreshLanguage();
    updateKillButtons();
}

StemsPanelComponent::~StemsPanelComponent()
{
    LanguageManager::getInstance().removeListener(this);
}

void StemsPanelComponent::languageChanged()
{
    refreshLanguage();
}

void StemsPanelComponent::refreshLanguage()
{
    lowKill1.setText(LanguageManager::trUtf8("BAJO", "BASS"));
    midKill1.setText(LanguageManager::trUtf8("VOZ", "VOCAL"));
    highKill1.setText(LanguageManager::trUtf8("AGUDO", "HATS"));
    lowKill2.setText(LanguageManager::trUtf8("BAJO", "BASS"));
    midKill2.setText(LanguageManager::trUtf8("VOZ", "VOCAL"));
    highKill2.setText(LanguageManager::trUtf8("AGUDO", "HATS"));
    stemsSubLabel.setText(LanguageManager::trUtf8("Cortes EQ", "EQ Cuts"), juce::dontSendNotification);
    repaint();
}

std::atomic<float>& StemsPanelComponent::eqParam(int deck, int band)
{
    auto& ch = mixer.getChannel(deck);
    if (band == 0) return ch.eqLow;
    if (band == 1) return ch.eqMid;
    return ch.eqHigh;
}

void StemsPanelComponent::toggleKill(int deck, int band)
{
    if (deck < 0 || deck > 1 || band < 0 || band > 2)
        return;

    auto& param = eqParam(deck, band);
    if (!killed[deck][band])
    {
        savedEq[deck][band] = param.load();
        param.store(0.0f);
        killed[deck][band] = true;
    }
    else
    {
        param.store(savedEq[deck][band]);
        killed[deck][band] = false;
    }
    updateKillButtons();
}

void StemsPanelComponent::updateKillButtons()
{
    DjButton* btns[2][3] = { { &lowKill1, &midKill1, &highKill1 },
                             { &lowKill2, &midKill2, &highKill2 } };
    for (int d = 0; d < 2; ++d)
    {
        for (int b = 0; b < 3; ++b)
        {
            if (killed[d][b])
            {
                btns[d][b]->setCustomColours(juce::Colour::fromRGB(80, 28, 36),
                                             juce::Colour::fromRGB(248, 113, 113),
                                             juce::Colours::transparentBlack,
                                             juce::Colour::fromRGB(248, 113, 113).withAlpha(0.5f));
            }
            else
            {
                btns[d][b]->setCustomColours(juce::Colour::fromRGB(18, 20, 28),
                                             juce::Colour::fromRGB(100, 116, 139),
                                             juce::Colours::transparentBlack,
                                             juce::Colours::transparentBlack);
            }
        }
    }
}

void StemsPanelComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colour::fromRGB(12, 14, 19));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(juce::Colour::fromRGB(26, 30, 42));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void StemsPanelComponent::resized()
{
    auto area = getLocalBounds().reduced(5, 3);

    int centerWidth = 84;
    int deckAreaWidth = (area.getWidth() - centerWidth - 12) / 2;

    // Deck 1 kills (Left)
    auto leftArea = area.removeFromLeft(deckAreaWidth);
    int btnW1 = (leftArea.getWidth() - 8) / 3;
    lowKill1.setBounds(leftArea.removeFromLeft(btnW1));
    leftArea.removeFromLeft(4);
    midKill1.setBounds(leftArea.removeFromLeft(btnW1));
    leftArea.removeFromLeft(4);
    highKill1.setBounds(leftArea);

    // Deck 2 kills (Right)
    auto rightArea = area.removeFromRight(deckAreaWidth);
    int btnW2 = (rightArea.getWidth() - 8) / 3;
    lowKill2.setBounds(rightArea.removeFromLeft(btnW2));
    rightArea.removeFromLeft(4);
    midKill2.setBounds(rightArea.removeFromLeft(btnW2));
    rightArea.removeFromLeft(4);
    highKill2.setBounds(rightArea);

    // Center labels
    area.reduce(2, 0);
    stemsTitleLabel.setBounds(area.removeFromTop(20));
    stemsSubLabel.setBounds(area.removeFromTop(16));
}
