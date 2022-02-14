#ifndef ASSIGNEMNT3_LOGGER_LIB
#define ASSIGNEMNT3_LOGGER_LIB

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <semaphore.h>

typedef struct
{
    char* prefix;   // Name of the logger
    char* path;     // Absolute path for log file
    int fd;         // File descriptor of log file
}Logger;

// Utility function to create a named Logger.
// It creates and opens the log file specified by the path
// and fills the necessary parameters of the Logger.
int create_logger(Logger* logger, char* prefix, char* path);

// Prints the following: time | [prefix] text: perror string
// on the log file and on console in red
// and sends a SIGTERM to the caller process
void perror_exit(Logger* logger, char* text);

// Prints the following: time | [prefix] text: perror string
// on the log file and on console in red
// and returns the control to the caller
void perror_cont(Logger* logger, char* text);

// Prints the following: time | [prefix] text
// on the log file and on console in red
// and sends a SIGTERM to the caller process
void error_exit(Logger* logger, char* text);

// Prints the following: time | [prefix] text
// on the log file and on console in red
// and returns the control to the caller
void error_cont(Logger* logger, char* text);

// Prints the following: time | [prefix] text
// on the log file and also on console if console=1
void info(Logger* logger, char* text, int console);

#endif //ASSIGNEMNT3_LOGGER_LIBR
