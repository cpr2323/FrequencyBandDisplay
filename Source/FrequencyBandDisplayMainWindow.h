#ifndef __MAINCOMPONENT_H_7D44AE5B__
#define __MAINCOMPONENT_H_7D44AE5B__

#include "../JuceLibraryCode/JuceHeader.h"
class MeterComp;

#define MAX_BINS 128

class FrequencyBandDisplayMainWindow  : public Component,
                                        public ButtonListener,
                                        public MultiTimer
{
public:
    //==============================================================================
    FrequencyBandDisplayMainWindow ();
    ~FrequencyBandDisplayMainWindow();

    //==============================================================================

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

	enum
	{
		eTimerId1ms = 0,
		eTimerId16ms
	};

	void timerCallback (int timerId);

private:
    void OpenSerialPort(void);
    void CloseSerialPort(void);

	enum
	{
		eParseStateIdle = 0,
		eParseStateReading,
	};

    #define kBeginPacket '<'
    #define kEndPacket   '>'
    #define kBandData    'D'
    #define kBandLabels  'L'

    ScopedPointer<SerialPort>            mSerialPort;
    ScopedPointer<SerialPortInputStream> mSerialPortInput;
	int			mParseState;

    int			mNumberOfBands;
	String		mRawSerialData;
	int			mFrequencyBandData[MAX_BINS];
    String      mFrequencyBandLabels[MAX_BINS];
	MeterComp*  mFrequencyBandMeters[MAX_BINS];

	void UpdateFrequencyBandsGui(void);
	int mBandWidth;
    //==============================================================================
    TextButton* quitButton;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyBandDisplayMainWindow)
};

#endif  // __MAINCOMPONENT_H_7D44AE5B__
