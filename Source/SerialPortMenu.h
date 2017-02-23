#ifndef __SERIAL_PORT_MENU_H__
#define __SERIAL_PORT_MENU_H__

#include "../JuceLibraryCode/JuceHeader.h"
#include "cSerialPortListMonitor.h"

////////////////////////////////////////////////////
// SerialPortMenu
////////////////////////////////////////////////////

enum MenuIDs
{
    eUsbPortSelectStart = 1500,
    eUsbPortSelectEnd = 1598,
    eUsbPortSelectNoneAvail = 1599,
};

class SerialPortMenu : public Label
{
public:
    SerialPortMenu(String _componentName, String _label, ApplicationProperties* applicationProperties);

    void   SetSelectedPort(String deviceName);
    String GetSelectedPort(void);

    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;
    void mouseDown(const MouseEvent &event) override;

private:
    ScopedPointer<cSerialPortListMonitor> mSerialPortListMonitorThread;
    String                                mSelectedPort;
    ApplicationProperties*                mApplicationProperties;
};

#endif // __SERIAL_PORT_MENU_H__