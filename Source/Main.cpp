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
    FrequencyDisplayWindow(String name, ApplicationProperties* _applicationProperties)
        : DocumentWindow (name, Colours::lightgrey, DocumentWindow::allButtons, true)
    {
        // Create an instance of our main content component, and add it to our window..
        setContentOwned (new FrequencyBandDisplayMainWindow(_applicationProperties), true);

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
    void initialise (const String& _commandLine)
    {
        mApplicationProperties = new ApplicationProperties();

        PropertiesFile::Options configuratorPropertiesOptions;
        configuratorPropertiesOptions.applicationName = getApplicationName();
        configuratorPropertiesOptions.filenameSuffix = "settings";
        configuratorPropertiesOptions.millisecondsBeforeSaving = 1000;
        configuratorPropertiesOptions.storageFormat = PropertiesFile::storeAsXML;

        // this is obviously only useful in OSX, but should neither help nor harm any
        // other OS, so we won't bother qualifying it for OSX only.
        configuratorPropertiesOptions.osxLibrarySubFolder = "Application Support";

        mApplicationProperties->setStorageParameters(configuratorPropertiesOptions);

        if (_commandLine.isNotEmpty())
        {
            processCommandLine(_commandLine);
        }

        mFrequencyDisplayWindow = new FrequencyDisplayWindow(getApplicationName(), mApplicationProperties);
    }

    void shutdown()
    {
        // This method is where you should clear-up your app's resources..

        // The helloWorldWindow variable is a ScopedPointer, so setting it to a null
        // pointer will delete the window.
        mFrequencyDisplayWindow = nullptr;
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

    void anotherInstanceStarted (const String& _commandLine)
    {
    }

private:
    void processCommandLine(const String& _commandLine)
    {
        StringArray args;
        args.addTokens(_commandLine, true);
        args.trim();

        //if (matchArgument(args[0], "help")) showHelp();
    }

    ScopedPointer<FrequencyDisplayWindow> mFrequencyDisplayWindow;
    ScopedPointer<ApplicationProperties>  mApplicationProperties;
};


//==============================================================================
// This macro creates the application's main() function..
START_JUCE_APPLICATION (JUCEFrequencyDisplayApplication)
