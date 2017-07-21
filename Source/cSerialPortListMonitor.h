#ifndef __C_SERIAL_PORT_LIST_MONITOR_H__
#define __C_SERIAL_PORT_LIST_MONITOR_H__

#include "../JuceLibraryCode/JuceHeader.h"

////////////////////////////////////////////////////
// cSerialPortListMonitor
////////////////////////////////////////////////////
#define kSerialPortListMonitorSleepTime 1000
class cSerialPortListMonitor : public Thread
{
public:
    cSerialPortListMonitor(void);
    ~cSerialPortListMonitor(void);
    bool HasListChanged(void);
    StringPairArray GetSerialPortList(void);
    void SetSleepTime(int _sleepTime);
    void run() override;

private:
    CriticalSection mDataLock;
    StringPairArray mSerialPortNames;
    int             mSleepTime{ kSerialPortListMonitorSleepTime };
    bool            mListChanged{ false };
};

#endif // __C_SERIAL_PORT_LIST_MONITOR_H__
