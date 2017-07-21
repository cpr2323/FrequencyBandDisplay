#ifndef __C_PEAK_METER_WITH_HOLD_H__
#define __C_PEAK_METER_WITH_HOLD_H__

#include "../JuceLibraryCode/JuceHeader.h"
#include "cSinglePoleFilter.h"

static const auto kDefaultHoldTime = 1000;
static const auto kDefaultFilterValue = 0.99f;

////////////////////////////////////////////////////
// cPeakMeterWithHold
////////////////////////////////////////////////////
class cPeakMeterWithHold
{
public:
    cPeakMeterWithHold(int64 _holdTime = kDefaultHoldTime)
        : mHoldTime(_holdTime),
          mHoldTimeStart(Time::currentTimeMillis() - mHoldTime)
    {
        mFilteredPeakValue.Config(kDefaultFilterValue);
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
    float             mPeakValue{ 0.0f };
    cSinglePoleFilter mFilteredPeakValue;
    int               mHoldTime{ 0 };
    int64             mHoldTimeStart{ 0 };
};

#endif // __C_PEAK_METER_WITH_HOLD_H__
