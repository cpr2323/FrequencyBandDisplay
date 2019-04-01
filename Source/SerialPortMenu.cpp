#include "SerialPortMenu.h"

SerialPortMenu::SerialPortMenu(String componentName, String label, ApplicationProperties* applicationProperties)
    : Label(componentName, label),
      mApplicationProperties(applicationProperties)
{
    SetSelectedPort(mApplicationProperties->getUserSettings()->getValue("UsbPortName"));
    mSerialPortListMonitorThread = new cSerialPortListMonitor;
}

void SerialPortMenu::SetSelectedPort(String deviceName)
{
    mApplicationProperties->getUserSettings()->setValue("UsbPortName", deviceName);
    mSelectedPort = deviceName;
    if (deviceName.length() != 0)
        setText(mSelectedPort, NotificationType::dontSendNotification);
    else
        setText("<none>", NotificationType::dontSendNotification);
}

String SerialPortMenu::GetSelectedPort(void)
{
    return mSelectedPort;
}

void SerialPortMenu::mouseEnter(const MouseEvent &)
{
    setColour(textColourId, Colours::green);
}

void SerialPortMenu::mouseExit(const MouseEvent &)
{
    setColour(textColourId, Colours::black);
}

void SerialPortMenu::mouseDown(const MouseEvent &)
{
    PopupMenu menu;

    StringPairArray serialPortList = mSerialPortListMonitorThread->GetSerialPortList();
    if (serialPortList.size() == 0)
    {
        menu.addItem(eUsbPortSelectNoneAvail, "<none>");
    }
    else
    {
        for (int curSerialPortListIndex = 0; curSerialPortListIndex < serialPortList.size(); ++curSerialPortListIndex)
            menu.addItem(curSerialPortListIndex + 1, serialPortList.getAllKeys()[curSerialPortListIndex], true, serialPortList.getAllValues()[curSerialPortListIndex] == mSelectedPort);
    }

    const auto serialPortSelected = menu.show();

    if (serialPortSelected > 0)
        if (serialPortList.getAllValues()[serialPortSelected - 1] != mSelectedPort)
            SetSelectedPort(serialPortList.getAllValues()[serialPortSelected - 1]);
        else
            SetSelectedPort("");
}
