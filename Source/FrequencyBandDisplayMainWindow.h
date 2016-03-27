#ifndef __MAINCOMPONENT_H_7D44AE5B__
#define __MAINCOMPONENT_H_7D44AE5B__

#include "../JuceLibraryCode/JuceHeader.h"
class MeterComp;

#define MAX_BINS 128

class FrequencyBandDisplayMainWindow  : public Component,
                                        public ButtonListener,
                                        public MultiTimer,
                                        public Thread
{
public:
    //==============================================================================
    FrequencyBandDisplayMainWindow ();
    ~FrequencyBandDisplayMainWindow();

    //==============================================================================


private:
    /////////////////////////////
    enum
    {
        eTimerIdFastTimer = 0,
        eTimerId60FPSTimer
    };

    enum
    {
        eParseStateIdle = 0,
        eParseStateReading,
    };

    #define kBeginPacket '<'
    #define kEndPacket   '>'
    #define kBandData    'D'
    #define kBandLabels  'L'

    /////////////////////////////
    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    
    void timerCallback (int timerId) override;
    
    void run() override;

    void OpenSerialPort(void);
    void CloseSerialPort(void);
    void UpdateFrequencyBandsGui(void);

    /////////////////////////////
    ScopedPointer<SerialPort>            mSerialPort;
    ScopedPointer<SerialPortInputStream> mSerialPortInput;
	int			mParseState;

    int			mNumberOfBands;
	String		mRawSerialData;
	int			mFrequencyBandData[MAX_BINS];
    String      mFrequencyBandLabels[MAX_BINS];
	MeterComp*  mFrequencyBandMeters[MAX_BINS];
    TextButton* quitButton;
    
    CriticalSection mFrequencyBandDataLock;
    CriticalSection mFrequencyBandLabelLock;
    
    bool mDoGuiResize;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyBandDisplayMainWindow)
};

#endif  // __MAINCOMPONENT_H_7D44AE5B__
