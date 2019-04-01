#include "cSerialPortListMonitor.h"

cSerialPortListMonitor::cSerialPortListMonitor(void)
    : Thread(String("Serial Port Monitor"))
{
    startThread();
}

cSerialPortListMonitor::~cSerialPortListMonitor(void)
{
    stopThread(5000);
}

StringPairArray cSerialPortListMonitor::GetSerialPortList(void)
{
    ScopedLock dataLock(mDataLock);
    mListChanged = false;
    return mSerialPortNames;
}

void cSerialPortListMonitor::SetSleepTime(int sleepTime)
{
    mSleepTime = sleepTime;
}

bool cSerialPortListMonitor::HasListChanged(void)
{
    return mListChanged;
}

// thread for keeping an up to date list of serial port names
void cSerialPortListMonitor::run()
{
    while (!threadShouldExit())
    {
        // wake up every mSleepTime to check the serial port list
        sleep(mSleepTime);

        const auto serialPortList(SerialPort::getSerialPortPaths());
        ScopedLock dataLock(mDataLock);
        if ((serialPortList.size() != mSerialPortNames.size()) || serialPortList != mSerialPortNames)
        {
            mSerialPortNames = serialPortList;
            mListChanged = true;
        }
    }
}
