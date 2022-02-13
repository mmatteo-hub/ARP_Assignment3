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
#include <semaphore.h>

typedef struct
{
    char* prefix;
    char* path;     // Absolute path for log file
    int fd;         // File descriptor of opened log file
}Logger;

// Utility function to create a logger
// It creates and opens the log file specified by the path
// and fills the necessary parameters of the logger.
int create_logger(Logger* logger, char* prefix, char* path);

// Prints the specified text error + the perror string
// on the log file and sends a SIGTERM to the caller process
void perror_exit(Logger* logger, char* text);

// Prints the specified text error concatenated to 
// the perror string on the log file
void perror_cont(Logger* logger, char* text);

// Prints the specified text error on the log file 
// and send a SIGTERM to the caller process
void error_exit(Logger* logger, char* text);

// Prints the specified text error on the log file
void error_cont(Logger* logger, char* text);

// Prints the specified text inside the log file 
// and also on console if console=1
void info(Logger* logger, char* text, int console);

#endif //ASSIGNEMNT3_LOGGER_LIBR
