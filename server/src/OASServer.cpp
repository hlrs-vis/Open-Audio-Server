#include "OASServer.h"
#ifdef WIN32
#include <windows.h>
#include <synchapi.h>
//static void sleep(unsigned int dwMilliseconds) { Sleep(dwMilliseconds); };
void sleep(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock())
        ;
}

#endif
oas::Server::Server() :
    _audioHandler(oas::AudioHandler::getInstance())
{
	_serverInfo = NULL;
}

// static
oas::Server& oas::Server::getInstance()
{
    static oas::Server *instance = new Server();

    return *instance;
}

const oas::ServerInfo* oas::Server::getServerInfo() const
{
	return _serverInfo;
}

// private
bool oas::Server::_readConfigFile(int argc, char **argv)
{
    // If there aren't any command line arguments
    if (argc < 2)
    {
        oas::Logger::logf("Usage: \"%s [config file]\"\n", argv[0]);
        return false;
    }
#ifdef WIN32
    WSADATA wsaData;

    if (WSAStartup(0x202, &wsaData) != 0)
    {
        printf("ERROR: Initialization failure.\n");
    }
#endif

    std::string configFilePath(argv[1]);

    // Check if the command line arg is unreasonably large to stop simple buffer overflow attacks
    if (configFilePath.size() > 1024)
    {
    	oas::Logger::logf("The specified config file location is unreasonably large.\n");
    	return false;
    }

    oas::FileHandler fh;

	// Check the first argument as the config file location
	if (!fh.doesFileExist(configFilePath))
	{
		oas::Logger::errorf("The specified configuration file at '%s' does not exist.\n", configFilePath.c_str());
		return false;
	}

    // Load the XML file
    if (!fh.loadXML(configFilePath, "OAS"))
    {
    	oas::Logger::errorf("Unable to load a valid configuration file!");
    	return false;
    }

    /*
     * Parse required sections of the config file:
     *   cache directory
     *   port
     */

    std::string cacheDirectory; 
    std::string port;

    // The config file MUST contain a cache directory and the port number the server will listen on
    // Find these two items in the XML file
    if (fh.findXML("cache_directory", NULL, NULL, cacheDirectory) &&
        fh.findXML("port", NULL, NULL, port))
    {
        this->_serverInfo = new ServerInfo(cacheDirectory, atol(port.c_str()));
    }
    else
    {
        if (!cacheDirectory.size())
            this->_fatalError("Could not retrieve cache directory information from configuration file.");
        else if (!port.size())
            this->_fatalError("Could not retrieve port number information from configuration file.");
    }

    /*
     * Parse optional sections of the config file:
     *   audioDevice
     *   gui
     */
    std::string audioDevice;
    std::string gui;

    if (fh.findXML("audio_device", NULL, NULL, audioDevice) && audioDevice.size())
        this->_serverInfo->setAudioDeviceString(audioDevice);

    // GUI is enabled by default. We disable it only if explicitly specified
    this->_serverInfo->setGUI(true);

    if (fh.findXML("gui", NULL, NULL, gui)
    		&& gui.size())
    {
        if (!gui.compare("off") 
            || !gui.compare("false")
            || !gui.compare("no")
            || !gui.compare("disable")
            || !gui.compare("disabled"))
        {
            oas::Logger::logf("The GUI has been disabled, as requested by the configuration file.");
            this->_serverInfo->setGUI(false);
        }
    }

    return true;
}

// private
void oas::Server::_processMessage(const Message &message)
{
    // If error, don't process message contents
    if (oas::Message::MERROR_NONE != message.getError())
    {
        return;
    }
    
    int newSource, state;
    unsigned int delay = 5;

    switch(message.getMessageType())
    {
        case oas::Message::MT_GHDL_FN:
            // Generate new audio source based on filename
            // 1) If the file is already loaded into a buffer, the buffer is reused in the new source
            // 2) Else look at filesystem to see if file exists
            //      2a) If exists, load audio file into a new buffer, then create new source
            //      2b) Else file does not exist, send  "-1" response
//            oas::Logger::logf("GHDL %s", message.getFilename());
            newSource = _audioHandler.createSource(message.getFilename());
            if (-1 == newSource)
                oas::Logger::logf("Server was unable to generate new audio source for file \"%s\".",
                                  message.getFilename().c_str());
            else
                oas::Logger::logf("New sound source created for \"%s\". (Sound ID = %d)",
                                  message.getFilename().c_str(),
                                  newSource);
            oas::SocketHandler::addOutgoingResponse(newSource);
            break;
        case oas::Message::MT_WAVE_1I_3F:
            newSource = _audioHandler.createSource(message.getIntegerParam(),
                                                        message.getFloatParam(0),
                                                        message.getFloatParam(1),
                                                        message.getFloatParam(2));
            if (-1 == newSource)
                oas::Logger::logf("Server was unable to generate a new audio source based on the waveform:\n"
                                  "    waveshape = %d, freq = %.2f, phase = %.2f, duration = %.2f",
                                  message.getIntegerParam(), message.getFloatParam(0), message.getFloatParam(1),
                                  message.getFloatParam(2));
            else
                oas::Logger::logf("New sound source created based on the waveform:\n"
                                    "    waveshape = %d, freq = %.2f, phase = %.2f, duration = %.2f",
                                    message.getIntegerParam(), message.getFloatParam(0), message.getFloatParam(1),
                                    message.getFloatParam(2));
            oas::SocketHandler::addOutgoingResponse(newSource);
            break;
        case oas::Message::MT_RHDL_HL:
            _audioHandler.deleteSource(message.getHandle());
            break;
        case oas::Message::MT_PTFI_FN_1I:
            // Shouldn't need to do anything!
            break;
        case oas::Message::MT_PLAY_HL:
            _audioHandler.playSource(message.getHandle());
            break;
        case oas::Message::MT_STOP_HL:
            _audioHandler.stopSource(message.getHandle());
            break;
        case oas::Message::MT_PAUS_HL:
            _audioHandler.pauseSource(message.getHandle());
            break;
        case oas::Message::MT_SSEC_HL_1F:
            _audioHandler.setSourcePlaybackPosition(message.getHandle(), message.getFloatParam(0));
            break;
        case oas::Message::MT_SSPO_HL_3F:
            _audioHandler.setSourcePosition( message.getHandle(),
                                                  message.getFloatParam(0), 
                                                  message.getFloatParam(1),
                                                  message.getFloatParam(2));
            break;
        case oas::Message::MT_SSVO_HL_1F:
            _audioHandler.setSourceGain( message.getHandle(),
                                              message.getFloatParam(0));
            break;
        case oas::Message::MT_SSLP_HL_1I:
            _audioHandler.setSourceLoop( message.getHandle(),
                                              message.getIntegerParam());
            break;
        case oas::Message::MT_SSVE_HL_1F:
            oas::Logger::warnf("SSVE using speed instead of velocity is deprecated!");
            _audioHandler.setSourceSpeed( message.getHandle(),
                                               message.getFloatParam(0));
            break;
        case oas::Message::MT_SSVE_HL_3F:
            _audioHandler.setSourceVelocity( message.getHandle(),
                                                  message.getFloatParam(0),
                                                  message.getFloatParam(1),
                                                  message.getFloatParam(2));
            break;
        case oas::Message::MT_SSDI_HL_1F:
            _audioHandler.setSourceDirection( message.getHandle(),
                                                   message.getFloatParam(0));
            break;
        case oas::Message::MT_SSDI_HL_3F:
            _audioHandler.setSourceDirection( message.getHandle(),
                                                   message.getFloatParam(0),
                                                   message.getFloatParam(1),
                                                   message.getFloatParam(2));
            break;
        case oas::Message::MT_SSDV_HL_1F_1F:
            _audioHandler.setSourceDirection( message.getHandle(),
                                                   message.getFloatParam(0));
            _audioHandler.setSourceGain( message.getHandle(),
                                              message.getFloatParam(1));
            break;
        case oas::Message::MT_SPIT_HL_1F:
            _audioHandler.setSourcePitch(	message.getHandle(),
            									message.getFloatParam(0));
            break;
        case oas::Message::MT_FADE_HL_1F_1F:
        	_audioHandler.setSourceFade(message.getHandle(),
        									 message.getFloatParam(0),
        									 message.getFloatParam(1));
        	break;
        case oas::Message::MT_SPAR_HL_1I_1F:
            _audioHandler.setSoundRenderingParameter(message.getHandle(),
                                                        message.getIntegerParam(),
                                                        message.getFloatParam(0));
            break;
        case oas::Message::MT_STAT_HL:
            state = _audioHandler.getSourceState(message.getHandle());
            oas::SocketHandler::addOutgoingResponse(state);
            break;
        case oas::Message::MT_SLPO_3F:
            _audioHandler.setListenerPosition(message.getFloatParam(0),
                                                   message.getFloatParam(1),
                                                   message.getFloatParam(2));
            break;
        case oas::Message::MT_SLVE_3F:
            _audioHandler.setListenerVelocity(message.getFloatParam(0),
                                                   message.getFloatParam(1),
                                                   message.getFloatParam(2));
            break;
        case oas::Message::MT_GAIN_1F:
            _audioHandler.setListenerGain(message.getFloatParam(0));
            break;
        case oas::Message::MT_SLOR_3F_3F:
            _audioHandler.setListenerOrientation( message.getFloatParam(0),
                                                       message.getFloatParam(1),
                                                       message.getFloatParam(2),
                                                       message.getFloatParam(3),
                                                       message.getFloatParam(4),
                                                       message.getFloatParam(5));
            break;
        case oas::Message::MT_PARA_1I_1F:
            _audioHandler.setGlobalRenderingParameter(message.getIntegerParam(),
                                                           message.getFloatParam(0));
            break;
        case oas::Message::MT_SSDR_HL_1F:
            oas::Logger::warnf("SSDR is unsupported! Ignoring instruction.");
            break;
        case oas::Message::MT_SSRV_HL_1F_1F:
            oas::Logger::warnf("SSRV is unsupported! Ignoring instruction.");
            break;
        case Message::MT_SSRV_HL_3F_1F:
            oas::Logger::warnf("SSRV is unsupported! Ignoring instruction.");
            break;
        case oas::Message::MT_TEST:
            break;
        case oas::Message::MT_SYNC:
            // Send a simple "SYNC" response
            oas::SocketHandler::addOutgoingResponse("SYNC");
            break;
        case oas::Message::MT_QUIT:
            oas::Logger::logf("Terminating current session.");
#ifdef FLTK_FOUND
            oas::ServerWindow::reset();
#endif
            // Will need to release all audio resources and then re-initialize them
            _audioHandler.release();
            // If for some reason initialization fails, try again
            while (!_audioHandler.initialize(getServerInfo()->getAudioDeviceString()))
            {
                oas::Logger::errorf("Failed to reset audio resources. Trying again in %d seconds.", delay);
                sleep(delay);
                delay += 5;
            }
            break;
        default:
            return;
    }
}

// public
void oas::Server::initialize(int argc, char **argv)
{
	// Attempt to read the config file specified by the command line argument
    if (!this->_readConfigFile(argc, argv))
    {
    	// Exit if failed
    	exit(1);
    }
#ifdef FLTK_FOUND
    if (getServerInfo()->useGUI()
        && !oas::ServerWindow::initialize(argc, argv, &oas::Server::terminate))
    {
        _fatalError("Could not initialize the windowed user interface!");
    }
#endif
    if (!oas::FileHandler::initialize(this->_serverInfo->getCacheDirectory()))
    {
        _fatalError("Could not initialize the File Handler!");
    }

    if (!_audioHandler.initialize(this->_serverInfo->getAudioDeviceString()))
    {
        _fatalError("Could not initialize the Audio Handler!");
    }

    if (!oas::SocketHandler::initialize(this->_serverInfo->getPort()))
    {
        _fatalError("Could not initialize the Socket Handler!");
    }

    // Thread attribute variable
    pthread_attr_t threadAttr;

    // Initialize thread attribute
    pthread_attr_init(&threadAttr);

    // Set joinable thread attribute
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_JOINABLE);

    // Spawn thread to run the core server loop
    int threadError;

#ifdef FLTK_FOUND
    // With the GUI
    if (getServerInfo()->useGUI())
        threadError = pthread_create(&this->_serverThread, &threadAttr, &oas::Server::runServer, NULL);
    // Without the GUI
    else
        threadError = pthread_create(&this->_serverThread, &threadAttr, &oas::Server::runServerNoGUI, NULL);
#else
    // Without the GUI
    threadError = pthread_create(&this->_serverThread, &threadAttr, &oas::Server::runServerNoGUI, NULL);
#endif
    // Destroy thread attribute
    pthread_attr_destroy(&threadAttr);

    if (threadError)
    {
        _fatalError("Could not create server thread!");
    }
}

#ifdef FLTK_FOUND
// private
void* oas::Server::_run(void *parameter)
{
    std::queue<Message*> messages;
    std::queue<const AudioUnit*> sources;
    const AudioUnit *audioUnit;

    Time timeOut;

    // Add the listener to the GUI, before the loop even starts
    oas::ServerWindow::audioListenerWasModified(_audioHandler.getListener());

    while (1)
    {
    	// Update timeOut to current time
    	timeOut.update(oas::Time::OAS_CLOCK_MONOTONIC);

        // If a client is connected, use a very short timeout allowing for fast updates
        if (SocketHandler::isConnectedToClient())
        	timeOut += Time(0.0005); 	// 0.5 ms -> maximum of ~2000 loop iterations per second
        // Else use a longer timeout to save CPU cycles
        else
            timeOut += Time(2);

        // If there are no incoming messages, populateQueueWithIncomingMessages() will block
        // until timeout
        oas::SocketHandler::populateQueueWithIncomingMessages(messages, timeOut);

        while (!messages.empty())
        {
            Message *nextMessage = messages.front();
            oas::Server::getInstance()._processMessage(*nextMessage);
//            oas::Logger::logf("Server processed message \"%s\"", nextMessage->getOriginalString().c_str());
            delete nextMessage;
            messages.pop();

            audioUnit = _audioHandler.getRecentlyModifiedAudioUnit();
            if (audioUnit)
            {
                // Call ServerWindow method that will queue up the source
                oas::ServerWindow::audioUnitWasModified(audioUnit);
            }
        }

        _audioHandler.populateQueueWithUpdatedSources(sources);

        if (!sources.empty())
             oas::ServerWindow::audioSourcesWereModified(sources);

    }

    return NULL;
}
#endif

// private
void* oas::Server::_runNoGUI(void *parameter)
{
    std::queue<Message*> messages;
    Time timeOut;

    while (1)
    {
        // Update timeOut to current time
        timeOut.update(oas::Time::OAS_CLOCK_MONOTONIC);

        // If a client is connected, use a very short timeout allowing for fast updates
        if (SocketHandler::isConnectedToClient())
            timeOut += Time(0.0005);    // 0.5 ms -> maximum of ~2000 loop iterations per second
        // Else use a longer timeout to save CPU cycles
        else
            timeOut += Time(2);

        // If there are no incoming messages, populateQueueWithIncomingMessages() will block
        // until timeout
        oas::SocketHandler::populateQueueWithIncomingMessages(messages, timeOut);

        while (!messages.empty())
        {
            Message *nextMessage = messages.front();
            oas::Server::getInstance()._processMessage(*nextMessage);
            //oas::Logger::logf("Server processed message \"%s\"", nextMessage->getOriginalString().c_str());
            delete nextMessage;
            messages.pop();
        }

        _audioHandler.updateSources();
    }

    return NULL;
}

// private
void oas::Server::_fatalError(const char *errorMessage)
{
	if (!errorMessage)
		errorMessage = "(no error was provided)";

    std::cerr << "OAS: Fatal Error occured!\n"
              << "     Error: " << errorMessage << "\n"
              << "Exiting OAS...\n\n";
    exit(1);
}

void oas::Server::_atExit()
{
#ifdef FLTK_FOUND
        oas::ServerWindow::reset();
#endif
    oas::SocketHandler::terminate();
    _audioHandler.release();
}

void oas::Server::terminate()
{
    oas::Server::getInstance()._atExit();
}

void* oas::Server::runServer(void *parameter)
{
    return oas::Server::getInstance()._run();
}

void* oas::Server::runServerNoGUI(void *parameter)
{
    return oas::Server::getInstance()._runNoGUI();
}

// Main
int main(int argc, char **argv)
{
    oas::Logger::logf("Starting up the OpenAL Audio Server...");

    oas::Server &server = oas::Server::getInstance();

    // Initialize all of the components of the server
    server.initialize(argc, argv);


#ifdef FLTK_FOUND
    // Fl::run() puts all of the FLTK window rendering on this current thread (main thread)
    // It will only return when program execution is meant to terminate
    if (server.getServerInfo()->useGUI())
    {
    	return Fl::run();
    }
    else
    {
    	oas::SocketHandler::waitForSocketHandlerToTerminate();
    	return 0;
    }
#else
    oas::SocketHandler::waitForSocketHandlerToTerminate();
    return 0;
#endif
}


