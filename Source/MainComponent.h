#ifndef __MAINCOMPONENT_H_7D44AE5B__
#define __MAINCOMPONENT_H_7D44AE5B__

#include "../JuceLibraryCode/JuceHeader.h"
class MeterComp;

#define MAX_BINS 128

class MainComponent  : public Component,
                       public ButtonListener,
					   public MultiTimer
{
public:
    //==============================================================================
    MainComponent ();
    ~MainComponent();

    //==============================================================================

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

	enum
	{
		eTimerId1ms = 0,
		eTimerId20ms
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

	String		mRawFHTData;
	int			mNumberOfBands ;
	int			mFHTBins[MAX_BINS];
    String      mBandLabels[MAX_BINS];
	MeterComp*  mFHTBandComps[MAX_BINS];

	void UpdateBinDisplay(void);
	int mBandWidth;
    //==============================================================================
    TextButton* quitButton;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

#endif  // __MAINCOMPONENT_H_7D44AE5B__
