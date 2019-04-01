#ifndef __C_SINGLE_POLE_FILTER_H__
#define __C_SINGLE_POLE_FILTER_H__

#include "../JuceLibraryCode/JuceHeader.h"

////////////////////////////////////////////////////
// cSinglePoleFilter
////////////////////////////////////////////////////
class cSinglePoleFilter
{
public:
    cSinglePoleFilter(float a)
    {
        Config(a);
    }
    
    cSinglePoleFilter(void)
        : cSinglePoleFilter(1.0f)
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

    void SetCurValue(float z)
    {
        mZ = z;
    }

    float GetFilteredValue(void)
    {
        return mZ;
    }

private:
    float mA{ 0.0f };
    float mB{ 0.0f };
    float mZ{ 0.0f };
};

#endif // __C_SINGLE_POLE_FILTER_H__
