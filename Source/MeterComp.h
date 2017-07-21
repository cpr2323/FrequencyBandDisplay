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

    void paint(Graphics& _g) override
    {
        // drae background
        _g.fillAll(Colours::grey);

        // drae main mater
        _g.setColour(Colours::green);
        float meterValue = mShowFilteredMeterValue ? mFilteredMeterValue.GetFilteredValue() : mMeterValue;
        int meterHeight = (int)(getHeight() * meterValue);
        _g.fillRect(0, getHeight() - meterHeight, getWidth(), meterHeight);
        if (mShowPeak && mPeakValue.GetValue() > 0.0)
        {
            _g.setColour(Colours::gold);
            int peakHeight = (int)(getHeight() * mPeakValue.GetValue());
            _g.fillRect(0, (getHeight() - peakHeight) - 2, getWidth(), 5);
        }
    }

    void resized(void) override
    {
        mMeterLabel.setBounds(1, 1, getWidth() - 2, 20);
    }

    void SetMeterValue(float _meterValue)
    {
        mMeterValue = _meterValue;

        if (_meterValue > mFilteredMeterValue.GetFilteredValue())
            mFilteredMeterValue.SetCurValue(_meterValue);
        else
            mFilteredMeterValue.DoFilter(_meterValue);

        //mPeakValue.SetValue(mFilteredMeterValue.GetFilteredValue());
        mPeakValue.SetValue(_meterValue);

        repaint();
    }

    void SetMeterLabel(String _meterLabel)
    {
        mMeterLabel.setText(_meterLabel, NotificationType::sendNotification);
        repaint();
    }

    void SetShoWFilteredMeterValue(bool _showFilteredMeterValue)
    {
        mShowFilteredMeterValue = _showFilteredMeterValue;
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
