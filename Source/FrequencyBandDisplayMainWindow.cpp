#include <vector>
#include <map>
using namespace std;

#include "FrequencyBandDisplayMainWindow.h"

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
                menu.addItem(curSerialPortListIndex + 1, serialPortList.getAllKeys()[curSerialPortListIndex], true, serialPortList.getAllValues()[curSerialPortListIndex] == mSelectedPort);
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
    : mNumberOfBandsDisplayed(0),
      mDoGuiResize(false),
      mApplicationProperties(applicationProperties)
{
    addAndMakeVisible(mComPortLabel = new Label("ComPortLabel", "COM Port:"));
    addAndMakeVisible(mComPortName = new SerialPortMenu("ComPortName", "", mApplicationProperties));
    addAndMakeVisible (mQuitButton = new TextButton (String::empty));
    mQuitButton->setButtonText ("Quit");
    mQuitButton->addListener (this);

    startTimer(eTimerIdFastTimer, 1);
    startTimer(eTimerIdGuiUpdateTimer, 33);
    startTimer(eTimerCheckSerialPortChange, 1000);

    mFrequencyBandDevice.SetSerialPortName(mComPortName->GetSelectedPort());
    mFrequencyBandDevice.Open();

    setSize (900, 300);
}

FrequencyBandDisplayMainWindow::~FrequencyBandDisplayMainWindow()
{
	stopTimer(eTimerIdGuiUpdateTimer);
	stopTimer(eTimerIdFastTimer);
}

void FrequencyBandDisplayMainWindow::paint (Graphics& g)
{
    g.fillAll (Colour (0xffc1d0ff));
}

void FrequencyBandDisplayMainWindow::resized()
{
    mDoGuiResize = true;
}

const int kQuitButtonWidth = 60;
const int kQuitButtonHeight = 20;
void FrequencyBandDisplayMainWindow::UpdateFrequencyBandsGui(void)
{
    if (mNumberOfBandsDisplayed > 0)
    {
        int bandWidth = getWidth() / (mNumberOfBandsDisplayed + 1);

        // resize and make visible
        for (int curBandIndex = 0; curBandIndex < mNumberOfBandsDisplayed + 1; ++curBandIndex)
        {
            mFrequencyBandMeters[curBandIndex]->setVisible(true);
            mFrequencyBandMeters[curBandIndex]->setBounds(curBandIndex * bandWidth + 2, 2, bandWidth - 2, mQuitButton->getY() - 4);
        }
    }
}

void FrequencyBandDisplayMainWindow::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mQuitButton)
        JUCEApplication::quit();
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
            
		case eTimerIdGuiUpdateTimer:
		{
            // check if number of bands has changed
            int numberOfBands = mFrequencyBandDevice.GetNumberOfBands();
            if (numberOfBands != mNumberOfBandsDisplayed)
            {
                // clean up old band objects
                for (int curBandIndex = 0; curBandIndex < mNumberOfBandsDisplayed; ++curBandIndex)
                    removeChildComponent(mFrequencyBandMeters[curBandIndex]);
                mFrequencyBandMeters.clear();

                // set pup new band objects
                mNumberOfBandsDisplayed = numberOfBands;
                for (int curBinIndex = 0; curBinIndex < mNumberOfBandsDisplayed + 1; ++curBinIndex)
                {
                    mFrequencyBandMeters.add(new MeterComp());
                    addAndMakeVisible(mFrequencyBandMeters[curBinIndex]);
                }
                mDoGuiResize = true;
            }

            if (mNumberOfBandsDisplayed != 0)
            {
                // display the current data and labels
                int total = 0;
                for (int curBandIndex = 0; curBandIndex < mNumberOfBandsDisplayed; ++curBandIndex)
                {
                    int frequencyBandValue = mFrequencyBandDevice.GetBandData(curBandIndex);
                    mFrequencyBandMeters[curBandIndex]->SetMeterValue((float)frequencyBandValue / (float)kInputMax);
                    total += frequencyBandValue;
                    //total += (frequencyBandValue / (mNumberOfBands / 2));

                    mFrequencyBandMeters[curBandIndex]->SetMeterLabel(mFrequencyBandDevice.GetBandLabel(curBandIndex));
                }

                if (mNumberOfBandsDisplayed > 0)
                    total /= mNumberOfBandsDisplayed;

                mFrequencyBandMeters[mNumberOfBandsDisplayed]->SetMeterValue((float)total / (float)kInputMax);
                mFrequencyBandMeters[mNumberOfBandsDisplayed]->SetMeterLabel("Total");
            }
		}
		break;

        case eTimerCheckSerialPortChange:
        {
            String selectedSerialPortName = mComPortName->GetSelectedPort();
            if (selectedSerialPortName != mFrequencyBandDevice.GetSerialPortName())
            {
                mFrequencyBandDevice.SetSerialPortName(selectedSerialPortName);

                if (selectedSerialPortName.length() == 0)
                    mFrequencyBandDevice.Close();
                else
                    mFrequencyBandDevice.Open();
            }
        }
        break;

	}
}
