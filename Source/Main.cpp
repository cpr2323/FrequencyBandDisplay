#include "../JuceLibraryCode/JuceHeader.h"
#include "FrequencyBandDisplayMainWindow.h"

//==============================================================================
/**
    This is the top-level window that we'll pop up. Inside it, we'll create and
    show a component from the MainComponent.cpp file (you can open this file using
    the Jucer to edit it).
*/
class FrequencyDisplayWindow  : public DocumentWindow
{
public:
    //==============================================================================
    FrequencyDisplayWindow(void)
        : DocumentWindow ("Frequency Display",
                          Colours::lightgrey,
                          DocumentWindow::allButtons,
                          true)
    {
        // Create an instance of our main content component, and add it to our window..
        setContentOwned (new FrequencyBandDisplayMainWindow(), true);

        // Centre the window on the screen
        centreWithSize (getWidth(), getHeight());

        setResizable(true, false);
        
        // And show it!
        setVisible (true);
    }

    ~FrequencyDisplayWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    //==============================================================================
    void closeButtonPressed()
    {
        // When the user presses the close button, we'll tell the app to quit. This
        // ArduinoFHTWindow object will be deleted by the JUCEArduinoFHTApplication class.
        JUCEApplication::quit();
    }
};

//==============================================================================
/** This is the application object that is started up when Juce starts. It handles
    the initialisation and shutdown of the whole application.
*/
class JUCEFrequencyDisplayApplication : public JUCEApplication
{
public:
    //==============================================================================
    JUCEFrequencyDisplayApplication()
    {
    }

    ~JUCEFrequencyDisplayApplication()
    {
    }

    //==============================================================================
    void initialise (const String& commandLine)
    {
        // For this demo, we'll just create the main window...
        frequencyDisplayWindow = new FrequencyDisplayWindow();

        /*  ..and now return, which will fall into to the main event
            dispatch loop, and this will run until something calls
            JUCEAppliction::quit().

            In this case, JUCEAppliction::quit() will be called by the
            hello world window being clicked.
        */
    }

    void shutdown()
    {
        // This method is where you should clear-up your app's resources..

        // The helloWorldWindow variable is a ScopedPointer, so setting it to a null
        // pointer will delete the window.
        frequencyDisplayWindow = nullptr;
    }

    //==============================================================================
    const String getApplicationName()
    {
        return "Frequency Display";
    }

    const String getApplicationVersion()
    {
        // The ProjectInfo::versionString value is automatically updated by the Jucer, and
        // can be found in the JuceHeader.h file that it generates for our project.
        return ProjectInfo::versionString;
    }

    bool moreThanOneInstanceAllowed()
    {
        return true;
    }

    void anotherInstanceStarted (const String& commandLine)
    {
    }

private:
    ScopedPointer<FrequencyDisplayWindow> frequencyDisplayWindow;
};


//==============================================================================
// This macro creates the application's main() function..
START_JUCE_APPLICATION (JUCEFrequencyDisplayApplication)
