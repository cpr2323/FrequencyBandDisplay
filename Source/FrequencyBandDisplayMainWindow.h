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
    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    
    void timerCallback (int timerId) override;
    
    void UpdateFrequencyBandsGui(void);

    /////////////////////////////
    FrequencyBandDevice           mFrequencyBandDevice;
    OwnedArray<MeterComp>         mFrequencyBandMeters;
    int                           mNumberOfBandsDisplayed;
    ScopedPointer<TextButton>     mQuitButton;
    ScopedPointer<Label>          mComPortLabel;
    ScopedPointer<SerialPortMenu> mComPortName;

    bool mDoGuiResize;

    ApplicationProperties* mApplicationProperties;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyBandDisplayMainWindow)
};

#endif  // __MAINCOMPONENT_H_7D44AE5B__
