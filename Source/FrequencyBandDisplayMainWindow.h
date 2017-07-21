#ifndef __MAINCOMPONENT_H_7D44AE5B__
#define __MAINCOMPONENT_H_7D44AE5B__

#include "../JuceLibraryCode/JuceHeader.h"
#include "FrequencyBandDevice.h"

class MeterComp;
class cSerialPortListMonitor;
class SerialPortMenu;

class FrequencyBandDisplayMainWindow  : public Component,
                                        public ButtonListener,
                                        public MultiTimer
{
public:
    //==============================================================================
    FrequencyBandDisplayMainWindow (ApplicationProperties* applicationProperties);
    ~FrequencyBandDisplayMainWindow();

    //==============================================================================

private:
    /////////////////////////////
    enum
    {
        eTimerIdResizeThrottleTimer = 0,
        eTimerIdGuiUpdateTimer,
        eTimerCheckSerialPortChange
    };


    /////////////////////////////
    void paint (Graphics& _g) override;
    void resized() override;
    void buttonClicked (Button* _buttonThatWasClicked) override;
    
    void timerCallback (int _timerId) override;
    
    void UpdateFrequencyBandsGui(void);

    /////////////////////////////
    FrequencyBandDevice           mFrequencyBandDevice;
    OwnedArray<MeterComp>         mFrequencyBandMeters;
    int                           mNumberOfBandsDisplayed {0 };
    ScopedPointer<TextButton>     mQuitButton;
    ScopedPointer<Label>          mComPortLabel;
    ScopedPointer<SerialPortMenu> mComPortName;

    bool mDoGuiResize { false };

    ApplicationProperties* mApplicationProperties{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyBandDisplayMainWindow)
};

#endif  // __MAINCOMPONENT_H_7D44AE5B__
