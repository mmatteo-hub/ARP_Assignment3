#include "logger.h"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define WHITE   "\033[37m"

void info_hidden(Logger* logger, char* text, char* color, int console);

int create_logger(Logger* logger, char* prefix, char* path)
{
    // Creating log file
    int fd = open(path, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0666);
    if(fd == -1)
        return -1;
        
    // Getting absolute path for log file
    char* absolute_path = realpath(path, NULL);
    if(absolute_path == NULL)
        return -1;
        
    logger->prefix = prefix;
    logger->path = absolute_path;
    logger->fd = fd;
    
    return 0;
}

// Prints error from perror and makes process terminate
void perror_exit(Logger* logger, char* text)
{
    // Printing on log and console
    perror_cont(logger, text);
    
    // Terminating caller process
    kill(getpid(), SIGTERM);
}

// Prints error from perror and makes process continue
void perror_cont(Logger* logger, char* text)
{
    // Concatenating string to obtain error
    char* error = strerror(errno);
    char* middle = ": \0";
    
    char string[512] = "\0";
    strcat(string, text);
    strcat(string, middle);
    strcat(string, error);
    
    // Print error in log and console but continue executing
    error_cont(logger, string);
}

// Prints error in text and makes process terminate
void error_exit(Logger* logger, char* text)
{
    // Printing on log and console
    error_cont(logger, text);
    
    // Terminating caller process
    kill(getpid(), SIGTERM);
}

// Prints error in text and makes process continue
void error_cont(Logger* logger, char* text)
{
    info_hidden(logger, text, RED, 1);
}

void info(Logger* logger, char* text, int console)
{
    info_hidden(logger, text, WHITE, console);
}

void info_hidden(Logger* logger, char* text, char* color, int console)
{
    // Getting current time for log
    char now[9];
    strftime(now, 9, "%X", localtime(&(time_t){time(NULL)}));
    
    // Writing info on log.txt file
    if(write(logger->fd, now, 8) == -1 ||
       write(logger->fd, " | [", 4) == -1 ||
       write(logger->fd, logger->prefix, strlen(logger->prefix)) == -1 ||
       write(logger->fd, "] ", 2) == -1 ||
       write(logger->fd, text, strlen(text)) == -1 ||
       write(logger->fd, "\n", 1) == -1)
        perror("Writing string on log file");
    
    // Writing info on console
    if(console)
    {
        printf("\n%s[", color);
        printf("%s", logger->prefix);
        printf("] ");
        printf("%s", text);
        printf(RESET "\n");
        fflush(stdout);
    }
}
