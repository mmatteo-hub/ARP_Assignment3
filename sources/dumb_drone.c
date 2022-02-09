#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include "./drone_api/drone_api.h"

int socket_fd;

void perror_exit(char* s);
void free_resources();

int main(int argc, char *argv[])
{
    srand(time(NULL));
    
 	// Opening socket
 	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
 		perror_exit("Opening socket");

    // Getting server hostname
 	struct hostent *server; 
	server = gethostbyname("127.0.0.1");
 	if (server == NULL)
 		perror_exit("Getting local host");
	
 	struct sockaddr_in serv_addr;
 	
	// Setting server data and port number (51234)
	bzero((char *) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
 	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
 	serv_addr.sin_port = htons(51234); 

	// Making a connection request to the server
	if (connect(socket_fd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
 		perror_exit("Connection with server");
 		
    int spawnx = atoi(argv[1]);
    int spawny = atoi(argv[2]);
    
    int result = send_spawn_message(socket_fd, spawnx, spawny, 0);
    printf("Spawn result is: %i\n", result);
    sleep(1);
    
    int moves = rand()%30+20;
    
    while(1)
    {
        int movement = rand() % 5 + 5;
        int offx = rand() % 3 - 1;
        int offy = rand() % 3 - 1;
        int offz = 0;
        
        if(rand()%5 == 0)
        {
            offx = 0;
            offy = 0;
            offz = rand()%2 ? 1 : -1;
            movement = rand()%3+2;
        }
        
        while(--movement > 0)
        {
            int result = send_move_message(socket_fd, offx, offy, offz);
            printf("Tried to move %i %i %i : ", offx, offy, offz); drone_error(result);
            --moves;
            usleep(500000);
        }
        
        if(moves <= 0)
        {
            for(int i=0; i<10; ++i)
            {
                int result = send_move_message(socket_fd, 0, 0, -1);
                printf("Tried to move down: "); drone_error(result);
                if(result == OUT_OF_BOUNDS_POSITION)
                    break;
                usleep(1000000);
            }
            
            send_landing_message(socket_fd, 1);
            moves = rand()%30+20;
            usleep(500000);
            send_landing_message(socket_fd, 0);
        }
    }
    
    // Freeing resources on termination
    free_resources();
    
 	return 0;
}

void perror_exit(char* s)
{
    free_resources();
    perror(s);
    exit(0);
}

void free_resources()
{
    // Closing socket
    if(close(socket_fd) == -1)
        perror("Closing socket");
}
