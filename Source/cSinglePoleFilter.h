#ifndef __C_SINGLE_POLE_FILTER_H__
#define __C_SINGLE_POLE_FILTER_H__

#include "../JuceLibraryCode/JuceHeader.h"

////////////////////////////////////////////////////
// cSinglePoleFilter
////////////////////////////////////////////////////
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
        mZ = _z;
    }

    float GetFilteredValue(void)
    {
        return mZ;
    }

private:
    float mA, mB, mZ;
};

#endif // __C_SINGLE_POLE_FILTER_H__