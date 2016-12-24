#ifndef __MAINCOMPONENT_H_7D44AE5B__
#define __MAINCOMPONENT_H_7D44AE5B__

#include "../JuceLibraryCode/JuceHeader.h"
class MeterComp;
class cSerialPortListMonitor;
class SerialPortMenu;

#define MAX_BINS 128

enum MenuIDs
{
    eUsbPortSelectStart = 1500,
    eUsbPortSelectEnd = 1598,
    eUsbPortSelectNoneAvail = 1599,
};

class FrequencyBandDisplayMainWindow  : public Component,
                                        public ButtonListener,
                                        public MultiTimer,
                                        public Thread
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
        eTimerIdFastTimer = 0,
        eTimerIdGuiUpdateTimer,
        eTimerOpenSerialPort
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
    String                               mCurrentSerialPortName;
    ScopedPointer<SerialPort>            mSerialPort;
    ScopedPointer<SerialPortInputStream> mSerialPortInput;
	int			mParseState;

    int			mNumberOfBands;
	String		mRawSerialData;
	int			mFrequencyBandData[MAX_BINS];
    String      mFrequencyBandLabels[MAX_BINS];
	MeterComp*  mFrequencyBandMeters[MAX_BINS];
    TextButton* mQuitButton;
    Label*      mComPortLabel;
    SerialPortMenu* mComPortName;
    
    CriticalSection mFrequencyBandDataLock;
    CriticalSection mFrequencyBandLabelLock;

    bool mDoGuiResize;

    ApplicationProperties* mApplicationProperties;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyBandDisplayMainWindow)
};

#endif  // __MAINCOMPONENT_H_7D44AE5B__
