#ifndef __MAINCOMPONENT_H_7D44AE5B__
#define __MAINCOMPONENT_H_7D44AE5B__

#include "../JuceLibraryCode/JuceHeader.h"
#include "FrequencyBandDevice.h"

class MeterComp;
class cSerialPortListMonitor;
class SerialPortMenu;

class FrequencyBandDisplayMainWindow  : public Component,
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
    
    void timerCallback (int timerId) override;
    
    void UpdateFrequencyBandsGui(void);

    /////////////////////////////
    FrequencyBandDevice   frequencyBandDevice;
    OwnedArray<MeterComp> frequencyBandMeters;
    int                   numberOfBandsDisplayed { 0 };

    TextButton quitButton;
    Label comPortLabel;
    std::unique_ptr<SerialPortMenu> comPortName;

    bool mDoGuiResize { false };

    ApplicationProperties* mApplicationProperties{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyBandDisplayMainWindow)
};

#endif  // __MAINCOMPONENT_H_7D44AE5B__
