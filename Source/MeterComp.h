#ifndef __METER_COMP_H__
#define __METER_COMP_H__

#include "../JuceLibraryCode/JuceHeader.h"
#include "cPeakMeterWithHold.h"

////////////////////////////////////////////////////
// MeterComp
////////////////////////////////////////////////////
class MeterComp : public Component
{
public:
    MeterComp(void)
    {
        addAndMakeVisible(mMeterLabel);
        mMeterLabel.setJustificationType(Justification::centredTop);
        mFilteredMeterValue.Config(0.9f);
        setOpaque(true);
    }

    void paint(Graphics& g) override
    {
        // drae background
        g.fillAll(Colours::grey);

        // drae main mater
        g.setColour(Colours::green);
        const float meterValue = mShowFilteredMeterValue ? mFilteredMeterValue.GetFilteredValue() : mMeterValue;
        const int meterHeight = (int)(getHeight() * meterValue);
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

    void SetShoWFilteredMeterValue(bool showFilteredMeterValue)
    {
        mShowFilteredMeterValue = showFilteredMeterValue;
    }

private:
    float             mMeterValue{ 0.0f };
    cSinglePoleFilter mFilteredMeterValue;
    Label             mMeterLabel;

    cPeakMeterWithHold mPeakValue;
    bool               mShowPeak{ true };
    bool               mShowFilteredMeterValue{ true };
};

#endif // __METER_COMP_H__
