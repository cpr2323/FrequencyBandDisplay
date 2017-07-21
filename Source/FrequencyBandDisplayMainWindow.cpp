#include <vector>
#include <map>
using namespace std;

#include "FrequencyBandDisplayMainWindow.h"
#include "cSerialPortListMonitor.h"
#include "MeterComp.h"
#include "SerialPortMenu.h"

#define kInputMax 1023

////////////////////////////////////////////////////
// FrequencyBandDisplayMainWindow
////////////////////////////////////////////////////
FrequencyBandDisplayMainWindow::FrequencyBandDisplayMainWindow (ApplicationProperties* applicationProperties)
    : mApplicationProperties(applicationProperties)
{
    addAndMakeVisible(mComPortLabel = new Label("ComPortLabel", "COM Port:"));
    addAndMakeVisible(mComPortName = new SerialPortMenu("ComPortName", "", mApplicationProperties));
    addAndMakeVisible(mQuitButton = new TextButton(String::empty));
    mQuitButton->setButtonText ("Quit");
    mQuitButton->addListener (this);

    startTimer(eTimerIdResizeThrottleTimer, 1);
    startTimer(eTimerIdGuiUpdateTimer, 33);
    startTimer(eTimerCheckSerialPortChange, 1000);

    mFrequencyBandDevice.SetSerialPortName(mComPortName->GetSelectedPort());
    mFrequencyBandDevice.Open();

    setSize (900, 300);
}

FrequencyBandDisplayMainWindow::~FrequencyBandDisplayMainWindow()
{
    stopTimer(eTimerCheckSerialPortChange);
    stopTimer(eTimerIdGuiUpdateTimer);
	stopTimer(eTimerIdResizeThrottleTimer);
}

void FrequencyBandDisplayMainWindow::paint (Graphics& _g)
{
    _g.fillAll (Colour (0xffc1d0ff));
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
        auto bandWidth = getWidth() / (mNumberOfBandsDisplayed + 1);

        // resize and make visible
        for (auto curBandIndex = 0; curBandIndex < mNumberOfBandsDisplayed + 1; ++curBandIndex)
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
        case eTimerIdResizeThrottleTimer :
        {
            if (mDoGuiResize)
            {
                const auto kComPortLabelWidth = 55;
                const auto kComPostTextSpacing = 2;
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
            auto numberOfBands = mFrequencyBandDevice.GetNumberOfBands();
            if (numberOfBands != mNumberOfBandsDisplayed)
            {
                // clean up old band objects
                for (auto curBandIndex = 0; curBandIndex < mNumberOfBandsDisplayed; ++curBandIndex)
                    removeChildComponent(mFrequencyBandMeters[curBandIndex]);
                mFrequencyBandMeters.clear();

                // set pup new band objects
                mNumberOfBandsDisplayed = numberOfBands;
                for (auto curBinIndex = 0; curBinIndex < mNumberOfBandsDisplayed + 1; ++curBinIndex)
                {
                    mFrequencyBandMeters.add(new MeterComp());
                    addAndMakeVisible(mFrequencyBandMeters[curBinIndex]);
                }
                mDoGuiResize = true;
            }

            if (mNumberOfBandsDisplayed != 0)
            {
                // display the current data and labels
                auto total = 0;
                for (auto curBandIndex = 0; curBandIndex < mNumberOfBandsDisplayed; ++curBandIndex)
                {
                    auto frequencyBandValue = mFrequencyBandDevice.GetBandData(curBandIndex);
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
