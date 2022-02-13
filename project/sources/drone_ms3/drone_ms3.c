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
#include "./../logger/logger.h"

// define the max error on the position of the drone
#define max_err 1
#define max_inc 2
#define max_step 10
#define min_step 2
#define battery 20
#define max_vel 3
#define min_vel 1

// define rows and columns number
#define R_MAP 80
#define C_MAP 40

// define the colours for the battery status
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMGN  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

// define a buffer to write string
char buff [50];

// define socket fd
int socket_fd;

// define a logger struct
Logger logger = {"", "", -1};

// define function to free resources
void free_resources();

// function to print the map
void print_array();

// define an 2D array to count the number of times a cell has been visited (in the plane xy)
char map[R_MAP][C_MAP];

// sig handles
void err_sig (int signo)
{
    if(signo == SIGTERM)
    {
        free_resources();
        exit(1);
    }
}

// function to compute the position
double randomPos()
{
    return (((double)rand()) / ((double)RAND_MAX)) * (double)max_inc;
}

// function to compute the step number
double randomStep()
{
    return (((double)rand()) / ((double)RAND_MAX)) * (double)max_step + min_step;
}

// function to compute the velocity
double randomVel()
{
    return (((double)rand()) / ((double)RAND_MAX)) * (double)max_vel + min_vel;
}

// sign for the position increment/decrement
int sign()
{
    float n = (double)rand() / (double)RAND_MAX;
    if(n >  0.5) return 1;
    else return 0;
}

// main
int main(int argc, char *argv[])
{
    // take the random time
    srand(time(NULL));
    // create the logger specifying the name and the path 
    create_logger(&logger, "drone_ms3", "./sources/logfile/log_drone_ms3.txt");
    // call sig handler
    signal(SIGTERM, err_sig);
    // writing into the log file
    info(&logger, "Starting", 0);

    // define the current position of the drone
    int curr_x;
    int curr_y;
    int curr_z;

 	// Opening socket
 	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
 		perror_exit(&logger,"Opening socket");

    // Getting server hostname
 	struct hostent *server;
    // specify the IP address
	server = gethostbyname("127.0.0.1");
 	if (server == NULL)
 		perror_exit(&logger, "Getting local host");
	
    // define struct
 	struct sockaddr_in serv_addr;
 	
	// Setting server data and port number (51234)
	bzero((char *) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
 	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
 	serv_addr.sin_port = htons(51234); 

	// Making a connection request to the server
	if (connect(socket_fd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
 		perror_exit(&logger, "Connection with server");
 	
    // store the spawn x and y position taken as argument
    int spawnx = atoi(argv[1]);
    int spawny = atoi(argv[2]);

    // update the current position of the drone
    curr_x = spawnx;
    curr_y = spawny;
    curr_z = 0;

    // set the spawn position as visited
    map[curr_x][curr_y] = 'V';
    // print the map
    print_array(map);
    
    // check the result of the spawn position
    int result = send_spawn_message(socket_fd, spawnx, spawny, 0);
    // print on the console
    printf("Spawn result is: %i\n", result);
    // write into the buffer
    sprintf(buff, "Spawn position: [x = %d, y = %d, z = 0]", spawnx, spawny);
    // write into the log file
    info(&logger, buff, 0);
    sleep(1);
    
    // initialise the maximum amount of battery
    int bat = battery;

    // initialise the direction
    int direction_x = 0;
    int direction_y = 0;
    int direction_z = 0;
    
    // infinite loop
    while(1)
    {
        // compute randomly the amount of steps to do
        int steps = randomStep();
        // compute randomly the sign of the xyz axis
        int sx = sign();
        int sy = sign();
        int sz = sign();
        // compute randomly the velocity
        int vel = randomVel();

        // choose the correct velocity sign according to the random computed
        if(sx)
        {
            direction_x = randomPos();
        } 
        else
        {
            direction_x = -randomPos();
        }

        if(sy)
        {
            direction_y = randomPos();
        } 
        else
        {
            direction_y = -randomPos();
        }
        
        if(sz)
        {
            direction_z = randomPos();
        } 
        else
        {
            direction_z = -randomPos();
        }
        
        // loop to compute the movement for the amount of steps computed
        while(--steps > 0)
        {
            // clear the console
            system("clear");
            // check the direction is available
            int result = send_move_message(socket_fd, direction_x, direction_y, direction_z);
            // put into the buffer the direction
            sprintf(buff, "Direction computed = [Vx = %i, Vy = %i, Vz = %i].", direction_x, direction_y, direction_z);
            // write into the log file
            info(&logger, buff, 1);
            // check the error
            drone_error(result);
            printf("\n");
        
            // if the drone can move
            if(result != SUCCESS)
            {
                // check if it is a wall
                if(result == OCCUPIED_POSITION_WALL)
                {
                   map[curr_x + direction_x][curr_y + direction_y] = 'X';
                }

                printf("%s",KRED);
                // print into the log file
                info(&logger, "Cannot move due to a wall.", 1);
                printf("%s",KNRM);
                // print the map
                print_array(map);
                break;
            }

            // update the position in which the drone is as visited
            map[curr_x][curr_y] = 'V';

            // compute the current position of the drone
            curr_x += direction_x;
            curr_y += direction_y;
            curr_z += direction_z;
            
            // Put into the buffer the infos about the position
            sprintf(buff, "Current position = [%d, %d, %d].", curr_x, curr_y, curr_z);
            
            // print on the log file the current position of the drone
            info(&logger, buff, 0);

            // update the position in which the drone is as visited
            map[curr_x][curr_y] = 'P';

            // use battery
            --bat;

            // check battery is sufficient for landing
            if(bat > direction_z)
            {
                // print on the console
                printf("Battery remaining: %s %d %% %s\n",KGRN, 100*bat/battery, KNRM);
                // write inside the buffer
                sprintf(buff, "Battery remaining: %d %%", 100*bat/battery);
                // write into the log file
                info(&logger, buff, 0);
            }
            printf("\n");
            // print the map
            print_array(map);
            // sleep proportionally to the velocity of the drone
            usleep(500000/vel);
        
            // check the amount of battery remaining to be sure that the drone can land 
            if(bat <= direction_z)
            {
                // loop of 10 steps to recharge
                for(int i=0; i<10; ++i)
                {
                    // clear the output
                    system("clear");
                    // check the result of the move message
                    int result = send_move_message(socket_fd, 0, 0, -1);
                    // print on the console
                    printf("Landing: ");
                    drone_error(result);
                    // write into the log file
                    info(&logger, "Landing due to battery charge.", 0);
                    // use battery
                    --bat;
                    // print on the console
                    printf("Battery remaining: %s0 %% %s", KRED, KNRM);
                    printf("\n\n");
                    // print the map
                    print_array(map);
                    // check the landing is successfull
                    if(result == OUT_OF_BOUNDS_POSITION)
                        break;
                    // sleep
                    usleep(250000);
                }
                
                // send a message to the socket
                send_land_message(socket_fd, 1);
                // for the maximum amount of the battery
                for(bat = 0; bat < battery; bat++)
                {
                    // clear output
                    system("clear");
                    // print on the console
                    printf("Recharging: %s %d %% %s", KYEL, 100*bat/battery, KNRM);
                    // write on the buffer
                    sprintf(buff, "Recharging: %d %%", 100*bat/battery);
                    // write into the log file
                    info(&logger, buff, 0);
                    printf("\n\n");
                    // print the map
                    print_array(map);
                    // sleep
                    usleep(250000);
                }
                // print a message to the socket
                send_land_message(socket_fd, 0);
            }
        }
    }
    
    // Freeing resources on termination
    free_resources();

    // print on the log file
    info(&logger, "The drone has been stopped.",0);
    
    // exit
 	return 0;
}

// implementation of the function to print the map
void print_array()
{
    // loop on the columns of the array
    for(int i = C_MAP - 1; i >= 0; i--)
    {
        // loop on the rows of the array
        for(int j = 0; j < R_MAP - 1; j++)
        {
            // take the value (j,i)
            char v = map[j][i];
            if(v == 'P')
            {
                // print the position in blu
                printf("%sP%s",KBLU,KNRM);
            }
            else if(v == 'V')
            {
                // print the visited position in green
                printf("%sV%s",KGRN,KNRM);
            }
            else if(v == 'X')
            {
                // print the wall in white
                printf("%sX%s",KNRM,KNRM);
            }
            else
            {
                // print the unvisited in yellow
                printf("%sU%s",KYEL,KNRM);
            }
        }
        // print in a new line to perform the map shape
        printf("\n");
    }
}

// implementation of the function to free the resources
void free_resources()
{
    // Closing socket
    if(close(socket_fd) == -1)
        perror_cont(&logger, "Closing socket");
}