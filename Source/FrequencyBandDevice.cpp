#include "FrequencyBandDevice.h"

#define kBPS 115200

///////////////////////////////////////////////////////
// FrequencyBandDevice
FrequencyBandDevice::FrequencyBandDevice()
    : Thread(String("FrequencyBandDevice")),
    mNumberOfBands(0),
    mThreadTask(eThreadTaskIdle)
{
    mParseState = eParseStateIdle;
    mThreadTask = eThreadTaskIdle;

    // start the serial thread reading data
    startThread();
}

FrequencyBandDevice::~FrequencyBandDevice()
{
    stopThread(500);
    CloseSerialPort();
}

uint16_t FrequencyBandDevice::GetNumberOfBands(void)
{
    return mNumberOfBands;
}

void FrequencyBandDevice::SetSerialPortName(String serialPortName)
{
    mCurrentSerialPortName = serialPortName;
}

String FrequencyBandDevice::GetSerialPortName(void)
{
    return mCurrentSerialPortName;
}

void FrequencyBandDevice::Open(void)
{
    mThreadTask = eThreadTaskOpenSerialPort;
}

void FrequencyBandDevice::Close(void)
{
    mThreadTask = eThreadTaskCloseSerialPort;
}

void FrequencyBandDevice::OpenSerialPort(void)
{
    mSerialPort = new SerialPort();
    bool opened = mSerialPort->open(mCurrentSerialPortName);

    if (opened)
    {
        SerialPortConfig serialPortConfig;
        mSerialPort->getConfig(serialPortConfig);
        serialPortConfig.bps = kBPS;
        serialPortConfig.databits = 8;
        serialPortConfig.parity = SerialPortConfig::SERIALPORT_PARITY_NONE;
        serialPortConfig.stopbits = SerialPortConfig::STOPBITS_1;
        mSerialPort->setConfig(serialPortConfig);

        mSerialPortInput = new SerialPortInputStream(mSerialPort);
    }
    else
    {
        // report error
        Logger::outputDebugString(String("Unable to open serial port:") + mCurrentSerialPortName);
    }
}

void FrequencyBandDevice::CloseSerialPort(void)
{
    SetNumberOfBands(0);
    mSerialPortInput = nullptr;
    if (mSerialPort != nullptr)
    {
        mSerialPort->close();
        mSerialPort = nullptr;
    }
}

void FrequencyBandDevice::SetNumberOfBands(uint16_t numberOfBands)
{
    Logger::outputDebugString(String(numberOfBands) + String(" ************band count changed************"));

    const ScopedLock scopedLock_Data(mFrequencyBandDataLock);
    const ScopedLock scopedLock_Labels(mFrequencyBandLabelLock);

    mNumberOfBands = numberOfBands;

    mFrequencyBandData.resize(mNumberOfBands);
    mFrequencyBandLabels.resize(mNumberOfBands);
}

int FrequencyBandDevice::GetBandData(uint16_t bandIndex)
{
    const ScopedLock scopedLock(mFrequencyBandDataLock);
    return mFrequencyBandData[bandIndex];
}

String FrequencyBandDevice::GetBandLabel(uint16_t bandIndex)
{
    const ScopedLock scopedLock(mFrequencyBandLabelLock);
    return mFrequencyBandLabels[bandIndex];
}

#define kSerialPortBufferLen 256
void FrequencyBandDevice::run()
{
    while (!threadShouldExit())
    {
        switch (mThreadTask)
        {
        case eThreadTaskOpenSerialPort:
        {
            OpenSerialPort();
            mThreadTask = eThreadTaskReadSerialPort;
        }
        break;

        case eThreadTaskCloseSerialPort:
        {
            CloseSerialPort();
            mThreadTask = eThreadTaskIdle;
        }
        break;

        case eThreadTaskReadSerialPort:
        {
            // handle reading from the serial port
            if ((mSerialPortInput != nullptr) && (!mSerialPortInput->isExhausted()))
            {
                int  bytesRead = 0;
                int  curByteOffset = 0;
                char incomingData[kSerialPortBufferLen] = "";

                bytesRead = mSerialPortInput->read(incomingData, kSerialPortBufferLen);
                if (bytesRead < 1)
                {
                    wait(1);
                    continue;
                }
                else
                {
                    // parse incoming data
                    while (curByteOffset < bytesRead)
                    {
                        switch (mParseState)
                        {
                        case eParseStateIdle:
                        {
                            if (incomingData[curByteOffset] == kBeginPacket)
                            {
                                mRawSerialData = String::empty;
                                mParseState = eParseStateReading;
                            }
                        }
                        break;

                        case eParseStateReading:
                        {
                            if (incomingData[curByteOffset] == kEndPacket)
                            {
                                mParseState = eParseStateIdle;
                                Logger::outputDebugString("pkt: " + mRawSerialData);
                                int startOfByteCount = mRawSerialData.indexOf("|");
                                if (startOfByteCount == -1)
                                    break;
                                int expectedByteCount = mRawSerialData.substring(startOfByteCount + 1).getIntValue();
                                if (expectedByteCount != startOfByteCount + 1)
                                    break;

                                char cmd = mRawSerialData[0];
                                // skip over command byte
                                mRawSerialData = mRawSerialData.substring(1);
                                switch (cmd)
                                {
                                case kBandData:
                                {
                                    // count:<comma separated band data>
                                    // D7:126,100,5,34,79,80,120
                                    int bandCount = mRawSerialData.getIntValue();
                                    if (bandCount > 0)
                                    {
                                        if (bandCount != mNumberOfBands)
                                            SetNumberOfBands(bandCount);

                                        // skip over band count and ':' separator
                                        mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789"));
                                        mRawSerialData = mRawSerialData.trimCharactersAtStart(String(":"));

                                        // read in data for each band
                                        {
                                            const ScopedLock scopedLock(mFrequencyBandDataLock);
                                            for (int curBandIndex = 0; curBandIndex < mNumberOfBands; ++curBandIndex)
                                            {
                                                // 126,100,5,etc
                                                mFrequencyBandData.set(curBandIndex, mRawSerialData.getIntValue());
                                                mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789."));
                                                mRawSerialData = mRawSerialData.trimCharactersAtStart(String(","));
                                            }
                                        }
                                    }
                                }
                                break;

                                case kBandLabels:
                                {
                                    // count:<comma separated quoted labels data>
                                    // L7:"1Hz","5Hz","10Hz","20Hz","40Hz","80Hz","160Hz"
                                    int bandCount = mRawSerialData.getIntValue();
                                    if (bandCount > 0)
                                    {
                                        if (bandCount != mNumberOfBands)
                                            SetNumberOfBands(bandCount);

                                        // skip over band count and ':' separator
                                        mRawSerialData = mRawSerialData.trimCharactersAtStart(String("0123456789"));
                                        mRawSerialData = mRawSerialData.trimCharactersAtStart(String(":"));

                                        // read in label for each band
                                        {
                                            const ScopedLock scopedLock(mFrequencyBandLabelLock);
                                            for (int curBandIndex = 0; curBandIndex < mNumberOfBands; ++curBandIndex)
                                            {
                                                // find and strip off first "
                                                mRawSerialData = mRawSerialData.fromFirstOccurrenceOf(String("\""), false, false);
                                                // "20Hz", "10Hz", etc
                                                mFrequencyBandLabels[curBandIndex] = mRawSerialData.upToFirstOccurrenceOf(String("\""), false, false);
                                                // skip ending '",'
                                                mRawSerialData = mRawSerialData.fromFirstOccurrenceOf(String("\","), false, false);
                                            }
                                        }
                                    }
                                }
                                break;

                                default:
                                {
                                    // error - unknown cmd
                                }
                                break;
                                }
                            }
                            else
                            {
                                mRawSerialData += String(String::charToString(incomingData[curByteOffset]));
                            }
                        }
                        break;
                        }

                        ++curByteOffset;
                    }
                }
            }
            else
            {
                wait(1);
            }
        }
        break;
        }
    }
}
