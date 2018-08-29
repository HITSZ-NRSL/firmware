#include "wiring_ex_bridge.h"
#include "wiring_ex_process.h"

Process::~Process(void)
{
    close();
}

/**
 * Writes data to a Linux process.
 * This data is sent as a byte or series of bytes. write() inherits from the Stream utility class.
**/
size_t Process::write(uint8_t c)
{
    uint8_t cmd[] = {'I', handle, c};

    bridge.transfer(cmd, 3);
    return 1;
}

/**
 * Clears the Process buffer of any bytes. Waits for the transmission of outgoing data to complete.
**/
void Process::flush(void)
{
}

/**
 *Get the number of bytes (characters) available for reading from the Linux connection.
 *This is data that's already arrived and stored in the receive buffer.
**/
int Process::available(void)
{
    // Look if there is new data available
    doBuffer();
    return buffered;
}

/**
 * Reads incoming data from a Linux process. read() inherits from the Stream utility class.
**/
int Process::read(void)
{
    doBuffer();
    if (buffered == 0)
    {return -1;} // no chars available
    else
    {
        buffered--;
        return buffer[readPos++];
    }
}

/**
 * Returns the next byte (character) of incoming data from a Linux process without removing it from the internal buffer.
***/
int Process::peek(void)
{
    doBuffer();
    if (buffered == 0)
    {return -1;} // no chars available
    else
    {return buffer[readPos];}
}

/**
 * read data from linux to buffer
***/
void Process::doBuffer(void)
{
    // If there are already char in buffer exit
    if (buffered > 0)
    {return;}

    // Try to buffer up to 32 characters
    readPos = 0;
    uint8_t cmd[] = {'O', handle, sizeof(buffer)};
    buffered = bridge.transfer(cmd, 3, buffer, sizeof(buffer));
}

/**
 * start a linux named process. only the command should called hear.
 * followed by run() or runAsynchronously(), and optionally addParameter().
 * The named process does not start executing until run() is called
**/
void Process::begin(const String &command)
{
    close();
    cmdline = new String(command);
}

/**
 * addParameter() adds a parameter to a Linux command initiated with begin().
 * It's not necessary to add spaces, addParameter() inserts them for you.
**/
void Process::addParameter(const String &param)
{
    *cmdline += "\xFE";
    *cmdline += param;
}

/**
 * Starts a Linux process identified in Process.begin().
 * Unlike run(), runAsynchronously() is not-blocking.
 * Your sketch will continue to run while the Linux process runs in the background.
**/
void Process::runAsynchronously(void)
{
    uint8_t cmd[] = {'R'};
    uint8_t res[2];

    bridge.transfer(cmd, 1, (uint8_t*)cmdline->c_str(), cmdline->length(), res, 2);
    handle = res[1];

    delete cmdline;
    cmdline = NULL;

    if (res[0] == 0) // res[0] contains error code
    {started = true;}
}

/**
 * Checks a process started by runAsynchronously() to see if it is still running.
**/
boolean Process::running(void)
{
    uint8_t cmd[] = {'r', handle};
    uint8_t res[1];

    bridge.transfer(cmd, 2, res, 1);
    return (res[0] == 1);
}

/**
 * Returns the exit value (aka return code) of a process that was running.
 * Used by run() and runShellCommand() to return the status of the process that the user has launched.
**/
unsigned int Process::exitValue(void)
{
    uint8_t cmd[] = {'W', handle};
    uint8_t res[2];

    bridge.transfer(cmd, 2, res, 2);
    return (res[0] << 8) + res[1];
}

/**
 * Starts a Linux process identified in Process.begin().run() is a blocking function.
 * That is, once you call Process.run(), nothing else will happen in your sketch until it has completed. The time depends on the nature of the command you are executing.
 * For a non-blocking alternative, see runAsynchronously().
**/
unsigned int Process::run(void)
{
    runAsynchronously();
    while (running())
    {delay(100);}
    return exitValue();
}

/**
 * Closes a process started by runAsynchronously()
**/
void Process::close(void)
{
    if (started)
    {
        uint8_t cmd[] = {'w', handle};
        bridge.transfer(cmd, 2);
    }
    started = false;
}

/**
 * Starts a shell command in Linux. runShellCommand() is a blocking function
**/
unsigned int Process::runShellCommand(const String &command)
{
    runShellCommandAsynchronously(command);
    while (running())
    {delay(100);}
    return exitValue();
}

/**
 * Starts a Linux shell command.runShellCommandAsynchronously() is not-blocking
**/
void Process::runShellCommandAsynchronously(const String &command)
{
    begin("/bin/ash");
    addParameter("-c");
    addParameter(command);
    runAsynchronously();
}

bool Process::cleanProcess(int id)
{
    uint8_t cmd[] = {'w', (uint8_t)(id&0xFF)};
    uint8_t res[2];
    bridge.transfer(cmd, 2, res, 2);
}

