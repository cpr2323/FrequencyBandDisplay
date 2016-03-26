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
    float mA, mB, mZ;};

class MeterComp : public Component
{
public:
	MeterComp(void)
		: mMeterValue(0.0),
          mShowFilteredMeterValue(true)
	{
        addAndMakeVisible(mMeterLabel);
        mMeterLabel.setJustificationType(Justification::centredTop);
        mFilteredMeterValue.Config(0.9);
	}

	void paint(Graphics& g) override
	{
	    g.fillAll (Colours::white);

		g.setColour(Colours::green);
        float meterValue = mShowFilteredMeterValue ? mFilteredMeterValue.GetFilteredValue() : mMeterValue;
		int meterHeight = (int)(getHeight() * meterValue);
		g.fillRect(0, getHeight() - meterHeight, getWidth(), meterHeight);
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
    bool              mShowFilteredMeterValue;
};

//==============================================================================
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
		addAndMakeVisible(mFHTBandComps[curBinIndex] = new MeterComp());
	}

	for (int curBinIndex = 0; curBinIndex < MAX_BINS; ++curBinIndex)
	{
		mFHTBins[curBinIndex] = 0;
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
		deleteAndZero(mFHTBandComps[curBinIndex]);
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

void MainComponent::resized()
{
	UpdateBinDisplay();
	quitButton->setBounds(getWidth() - 176, getHeight() - 60, 120, 32);
}

void MainComponent::UpdateBinDisplay(void)
{
	if (mNumberOfBands > 0)
		mBandWidth = getWidth() / mNumberOfBands;

	for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
	{
		mFHTBandComps[curBinIndex]->setVisible(true);
		mFHTBandComps[curBinIndex]->setBounds(curBinIndex * mBandWidth + 2, 10, mBandWidth - 2, getHeight() * 0.75);
	}
	for (int curBinIndex = jmin(mNumberOfBands, MAX_BINS); curBinIndex < MAX_BINS; ++curBinIndex)
		mFHTBandComps[curBinIndex]->setVisible(false);
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
									mRawFHTData = String::empty;
									mParseState = eParseStateReading;
								}
							}
							break;

							case eParseStateReading :
							{
								if (incomingData[curByteOffset] == kEndPacket)
								{
									mParseState = eParseStateIdle;
									Logger::outputDebugString("pkt: " + mRawFHTData);
                                    int startOfByteCount = mRawFHTData.indexOf("|");
                                    if (startOfByteCount == -1)
                                        break;
                                    int expectedByteCount = mRawFHTData.substring(startOfByteCount + 1).getIntValue();
                                    if (expectedByteCount != startOfByteCount + 1)
                                        break;
                                    char cmd = mRawFHTData[0];
                                    // skip over command byte
                                    mRawFHTData = mRawFHTData.substring(1);
                                    
                                    switch (cmd)
                                    {
                                        case kBandData:
                                        {
                                            // count:<comma separated bin data>
                                            // D7:126,100,5,34,79,80,120
                                            
                                            String testString(mRawFHTData.substring(0, mRawFHTData.indexOf(":")));
                                            if (testString.isEmpty() || !testString.containsOnly("0123456789"))
                                                break;
                                            
                                            int commaCount = 0;
                                            int findOffset = 0;
                                            String tempString(mRawFHTData);
                                            int newFindOffset = tempString.indexOf(findOffset, ",");
                                            while (newFindOffset != -1)
                                            {
                                                ++commaCount;
                                                findOffset = newFindOffset + 1;
                                                newFindOffset = tempString.indexOf(findOffset, ",");
                                            }
                                            int binCount = mRawFHTData.getIntValue();
                                            if (mRawFHTData.containsChar(':') && binCount == 7 && commaCount == 6)
                                            {
                                                if (binCount != mNumberOfBands)
                                                {
                                                    mNumberOfBands = binCount;
                                                    UpdateBinDisplay();
                                                    Logger::outputDebugString(String(binCount) + String("************bin count changed************"));
                                                }
                                                // skip over bin count and ':' separator
                                                mRawFHTData = mRawFHTData.trimCharactersAtStart(String("0123456789"));
                                                mRawFHTData = mRawFHTData.trimCharactersAtStart(String(":"));
                                                
                                                // read in data for each band
                                                for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
                                                {
                                                    // 126,100,5,etc
                                                    mFHTBins[curBinIndex] = mRawFHTData.getIntValue();
                                                    mRawFHTData = mRawFHTData.trimCharactersAtStart(String("0123456789."));
                                                    mRawFHTData = mRawFHTData.trimCharactersAtStart(String(","));
                                                }
                                            }
                                        }
                                        break;
                                            
                                        case kBandLabels:
                                        {
                                            // count:<comma separated quoted labels data>
                                            // L7:"1Hz","5Hz","10Hz","20Hz","40Hz","80Hz","160Hz"
                                            String testString(mRawFHTData.substring(0, mRawFHTData.indexOf(":")));
                                            if (testString.isEmpty() || !testString.containsOnly("0123456789"))
                                                break;
 
                                            int commaCount = 0;
                                            int findOffset = 0;
                                            String tempString(mRawFHTData);
                                            int newFindOffset = tempString.indexOf(findOffset, ",");
                                            while (newFindOffset != -1)
                                            {
                                                ++commaCount;
                                                findOffset = newFindOffset + 1;
                                                newFindOffset = tempString.indexOf(findOffset, ",");
                                            }
                                            int binCount = mRawFHTData.getIntValue();
                                            if (mRawFHTData.containsChar(':') && binCount == 7 && commaCount == 6)
                                            {
                                                if (binCount != mNumberOfBands)
                                                {
                                                    mNumberOfBands = binCount;
                                                    UpdateBinDisplay();
                                                    Logger::outputDebugString(String(binCount) + String(" ************bin count changed************"));
                                                }
                                                // skip over bin count and ':' separator
                                                mRawFHTData = mRawFHTData.trimCharactersAtStart(String("0123456789"));
                                                mRawFHTData = mRawFHTData.trimCharactersAtStart(String(":"));
                                                
                                                // read in label for each band
                                                for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
                                                {
                                                    // find and strip off first "
                                                    mRawFHTData = mRawFHTData.fromFirstOccurrenceOf(String("\""), false, false);
                                                    // "20Hz", "10Hz", etc
                                                    mBandLabels[curBinIndex] = mRawFHTData.upToFirstOccurrenceOf(String("\""), false, false);
                                                    // skip ending '",'
                                                    mRawFHTData = mRawFHTData.fromFirstOccurrenceOf(String("\","), false, false);
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
									mRawFHTData += String(String::charToString(incomingData[curByteOffset]));
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
				//if (mFHTBins[curBinIndex] > 19) // silly hack to hide some noise
                int fhtValue = jmax(mFHTBins[curBinIndex], 0);
                mFHTBandComps[curBinIndex]->SetMeterValue((float)fhtValue/(float)kInputMax);
				//else
					//mFHTBandComps[curBinIndex]->SetMeterLevel(0.0);
                mFHTBandComps[curBinIndex]->SetMeterLabel(mBandLabels[curBinIndex]);
			}
		}
		break;
	}
}
