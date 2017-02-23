#ifndef __FREQUENCY_BAND_DEVICE_H__
#define __FREQUENCY_BAND_DEVICE_H__

#include "../JuceLibraryCode/JuceHeader.h"

class FrequencyBandDevice : public Thread
{
public:
    FrequencyBandDevice(void);
    ~FrequencyBandDevice();

    void SetSerialPortName(String serialPortName);
    String GetSerialPortName(void);
    void Open(void);
    void Close(void);

    uint16_t GetNumberOfBands(void);
    int      GetBandData(uint16_t bandIndex);
    String   GetBandLabel(uint16_t bandIndex);

private:
    #define kBeginPacket '<'
    #define kEndPacket   '>'
    #define kBandData    'D'
    #define kBandLabels  'L'

    enum
    {
        eParseStateIdle = 0,
        eParseStateReading,
    };

    enum
    {
        eThreadTaskIdle,
        eThreadTaskOpenSerialPort,
        eThreadTaskCloseSerialPort,
        eThreadTaskReadSerialPort,
    };

    void run() override;

    void SetNumberOfBands(uint16_t numberOfBands);

    void OpenSerialPort(void);
    void CloseSerialPort(void);

    String                               mCurrentSerialPortName;
    ScopedPointer<SerialPort>            mSerialPort;
    ScopedPointer<SerialPortInputStream> mSerialPortInput;
    int			  mParseState;
    int			  mNumberOfBands;
    String		  mRawSerialData;
    Array<int>    mFrequencyBandData;
    Array<String> mFrequencyBandLabels;

    uint16_t    mThreadTask;

    CriticalSection mFrequencyBandDataLock;
    CriticalSection mFrequencyBandLabelLock;
};

#endif // __FREQUENCY_BAND_DEVICE_H__