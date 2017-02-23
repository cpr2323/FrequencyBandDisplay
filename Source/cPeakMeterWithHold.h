#ifndef __C_PEAK_METER_WITH_HOLD_H__
#define __C_PEAK_METER_WITH_HOLD_H__

#include "../JuceLibraryCode/JuceHeader.h"
#include "cSinglePoleFilter.h"

////////////////////////////////////////////////////
// cPeakMeterWithHold
////////////////////////////////////////////////////
class cPeakMeterWithHold
{
public:
    cPeakMeterWithHold(int64 _holdTime = 1000)
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

#endif // __C_PEAK_METER_WITH_HOLD_H__