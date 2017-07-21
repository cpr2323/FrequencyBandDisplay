#include "SerialPortMenu.h"

SerialPortMenu::SerialPortMenu(String _componentName, String _label, ApplicationProperties* _applicationProperties)
    : Label(_componentName, _label),
      mApplicationProperties(_applicationProperties)
{
    SetSelectedPort(mApplicationProperties->getUserSettings()->getValue("UsbPortName"));
    mSerialPortListMonitorThread = new cSerialPortListMonitor;
}

void SerialPortMenu::SetSelectedPort(String _deviceName)
{
    mApplicationProperties->getUserSettings()->setValue("UsbPortName", _deviceName);
    mSelectedPort = _deviceName;
    if (_deviceName.length() != 0)
        setText(mSelectedPort, NotificationType::dontSendNotification);
    else
        setText("<none>", NotificationType::dontSendNotification);
}

String SerialPortMenu::GetSelectedPort(void)
{
    return mSelectedPort;
}

void SerialPortMenu::mouseEnter(const MouseEvent &_event)
{
    setColour(textColourId, Colours::green);
}

void SerialPortMenu::mouseExit(const MouseEvent &_event)
{
    setColour(textColourId, Colours::black);
}

void SerialPortMenu::mouseDown(const MouseEvent &_event)
{
    PopupMenu menu;

    StringPairArray serialPortList = mSerialPortListMonitorThread->GetSerialPortList();
    if (serialPortList.size() == 0)
    {
        menu.addItem(eUsbPortSelectNoneAvail, "<none>");
    }
    else
    {
        for (unsigned int curSerialPortListIndex = 0; curSerialPortListIndex < serialPortList.size(); ++curSerialPortListIndex)
            menu.addItem(curSerialPortListIndex + 1, serialPortList.getAllKeys()[curSerialPortListIndex], true, serialPortList.getAllValues()[curSerialPortListIndex] == mSelectedPort);
    }

    auto serialPortSelected = menu.show();

    if (serialPortSelected > 0)
        if (serialPortList.getAllValues()[serialPortSelected - 1] != mSelectedPort)
            SetSelectedPort(serialPortList.getAllValues()[serialPortSelected - 1]);
        else
            SetSelectedPort("");
}
