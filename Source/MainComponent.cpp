#include "MainComponent.h"

#define kBPS 115200
#define kInputMax 1023

#ifdef JUCE_WIN
#define kSerialPortName "\\\\.\\COM3"
#endif

#ifdef JUCE_MAC
#define kSerialPortName "/dev/cu.usbmodem1421"
#endif

/////////////////////////////////////////////////////////////////////////////////
// Data format
//  Labels
//    <Lc:l1,l2,lc>
//    <L4:"10Hz","20Hz","30Hz","40Hz">
//
//    <Dc:b1,b2,bc>
//    <D4:1.2,3.5,1.34,5.63>
//
/////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// cSinglePoleFilter
class cSinglePoleFilter
{
public:
    cSinglePoleFilter(void)
    : mA(1.0),
      mB(1.0f - mA),
      mZ(0.0f)
    {
        
    }
    
    cSinglePoleFilter(float a)
     : mA(a),
       mB(1.0f - mA),
       mZ(0.0f)
    {
    }

    float DoFilter(float data)
    {
        mZ = (data * mB) + (mZ * mA);
        return mZ;
    }
    
    void Config(float a)
    {
        mA = a;
        mB = 1.0f - mA;
        mZ = 0.0f;
    }
    
    void SetCurValue(float _z)
    {
        mZ =_z;
    }
    
    float GetFilteredValue(void)
    {
        return mZ;
    }
    
private:
    float mA, mB, mZ;
};

class cPeakWithHold
{
public:
    cPeakWithHold(int64 _holdTime = 1000)
    : mPeakValue(0.0),
      mHoldTime(_holdTime),
      mHoldTimeStart(Time::currentTimeMillis() - mHoldTime)
    {
        mFilteredPeakValue.Config(0.99);
    }
    
    void SetValue(float _newValue)
    {

        if (_newValue > mPeakValue)
        {
            // store the new peak value
            mPeakValue = _newValue;
            mFilteredPeakValue.SetCurValue(_newValue);

            // reset the timer
            mHoldTimeStart = Time::currentTimeMillis();
        }
        else
        {
            // run the current value through the filtered value
            if ((Time::currentTimeMillis() - mHoldTimeStart) >= mHoldTime)
            {
                mPeakValue = _newValue;
                mFilteredPeakValue.DoFilter(_newValue);
            }
        }
    }
    
    float GetValue(void)
    {
        float currentPeakHoldValue;
        if ((Time::currentTimeMillis() - mHoldTimeStart) >= mHoldTime)
        {
            currentPeakHoldValue = jmax(mFilteredPeakValue.GetFilteredValue(), mPeakValue);
        }
        else
        {
            currentPeakHoldValue = mPeakValue;
        }
        
        return currentPeakHoldValue;
    }
private:
    float             mPeakValue;
    cSinglePoleFilter mFilteredPeakValue;
    int               mHoldTime;
    int64             mHoldTimeStart;
};

///////////////////////////////////////////////////////
// MeterComp
class MeterComp : public Component
{
public:
	MeterComp(void)
		: mMeterValue(0.0),
          mShowPeak(true),
          mShowFilteredMeterValue(true)
	{
        addAndMakeVisible(mMeterLabel);
        mMeterLabel.setJustificationType(Justification::centredTop);
        mFilteredMeterValue.Config(0.9);
	}

	void paint(Graphics& g) override
	{
        // drae background
	    g.fillAll (Colours::grey);

        // drae main mater
		g.setColour(Colours::green);
        float meterValue = mShowFilteredMeterValue ? mFilteredMeterValue.GetFilteredValue() : mMeterValue;
		int meterHeight = (int)(getHeight() * meterValue);
		g.fillRect(0, getHeight() - meterHeight, getWidth(), meterHeight);
        if (mShowPeak && mPeakValue.GetValue() > 0.0)
        {
            g.setColour(Colours::gold);
            int peakHeight = (int)(getHeight() * mPeakValue.GetValue());
            g.fillRect(0, (getHeight() - peakHeight) - 2, getWidth(), 5);
        }
	}

    void resized(void) override
    {
        mMeterLabel.setBounds(1, 1, getWidth() - 2, 20);
    }
    
	~MeterComp()
	{
	}

	void SetMeterValue(float meterValue)
	{
		mMeterValue = meterValue;
        
        if (meterValue > mFilteredMeterValue.GetFilteredValue())
            mFilteredMeterValue.SetCurValue(meterValue);
        else
            mFilteredMeterValue.DoFilter(meterValue);
        
        //mPeakValue.SetValue(mFilteredMeterValue.GetFilteredValue());
        mPeakValue.SetValue(meterValue);
        
        repaint();
	}

    void SetMeterLabel(String meterLabel)
    {
        mMeterLabel.setText(meterLabel, NotificationType::sendNotification);
        repaint();
    }

    void SetShoWFilteredMeterValue(bool _showFilteredMeterValue)
    {
        mShowFilteredMeterValue = _showFilteredMeterValue;
    }
        
private:
    float             mMeterValue;
    cSinglePoleFilter mFilteredMeterValue;
    Label             mMeterLabel;
        
    cPeakWithHold     mPeakValue;
    bool              mShowPeak;
    bool              mShowFilteredMeterValue;
};

///////////////////////////////////////////////////////
// MainComponent
MainComponent::MainComponent ()
    : mNumberOfBands(0),
	  mBandWidth(0),
      quitButton (0)
{
    addAndMakeVisible (quitButton = new TextButton (String::empty));
    quitButton->setButtonText ("Quit");
    quitButton->addListener (this);

	mParseState = eParseStateIdle;

    OpenSerialPort();

	for (int curBinIndex = 0; curBinIndex < MAX_BINS; ++curBinIndex)
	{
		addAndMakeVisible(mFrequencyBandMeters[curBinIndex] = new MeterComp());
	}

	for (int curBinIndex = 0; curBinIndex < MAX_BINS; ++curBinIndex)
	{
		mFrequencyBandData[curBinIndex] = 0;
	}

	startTimer(eTimerId1ms, 1);
	startTimer(eTimerId20ms, 20);

    setSize (900, 300);

}

MainComponent::~MainComponent()
{
	stopTimer(eTimerId20ms);
	stopTimer(eTimerId1ms);

    CloseSerialPort();
    
	for (int curBinIndex = 0; curBinIndex < MAX_BINS; ++curBinIndex)
	{
		deleteAndZero(mFrequencyBandMeters[curBinIndex]);
	}
    deleteAndZero (quitButton);

}

void MainComponent::OpenSerialPort(void)
{
    StringPairArray serialPorts = SerialPort::getSerialPortPaths();
    for (int i = 0; i < serialPorts.size(); ++i)
    {
        String deviceName = serialPorts.getAllValues()[i];

        Logger::outputDebugString(deviceName);
    }

    mSerialPort = new SerialPort();
    String usbPortName(kSerialPortName);
    bool opened = mSerialPort->open(usbPortName);

    if (opened)
    {
        SerialPortConfig serialPortConfig;
        mSerialPort->getConfig(serialPortConfig);
        serialPortConfig.bps      = kBPS;
        serialPortConfig.databits = 8;
        serialPortConfig.parity   = SerialPortConfig::SERIALPORT_PARITY_NONE;
        serialPortConfig.stopbits = SerialPortConfig::STOPBITS_1;
        mSerialPort->setConfig(serialPortConfig);
    
        mSerialPortInput = new SerialPortInputStream(mSerialPort);
    }
    else
    {
        // report error
    }
}

void MainComponent::CloseSerialPort(void)
{
    mSerialPortInput = nullptr;
    if (mSerialPort != nullptr)
    {
        mSerialPort->close();
        mSerialPort = nullptr;
    }
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    g.fillAll (Colour (0xffc1d0ff));
}

const int kQuitButtonWidth = 60;
const int kQuitButtonHeight = 20;
void MainComponent::resized()
{
	UpdateBinDisplay();
	quitButton->setBounds(getWidth() - kQuitButtonWidth - 2, getHeight() - kQuitButtonHeight - 2, kQuitButtonWidth, kQuitButtonHeight);
}

void MainComponent::UpdateBinDisplay(void)
{
	if (mNumberOfBands > 0)
		mBandWidth = getWidth() / mNumberOfBands;

	for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
	{
		mFrequencyBandMeters[curBinIndex]->setVisible(true);
		mFrequencyBandMeters[curBinIndex]->setBounds(curBinIndex * mBandWidth + 2, 10, mBandWidth - 2, getHeight() * 0.75);
	}
	for (int curBinIndex = jmin(mNumberOfBands, MAX_BINS); curBinIndex < MAX_BINS; ++curBinIndex)
		mFrequencyBandMeters[curBinIndex]->setVisible(false);
}

void MainComponent::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == quitButton)
    {
        JUCEApplication::quit();
    }
}

#define kSerialPortBufferLen 256
void MainComponent::timerCallback(int timerId)
{
	switch(timerId)
	{
		case eTimerId1ms :
		{
            if (mSerialPortInput == nullptr)
                return;

            int  maxBytesPerTimerCallback = kSerialPortBufferLen * 2;
			int  totalBytesRead			  = 0;
			int  bytesRead				  = 0;
			int  curByteOffset			  = 0;
			int  dataLength				  = kSerialPortBufferLen;
			char incomingData[kSerialPortBufferLen] = "";

			while((!mSerialPortInput->isExhausted()) && totalBytesRead < maxBytesPerTimerCallback)
			{
				bytesRead = mSerialPortInput->read(incomingData, dataLength);
				if (bytesRead < 1)
					break;
				else
				{
					curByteOffset = totalBytesRead;
					while (curByteOffset < totalBytesRead + bytesRead)
					{
						switch (mParseState)
						{
							case eParseStateIdle :
							{
								if (incomingData[curByteOffset] == kBeginPacket)
								{
									mRawSerialData = String::empty;
									mParseState = eParseStateReading;
								}
							}
							break;

							case eParseStateReading :
							{
								if (incomingData[curByteOffset] == kEndPacket)
								{
									mParseState = eParseStateIdle;
									Logger::outputDebugString("pkt: " + mRawSerialData);
                                    int startOfByteCount = mRawSerialData.indexOf("|");
                                    if (startOfByteCount == -1)
                                        break;
                                    int expectedByteCount = mRawSerialData.substring(startOfByteCount + 1).getIntValue();
                                    if (expectedByteCount != startOfByteCount + 1)
                                        break;
                                    char cmd = mRawSerialData[0];
                                    // skip over command byte
                                    mRawSerialData = mRawSerialData.substring(1);
                                    
                                    switch (cmd)
                                    {
                                        case kBandData:
                                        {
                                            // count:<comma separated bin data>
                                            // D7:126,100,5,34,79,80,120
                                            
                                            String testString(mRawSerialData.substring(0, mRawSerialData.indexOf(":")));
                                            if (testString.isEmpty() || !testString.containsOnly("0123456789"))
                                                break;
                                            
                                            int commaCount = 0;
                                            int findOffset = 0;
                                            String tempString(mRawSerialData);
                                            int newFindOffset = tempString.indexOf(findOffset, ",");
                                            while (newFindOffset != -1)
                                            {
                                                ++commaCount;
                                                findOffset = newFindOffset + 1;
                                                newFindOffset = tempString.indexOf(findOffset, ",");
                                            }
                                            int binCount = mRawSerialData.getIntValue();
                                            if (mRawSerialData.containsChar(':') && binCount == 7 && commaCount == 6)
                                            {
                                                if (binCount != mNumberOfBands)
                                                {
                                                    mNumberOfBands = binCount;
                                                    UpdateBinDisplay();
                                                    Logger::outputDebugString(String(binCount) + String("************bin count changed************"));
                                                }
                                                // skip over bin count and ':' separator
                                                mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789"));
                                                mRawSerialData = mRawSerialData.trimCharactersAtStart(String(":"));
                                                
                                                // read in data for each band
                                                for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
                                                {
                                                    // 126,100,5,etc
                                                    mFrequencyBandData[curBinIndex] = mRawSerialData.getIntValue();
                                                    mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789."));
                                                    mRawSerialData = mRawSerialData.trimCharactersAtStart(String(","));
                                                }
                                            }
                                        }
                                        break;
                                            
                                        case kBandLabels:
                                        {
                                            // count:<comma separated quoted labels data>
                                            // L7:"1Hz","5Hz","10Hz","20Hz","40Hz","80Hz","160Hz"
                                            String testString(mRawSerialData.substring(0, mRawSerialData.indexOf(":")));
                                            if (testString.isEmpty() || !testString.containsOnly("0123456789"))
                                                break;
 
                                            int commaCount = 0;
                                            int findOffset = 0;
                                            String tempString(mRawSerialData);
                                            int newFindOffset = tempString.indexOf(findOffset, ",");
                                            while (newFindOffset != -1)
                                            {
                                                ++commaCount;
                                                findOffset = newFindOffset + 1;
                                                newFindOffset = tempString.indexOf(findOffset, ",");
                                            }
                                            int binCount = mRawSerialData.getIntValue();
                                            if (mRawSerialData.containsChar(':') && binCount == 7 && commaCount == 6)
                                            {
                                                if (binCount != mNumberOfBands)
                                                {
                                                    mNumberOfBands = binCount;
                                                    UpdateBinDisplay();
                                                    Logger::outputDebugString(String(binCount) + String(" ************bin count changed************"));
                                                }
                                                // skip over bin count and ':' separator
                                                mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789"));
                                                mRawSerialData = mRawSerialData.trimCharactersAtStart(String(":"));
                                                
                                                // read in label for each band
                                                for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
                                                {
                                                    // find and strip off first "
                                                    mRawSerialData = mRawSerialData.fromFirstOccurrenceOf(String("\""), false, false);
                                                    // "20Hz", "10Hz", etc
                                                    mFrequencyBandLabels[curBinIndex] = mRawSerialData.upToFirstOccurrenceOf(String("\""), false, false);
                                                    // skip ending '",'
                                                    mRawSerialData = mRawSerialData.fromFirstOccurrenceOf(String("\","), false, false);
                                                }
                                            }
                                        }
                                        break;
                                            
                                        default:
                                        {
                                            // error - unknown cmd
                                        }
                                        break;
                                    }
								}
								else
								{
									mRawSerialData += String(String::charToString(incomingData[curByteOffset]));
								}
							}
							break;
						}

						++curByteOffset;
					}
					totalBytesRead += bytesRead;
				}
			}
		}
		break;

		case eTimerId20ms :
		{
			for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
			{
                int frequencyBandValue = jmax(mFrequencyBandData[curBinIndex], 0);
                mFrequencyBandMeters[curBinIndex]->SetMeterValue((float)frequencyBandValue/(float)kInputMax);
                mFrequencyBandMeters[curBinIndex]->SetMeterLabel(mFrequencyBandLabels[curBinIndex]);
			}
		}
		break;
	}
}
