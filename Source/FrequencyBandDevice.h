#ifndef __FREQUENCY_BAND_DEVICE_H__
#define __FREQUENCY_BAND_DEVICE_H__

#include "../JuceLibraryCode/JuceHeader.h"

class FrequencyBandDevice : public Thread
{
public:
    FrequencyBandDevice(void);
    ~FrequencyBandDevice();

    void SetSerialPortName(String _serialPortName);
    String GetSerialPortName(void);
    void Open(void);
    void Close(void);

    uint16_t GetNumberOfBands(void);
    int      GetBandData(uint16_t _bandIndex);
    String   GetBandLabel(uint16_t _bandIndex);

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

    void SetNumberOfBands(uint16_t _numberOfBands);

    bool OpenSerialPort(void);
    void CloseSerialPort(void);

    String                               mCurrentSerialPortName;
    ScopedPointer<SerialPort>            mSerialPort;
    ScopedPointer<SerialPortInputStream> mSerialPortInput;
    
    int			  mParseState{ eParseStateIdle };
    int			  mNumberOfBands{ 0 };
    String		  mRawSerialData;
    Array<int>    mFrequencyBandData;
    Array<String> mFrequencyBandLabels;
    uint16_t      mThreadTask{ eThreadTaskIdle };

    CriticalSection mFrequencyBandDataLock;
    CriticalSection mFrequencyBandLabelLock;
};

#endif // __FREQUENCY_BAND_DEVICE_H__
