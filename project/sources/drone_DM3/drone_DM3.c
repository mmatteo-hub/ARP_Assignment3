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
#include "./../drone_api/drone_api.h"

#define MAX_FUEL 100
#define MAP_ROW 40
#define MAP_COL 80

int socket_fd;
FILE *f;
time_t clk;
char map[MAP_ROW][MAP_COL];

void perror_exit(char* s);
void free_resources();
void refuel();
void take_off();
void landing();
void print_direction(int x, int y, int z);
void print_map();
void exit_routine();

int fuel, posX, posY, posZ;

int main(int argc, char *argv[])
{
    srand(time(NULL));
    // opening the log file in writing mode to create if it does not exist
    if((f = fopen(argv[3],"w")) == NULL)
    	perror_exit("Opening logfile");
    
 	// Opening socket
 	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
 	if((socket_fd) == -1)
 		perror_exit("Opening socket");
 	clk = time(NULL);
 	fprintf(f,"DM3: Socket opened at : \t\t%s", ctime(&clk));
        fflush(f);

        // Getting server hostname
 	struct hostent *server; 
	server = gethostbyname("127.0.0.1");
 	if (server == NULL)
 	{
 		perror_exit("Getting local host");
	}
 	struct sockaddr_in serv_addr;
 	
	// Setting server data and port number (51234)
	bzero((char *) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
 	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
 	serv_addr.sin_port = htons(51234); 

	// Making a connection request to the server
	if (connect(socket_fd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
 		perror_exit("Connection with server");
 	clk = time(NULL);
 	fprintf(f,"DM3: Socket established at : \t\t%s", ctime(&clk));
        fflush(f);
 
    //Initial position from arguments		
    posX = atoi(argv[1]);
    posY = atoi(argv[2]);
    posZ = 0;
    
    //Send position to the master
    int result = send_spawn_message(socket_fd, posX, posY, 0);
    printf("Spawn result is: %i\n", result);
    clk = time(NULL);
    fprintf(f,"DM3: Drone spawned at : \t\t%s", ctime(&clk));
    fflush(f);
    sleep(1);
    
    //Initialize fuel at maximum value
    fuel = MAX_FUEL;
    int moveX;
    int moveY;
    int moveZ;
    
    take_off();
    //Mark initial position on the map
    map[posY][posX] = 'x';
    
    while(1)
    {
    	// Time to move in the same direction (length of the movement)
        int t = rand() % 5 + 10;
        // The drone decide in which direction moving along X
        int moveX = rand() % 3 - 1;
        if(moveX == 0) //if it doesn't move along X, it has to move along Y
        	moveY = rand()%2 ? 1 : -1;
        else
        	moveY = rand() % 3 - 1;
        moveZ = 0;
        
        // With a lower priority the drone can choose to move up or down
        if(rand()%4 == 0)
        {
            moveX = 0;
            moveY = 0;
            moveZ = rand()%2 ? 1 : -1;
            t = rand()%3+2;
        }
        
        // Try to move in the choosen direction for the choosen time
        while(--t > 0)
        {
            int result = send_move_message(socket_fd, moveX, moveY, moveZ);
            print_direction(moveX, moveY, moveZ); 
            drone_error(result);
            if(result == CONNECTION_ABORTED || result == -1)
            	exit_routine();
           
            if(result == SUCCESS)
            {
            	// Mark new position on the map
            	posZ += moveZ;
            	posY += moveY;
            	posX += moveX;
            	map[posY][posX] = 'x';
            }
            
            if(result == OUT_OF_BOUNDS_POSITION)
            {
            	clk = time(NULL);
		fprintf(f,"DM3: Refused: 'OUT OF BOUNDS POSITION' at : \t%s", ctime(&clk));
		fflush(f);
		break;
            }
            if(result == OCCUPIED_POSITION_WALL)
            {
            	// Mark wall position on the map
            	map[posY + moveY][posX + moveX] = '*';
            	clk = time(NULL);
		fprintf(f,"DM3: Refused: 'WALL' at : \t\t%s", ctime(&clk));
		fflush(f);
		break;
            } 
            if(result == OCCUPIED_POSITION_DRONE)
            {
            	clk = time(NULL);
		fprintf(f,"DM3: Refused: 'DRONE' at : \t\t%s", ctime(&clk));
		fflush(f);
		break;
            } 
            
            --fuel;
            if(fuel <= posZ)
            	break;
            usleep(500000);
        }
        
        // If the fuel is finished the drone lands and refuels
        if(fuel <= posZ)
        {
            printf("\nREFUELING NEEDED\n");
            landing();
            send_land_message(socket_fd, 1);
            refuel();
            usleep(500000);
            send_land_message(socket_fd, 0);
            take_off();
        }
    }
    
    // Freeing resources on termination
    free_resources();
    
 	return 0;
}

void landing()
{
	printf("\nLanding\n");
	clk = time(NULL);
	fprintf(f,"DM3: Landing at : \t\t\t%s", ctime(&clk));
	fflush(f);
	for(int i=0; i<10; ++i)
            {
                int result = send_move_message(socket_fd, 0, 0, -1);
                print_direction(0,0,-1); drone_error(result);
                if(result == CONNECTION_ABORTED || result == -1)
            		exit_routine();
                --fuel;
                if(result == OUT_OF_BOUNDS_POSITION)
                	return;
                usleep(1000000);
            }
}

void take_off()
{
	printf("\nTaking off\n");
	clk = time(NULL);
	fprintf(f,"DM3: Taking off at : \t\t\t%s", ctime(&clk));
	fflush(f);
	posZ = 0;
	for(int i = 0; i < 4; i++)
	{
		int result = send_move_message(socket_fd, 0, 0, 1);
		print_direction(0,0,1); 
		drone_error(result);
		if(result == CONNECTION_ABORTED || result == -1)
            		exit_routine();
		if(result == 0)
			posZ += 1;
		--fuel;
		usleep(500000);
        }
        return;
}

void refuel()
{
	print_map();
	printf("\nRefueling...\n");
	clk = time(NULL);
	fprintf(f,"DM3: Refueling at : \t\t\t%s", ctime(&clk));
	fflush(f);
	while(fuel < MAX_FUEL)
	{
		fuel += 1;
		if(fuel%5 == 0)
		{
			printf("\rFuel: %i/%i", fuel, MAX_FUEL);
			fflush(stdout);
		}
		usleep(100000);
	}
	printf("\n");
	
	return;
}

void print_direction(int x, int y, int z)
{
	char* s;
	if(z == 1)
		s = "up";
	else if(z == -1)
		s = "down";
	else if(x == 0 && y == 1)
		s = "north";
	else if(x == 0 && y == -1)
		s = "south";
	else if(x == 1 && y == 0)
		s = "east";
	else if(x == -1 && y == 0)
		s = "west";
	else if(x == 1 && y == 1)
		s = "north-east";
	else if(x == -1 && y == 1)
		s = "north-west";
	else if(x == 1 && y == -1)
		s = "south-east";
	else if(x == -1 && y == -1)
		s = "south-west";
		
	printf("\nTrying to move %s\n", s);
	clk = time(NULL);
	if(s == "up")
	{
		fprintf(f,"DM3: Trying to move %s at : \t\t%s", s, ctime(&clk));
		fflush(f);
	}
	else{
		fprintf(f,"DM3: Trying to move %s at : \t%s", s, ctime(&clk));
		fflush(f);
	}
	return;
	
}

void print_map()
{
	printf("PRINT_MAP\n");
	fflush(stdout);
	for(int i = (MAP_ROW-1); i >= 0; i--)
	{
		for(int j = 0; j < (MAP_COL-1); j++)
		{
			if(map[i][j] != 'x' && map[i][j] != '*')
				map[i][j] = '.';
			fprintf(f,"%c", map[i][j]);
    			fflush(f);
    			printf("%c", map[i][j]);
    			fflush(stdout);
		}
		fprintf(f,"\n");
    		fflush(f);
    		printf("\n");
    		fflush(stdout);
	}
	return;
}

void perror_exit(char* s)
{
    free_resources();
    perror(s);
    clk = time(NULL);
    fprintf(f,"DM3: %s received at : \t%s", s, ctime(&clk));
    fflush(f);
    exit(0);
}

void exit_routine()
{
	print_map();
	free_resources();
	exit(0);
}

void free_resources()
{
    // Closing socket
    if(close(socket_fd) == -1)
        perror("Closing socket");
    clk = time(NULL);
    fprintf(f,"DM3: socket closed at : \t%s", ctime(&clk));
    fflush(f);
}
