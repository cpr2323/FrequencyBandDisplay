#include <vector>
#include <map>
using namespace std;

#include "FrequencyBandDisplayMainWindow.h"
#include "cSerialPortListMonitor.h"
#include "MeterComp.h"
#include "SerialPortMenu.h"
#include <memory>

#define kInputMax 1023

////////////////////////////////////////////////////
// FrequencyBandDisplayMainWindow
////////////////////////////////////////////////////
FrequencyBandDisplayMainWindow::FrequencyBandDisplayMainWindow (ApplicationProperties* applicationProperties)
    : mApplicationProperties(applicationProperties)
{
    comPortLabel.setText ("COM Port:", NotificationType::dontSendNotification);
    addAndMakeVisible(comPortLabel);

    comPortName = std::make_unique<SerialPortMenu> ("ComPortName", "", mApplicationProperties);
    addAndMakeVisible(comPortName.get());

    quitButton.setButtonText ("Quit");
    quitButton.onClick = [this]() { JUCEApplication::quit (); };
    addAndMakeVisible(quitButton);

    startTimer(eTimerIdResizeThrottleTimer, 1);
    startTimer(eTimerIdGuiUpdateTimer, 33);
    startTimer(eTimerCheckSerialPortChange, 1000);

    frequencyBandDevice.SetSerialPortName(comPortName->GetSelectedPort());
    frequencyBandDevice.Open();

    setSize (900, 300);
}

FrequencyBandDisplayMainWindow::~FrequencyBandDisplayMainWindow()
{
    stopTimer(eTimerCheckSerialPortChange);
    stopTimer(eTimerIdGuiUpdateTimer);
	stopTimer(eTimerIdResizeThrottleTimer);
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
    if (numberOfBandsDisplayed > 0)
    {
        auto bandWidth = getWidth() / (numberOfBandsDisplayed + 1);

        // resize and make visible
        for (auto curBandIndex = 0; curBandIndex < numberOfBandsDisplayed + 1; ++curBandIndex)
        {
            frequencyBandMeters[curBandIndex]->setVisible(true);
            frequencyBandMeters[curBandIndex]->setBounds(curBandIndex * bandWidth + 2, 2, bandWidth - 2, quitButton.getY() - 4);
        }
    }
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
                comPortLabel.setBounds(kComPostTextSpacing, getHeight() - 20 - kComPostTextSpacing, kComPortLabelWidth, 20);
                comPortName->setBounds(kComPortLabelWidth, getHeight() - 20 - kComPostTextSpacing, 100, 20);
                quitButton.setBounds(getWidth() - kQuitButtonWidth - 2, getHeight() - kQuitButtonHeight - 2, kQuitButtonWidth, kQuitButtonHeight);
                UpdateFrequencyBandsGui();
                mDoGuiResize = false;
            }
        }
        break;
            
		case eTimerIdGuiUpdateTimer:
		{
            // check if number of bands has changed
            const auto numberOfBands = frequencyBandDevice.GetNumberOfBands();
            if (numberOfBands != numberOfBandsDisplayed)
            {
                // clean up old band objects
                for (auto curBandIndex = 0; curBandIndex < numberOfBandsDisplayed; ++curBandIndex)
                    removeChildComponent(frequencyBandMeters[curBandIndex]);
                frequencyBandMeters.clear();

                // set pup new band objects
                numberOfBandsDisplayed = numberOfBands;
                for (auto curBinIndex = 0; curBinIndex < numberOfBandsDisplayed + 1; ++curBinIndex)
                {
                    frequencyBandMeters.add(new MeterComp());
                    addAndMakeVisible(frequencyBandMeters[curBinIndex]);
                }
                mDoGuiResize = true;
            }

            if (numberOfBandsDisplayed != 0)
            {
                // display the current data and labels
                auto total = 0;
                for (auto curBandIndex = 0; curBandIndex < numberOfBandsDisplayed; ++curBandIndex)
                {
                    auto frequencyBandValue = frequencyBandDevice.GetBandData((uint16_t)curBandIndex);
                    frequencyBandMeters[curBandIndex]->SetMeterValue((float)frequencyBandValue / (float)kInputMax);
                    total += frequencyBandValue;
                    //total += (frequencyBandValue / (mNumberOfBands / 2));

                    frequencyBandMeters[curBandIndex]->SetMeterLabel(frequencyBandDevice.GetBandLabel((uint16_t)curBandIndex));
                }

                if (numberOfBandsDisplayed > 0)
                    total /= numberOfBandsDisplayed;

                frequencyBandMeters[numberOfBandsDisplayed]->SetMeterValue((float)total / (float)kInputMax);
                frequencyBandMeters[numberOfBandsDisplayed]->SetMeterLabel("Total");
            }
		}
		break;

        case eTimerCheckSerialPortChange:
        {
            String selectedSerialPortName = comPortName->GetSelectedPort();
            if (selectedSerialPortName != frequencyBandDevice.GetSerialPortName())
            {
                frequencyBandDevice.SetSerialPortName(selectedSerialPortName);

                if (selectedSerialPortName.length() == 0)
                    frequencyBandDevice.Close();
                else
                    frequencyBandDevice.Open();
            }
        }
        break;

	}
}
