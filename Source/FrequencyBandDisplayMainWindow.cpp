#include <vector>
#include <map>
using namespace std;

#include "FrequencyBandDisplayMainWindow.h"

#define kBPS 115200
#define kInputMax 1023

#ifdef JUCE_WINDOWS
#define kSerialPortName "\\\\.\\COM4"
#endif

#ifdef JUCE_MAC
#define kSerialPortName "/dev/cu.usbmodem1421"
#endif

/////////////////////////////////////////////////////////////////////////////////////
#define kSerialPortListMonitorSleepTime 1000
class cSerialPortListMonitor : public Thread
{
public:
    cSerialPortListMonitor(void);
    ~cSerialPortListMonitor(void);
    bool HasListChanged(void);
    StringPairArray GetSerialPortList(void);
    void SetSleepTime(int sleepTime);
    void run() override;

private:
    CriticalSection mDataLock;
    StringPairArray mSerialPortNames;
    int             mSleepTime;
    bool            mListChanged;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cSerialPortListMonitor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cSerialPortListMonitor::cSerialPortListMonitor(void)
    : Thread(String("Serial Port Monitor")),
    mSleepTime(kSerialPortListMonitorSleepTime),
    mListChanged(false)
{
    startThread();
}

cSerialPortListMonitor::~cSerialPortListMonitor(void)
{
    stopThread(5000);
}

StringPairArray cSerialPortListMonitor::GetSerialPortList(void)
{
    ScopedLock dataLock(mDataLock);
    mListChanged = false;
    return mSerialPortNames;
}

void cSerialPortListMonitor::SetSleepTime(int sleepTime)
{
    mSleepTime = sleepTime;
}

bool cSerialPortListMonitor::HasListChanged(void)
{
    return mListChanged;
}

// thread for keeping an up to date list of serial port names
void cSerialPortListMonitor::run()
{
    while (!threadShouldExit())
    {
        // wake up every mSleepTime to check the serial port list
        sleep(mSleepTime);

        StringPairArray serialPortList(SerialPort::getSerialPortPaths());
        ScopedLock dataLock(mDataLock);
        if ((serialPortList.size() != mSerialPortNames.size()) || serialPortList != mSerialPortNames)
        {
            mSerialPortNames = serialPortList;
            mListChanged = true;
        }
    }
}

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

///////////////////////////////////////////////////////
// cPeakWithHold
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
        setOpaque(true);
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
//
class SerialPortMenu : public Label
{
public:
    SerialPortMenu(String _componentName, String _label, ApplicationProperties* applicationProperties)
        : Label(_componentName, _label),
          mApplicationProperties(applicationProperties)
    {
        SetSelectedPort(mApplicationProperties->getUserSettings()->getValue("UsbPortName"));
        mSerialPortListMonitorThread = new cSerialPortListMonitor;
    }

    void SetSelectedPort(String deviceName)
    {
        mApplicationProperties->getUserSettings()->setValue("UsbPortName", deviceName);
        mSelectedPort = deviceName;
        if (deviceName.length() != 0)
            setText(mSelectedPort, NotificationType::dontSendNotification);
        else
            setText("<none>", NotificationType::dontSendNotification);
    }

    String GetSelectedPort(void )
    {
        return mSelectedPort;
    }

    void mouseEnter(const MouseEvent &event) override
    {
        setColour(textColourId, Colours::green);
    }

    void mouseExit(const MouseEvent &event) override
    {
        setColour(textColourId, Colours::black);
    }

    void mouseDown(const MouseEvent &event) override
    {
        PopupMenu menu;

        StringPairArray serialPortList = mSerialPortListMonitorThread->GetSerialPortList();
        if (serialPortList.size() == 0)
        {
            menu.addItem(eUsbPortSelectNoneAvail, "<none>");
        }
        else
        {
            bool selectedPortFound;
            for (unsigned int curSerialPortListIndex = 0; curSerialPortListIndex < serialPortList.size(); ++curSerialPortListIndex)
            {

                menu.addItem(curSerialPortListIndex + 1, serialPortList.getAllKeys()[curSerialPortListIndex], true, serialPortList.getAllValues()[curSerialPortListIndex] == mSelectedPort);
            }
        }

        int serialPortSelected = menu.show();

        if (serialPortSelected > 0)
            if (serialPortList.getAllValues()[serialPortSelected - 1] != mSelectedPort)
                SetSelectedPort(serialPortList.getAllValues()[serialPortSelected - 1]);
            else
                SetSelectedPort("");
    }

private:
    ScopedPointer<cSerialPortListMonitor> mSerialPortListMonitorThread;
    String                                mSelectedPort;
    ApplicationProperties*                mApplicationProperties;
};


///////////////////////////////////////////////////////
// FrequencyBandDisplayMainWindow
FrequencyBandDisplayMainWindow::FrequencyBandDisplayMainWindow (ApplicationProperties* applicationProperties)
    : Thread(String("FrequencyBandDisplayMainWindow")),
      mNumberOfBands(0),
      mQuitButton (0),
      mDoGuiResize(false),
      mApplicationProperties(applicationProperties)
{
    addAndMakeVisible(mComPortLabel = new Label("ComPortLabel", "COM Port:"));
    addAndMakeVisible(mComPortName = new SerialPortMenu("ComPortName", "", mApplicationProperties));
    addAndMakeVisible (mQuitButton = new TextButton (String::empty));
    mQuitButton->setButtonText ("Quit");
    mQuitButton->addListener (this);

	mParseState = eParseStateIdle;

	for (int curBinIndex = 0; curBinIndex < MAX_BINS; ++curBinIndex)
	{
		addAndMakeVisible(mFrequencyBandMeters[curBinIndex] = new MeterComp());
	}

	for (int curBinIndex = 0; curBinIndex < MAX_BINS; ++curBinIndex)
	{
		mFrequencyBandData[curBinIndex] = 0;
	}

    // start the serial thread reading data
    startThread();
    
    startTimer(eTimerIdFastTimer, 1);
    
    // start the GUI update timeer for updating the GUI
    startTimer(eTimerIdGuiUpdateTimer, 33);

    // start the serial port open timer
    startTimer(eTimerOpenSerialPort, 1000);

    setSize (900, 300);

}

FrequencyBandDisplayMainWindow::~FrequencyBandDisplayMainWindow()
{
    stopThread(500);
	stopTimer(eTimerIdGuiUpdateTimer);
	stopTimer(eTimerIdFastTimer);
    
    CloseSerialPort();
    
	for (int curBinIndex = 0; curBinIndex < MAX_BINS; ++curBinIndex)
	{
		deleteAndZero(mFrequencyBandMeters[curBinIndex]);
	}
    deleteAndZero (mQuitButton);
    deleteAndZero (mComPortLabel);
    deleteAndZero(mComPortName);

}

void FrequencyBandDisplayMainWindow::OpenSerialPort(void)
{
    String serialPortName = mComPortName->GetSelectedPort();

    mSerialPort = new SerialPort();
    bool opened = mSerialPort->open(serialPortName);

    if (opened)
    {
        SerialPortConfig serialPortConfig;
        mSerialPort->getConfig(serialPortConfig);
        serialPortConfig.bps = kBPS;
        serialPortConfig.databits = 8;
        serialPortConfig.parity = SerialPortConfig::SERIALPORT_PARITY_NONE;
        serialPortConfig.stopbits = SerialPortConfig::STOPBITS_1;
        mSerialPort->setConfig(serialPortConfig);

        mSerialPortInput = new SerialPortInputStream(mSerialPort);
        mCurrentSerialPortName = serialPortName;
    }
    else
    {
        // report error
        mComPortName->setText("", NotificationType::dontSendNotification);
        Logger::outputDebugString(String("Unable to open serial port:") + serialPortName);
    }
}

void FrequencyBandDisplayMainWindow::CloseSerialPort(void)
{
    mSerialPortInput = nullptr;
    if (mSerialPort != nullptr)
    {
        mSerialPort->close();
        mSerialPort = nullptr;
    }
}

void FrequencyBandDisplayMainWindow::paint (Graphics& g)
{
    g.fillAll (Colour (0xffc1d0ff));
}

const int kQuitButtonWidth = 60;
const int kQuitButtonHeight = 20;
void FrequencyBandDisplayMainWindow::resized()
{
    mDoGuiResize = true;;
}

void FrequencyBandDisplayMainWindow::UpdateFrequencyBandsGui(void)
{
	int bandWidth = getWidth() / (mNumberOfBands + 1);

    // resize and make visible
	for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands + 1, MAX_BINS); ++curBinIndex)
	{
		mFrequencyBandMeters[curBinIndex]->setVisible(true);
		mFrequencyBandMeters[curBinIndex]->setBounds(curBinIndex * bandWidth + 2, 2, bandWidth - 2, mQuitButton->getY() - 4);
	}
    // hide unneeded meters
	for (int curBinIndex = jmin(mNumberOfBands + 1, MAX_BINS); curBinIndex < MAX_BINS; ++curBinIndex)
		mFrequencyBandMeters[curBinIndex]->setVisible(false);
}

void FrequencyBandDisplayMainWindow::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mQuitButton)
    {
        JUCEApplication::quit();
    }
}

#define kSerialPortBufferLen 256
void FrequencyBandDisplayMainWindow::run()
{
    while (!threadShouldExit())
    {
        // handle reading from the serial port
        if (!threadShouldExit() && (mSerialPortInput != nullptr) && (!mSerialPortInput->isExhausted()))
        {
            int  bytesRead				  = 0;
            int  curByteOffset			  = 0;
            char incomingData[kSerialPortBufferLen] = "";
            
            bytesRead = mSerialPortInput->read(incomingData, kSerialPortBufferLen);
            if (bytesRead < 1)
            {
                wait(1);
                continue;
            }
            else
            {
                // parse incoming data
                while (curByteOffset < bytesRead)
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
                                        int binCount = mRawSerialData.getIntValue();
                                        if (binCount > 0)
                                        {
                                            if (binCount != mNumberOfBands)
                                            {
                                                mNumberOfBands = binCount;
                                                mDoGuiResize = true;
                                                Logger::outputDebugString(String(binCount) + String("************bin count changed************"));
                                            }
                                            // skip over bin count and ':' separator
                                            mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789"));
                                            mRawSerialData = mRawSerialData.trimCharactersAtStart(String(":"));
                                            
                                            // read in data for each band
                                            {
                                                const ScopedLock scopedLock(mFrequencyBandDataLock);
                                                for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
                                                {
                                                    // 126,100,5,etc
                                                    mFrequencyBandData[curBinIndex] = mRawSerialData.getIntValue();
                                                    mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789."));
                                                    mRawSerialData = mRawSerialData.trimCharactersAtStart(String(","));
                                                }
                                            }
                                        }
                                    }
                                    break;
                                        
                                    case kBandLabels:
                                    {
                                        // count:<comma separated quoted labels data>
                                        // L7:"1Hz","5Hz","10Hz","20Hz","40Hz","80Hz","160Hz"
                                        int binCount = mRawSerialData.getIntValue();
                                        if (binCount > 0)
                                        {
                                            if (binCount != mNumberOfBands)
                                            {
                                                mNumberOfBands = binCount;
                                                mDoGuiResize = true;
                                                Logger::outputDebugString(String(binCount) + String(" ************bin count changed************"));
                                            }
                                            // skip over bin count and ':' separator
                                            mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789"));
                                            mRawSerialData = mRawSerialData.trimCharactersAtStart(String(":"));
                                            
                                            // read in label for each band
                                            {
                                                const ScopedLock scopedLock(mFrequencyBandLabelLock);
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
            }
        }
        else
        {
            wait(1);
        }
    }
}


void FrequencyBandDisplayMainWindow::timerCallback(int timerId)
{
	switch(timerId)
	{
        case eTimerIdFastTimer :
        {
            if (mDoGuiResize)
            {
                const int kComPortLabelWidth = 55;
                const int kComPostTextSpacing = 2;
                mComPortLabel->setBounds(kComPostTextSpacing, getHeight() - 20 - kComPostTextSpacing, kComPortLabelWidth, 20);
                mComPortName->setBounds(kComPortLabelWidth, getHeight() - 20 - kComPostTextSpacing, 100, 20);
                mQuitButton->setBounds(getWidth() - kQuitButtonWidth - 2, getHeight() - kQuitButtonHeight - 2, kQuitButtonWidth, kQuitButtonHeight);
                UpdateFrequencyBandsGui();
                mDoGuiResize = false;
            }
        }
        break;
            
        case eTimerOpenSerialPort :
        {
            String serialPortName = mComPortName->GetSelectedPort();

            if (serialPortName.length() != 0)
            {
                if ((mSerialPort == nullptr) || mCurrentSerialPortName != serialPortName)
                    OpenSerialPort();
            }
            else if (mSerialPort != nullptr)
            {
                CloseSerialPort();
            }

        }
        break;

		case eTimerIdGuiUpdateTimer:
		{
            int total = 0;
			for (int curBinIndex = 0; curBinIndex < jmin(mNumberOfBands, MAX_BINS); ++curBinIndex)
			{
                int frequencyBandValue = 0;
                {
                    const ScopedLock scopedLock(mFrequencyBandDataLock);
                    frequencyBandValue = jmax(mFrequencyBandData[curBinIndex], 0);
                }
                mFrequencyBandMeters[curBinIndex]->SetMeterValue((float)frequencyBandValue/(float)kInputMax);
                total += frequencyBandValue;
                //total += (frequencyBandValue / (mNumberOfBands / 2));

                {
                    const ScopedLock scopedLock(mFrequencyBandLabelLock);
                    mFrequencyBandMeters[curBinIndex]->SetMeterLabel(mFrequencyBandLabels[curBinIndex]);
                }
			}

            if (mNumberOfBands > 0)
                total /= mNumberOfBands;

            mFrequencyBandMeters[mNumberOfBands]->SetMeterValue((float)total/(float)kInputMax);
            mFrequencyBandMeters[mNumberOfBands]->SetMeterLabel("Total");
		}
		break;
	}
}
