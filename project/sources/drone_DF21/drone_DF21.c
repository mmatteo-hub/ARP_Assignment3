/*
    AUTHORS:
    *PARISI DAVIDE LEO: 4329668
    *FERRAZZI FRANCESCO: 5262829
*/
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
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "./../logger/logger.h"
#include "../drone_api/drone_api.h"


#define TIME 2000000
int socket_fd;
int user_vel;
int result;
int pos_drone_x;
int pos_drone_y;
int pos_drone_z;
int first_p;
int second_p;
int third_p;
int full_battery;
int time_to_wait;
char user_request;
bool drone_in_air = false;
bool unvalid_input = true;
bool uncorrect_velocity = true;
char str_msg[100];
//void perror_exit(char* s);
void free_resources();
void check_user_vel();
int take_off(int t, int pos_z);
int landing(int t, int pos_z);
void check_battery_status(int n);

//Open a file pointer for writing
FILE *logfile;

//define a logger
Logger logger = {"", "", -1};

// initialize variables for time informations for log file
time_t rawtime;
struct tm * timeinfo;

int main(int argc, char *argv[])
{
    // wanted path "./../logfile/logfile_drone_DF21.txt"
    srand(time(NULL));
    int pid_process = getpid();

 	// Opening socket
 	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
 		perror_exit(&logger, "Opening socket");

    // Getting server hostname
 	struct hostent *server; 
	server = gethostbyname("127.0.0.1");
 	if (server == NULL)
 		perror_exit(&logger, "Getting local host");
	
 	struct sockaddr_in serv_addr;
 	
	// Setting server data and port number (51234)
	bzero((char *) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
 	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
 	serv_addr.sin_port = htons(51234); 

	// Making a connection request to the server
	if (connect(socket_fd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
 		perror_exit(&logger, "Connection with server");

    // Getting spawn position from user as arguments
    int spawnx = atoi(argv[1]);
    int spawny = atoi(argv[2]);
    // Getting logfile path from user as arguments
    char *log_path = argv[3];

    //open logfile
    create_logger (&logger, "logfile_drone_DF21", log_path);

    sprintf(str_msg, "Correctly connected with master");
    info(&logger, str_msg, 0);

    sprintf(str_msg, "Spawn psoition: [x = %d|y= %d| z= 0]", spawnx, spawny);
    info(&logger, str_msg, 0);
    // Getting drone velocity from user
    while(uncorrect_velocity)
    {
        check_user_vel();
    }
    sprintf(str_msg, "Velocity set by user: %d", user_vel);
    info(&logger, str_msg, 0);
    
    // Defining drone position at each instant
    int pos_drone_x = spawnx;
    int pos_drone_y = spawny;
    int pos_drone_z = 0;
    // Setting robot velocity by changing the sleep time
    int time_to_wait = TIME / user_vel;

    // Feedback for spawn
    int result = send_spawn_message(socket_fd, spawnx, spawny, 0);
    if(result==CONNECTION_ABORTED || result == -1)
    {
        free_resources();
        return 0;
    }
    printf("Spawn result is: %i\n", result);
    sprintf(str_msg, "Spawn result is: %i", result);
    info(&logger, str_msg, 0);

    sleep(1);
    
    // Set how many steps the drone should do before landing and refueling
    int steps_bef_landing = rand() % (110 + 1 - 90) + 90;
    // Devide battery in four percentiles to allow user to have a feedback on the state of charge
    int battery_percentage_steps = round(steps_bef_landing / 4);
    first_p = battery_percentage_steps;
    second_p = 2 * battery_percentage_steps;
    third_p = 3 * battery_percentage_steps;
    full_battery = steps_bef_landing;
    int battery_full = 100;
    int battery_percentile = 25;
    int battery_status;
    printf("\nDrone is taking off for %i steps before refueling\n\n", steps_bef_landing);
    fflush(stdout);
    sprintf(str_msg, "Drone is taking off for %i spetps before refueling", steps_bef_landing);
    info(&logger, str_msg, 0);
    // Make the robot take off 
    pos_drone_z = take_off(time_to_wait, pos_drone_z);
    
    while(1)
    {
        // Setting drone trajectory for n steps moving on a plane
        int trajectory_steps = rand() % 6 + 3;
        int offx = rand() % 3 - 1;
        int offy = rand() % 3 - 1;
        int offz = 0;
        
        // Rare cases drone moves along z axis and reset trajectory n steps 
        if(rand() % 5 == 0)
        {
            offx = 0;
            offy = 0;
            offz = rand() % 2 ? 1 : -1;
            trajectory_steps = rand() % (5 + 1 -1) + 1;
        }
        printf("\e[1;1H\e[2J");
        printf("Remaining steps before landing: %i\n", steps_bef_landing);
        printf("Trajectory steps: %i\n\n", trajectory_steps);
        fflush(stdout);

        // Move for previously specified trajectory n steps
        while(trajectory_steps > 0 && drone_in_air)
        {
            // If drone is on air in can not go lower than 1 in z axis to avoid collisions with the ground
            if (pos_drone_z <= 1 && drone_in_air && offz < 0)
                offz = 0;

            // Send move message and get the feedback if the drone can or cannot move in the desired direction
            int result = send_move_message(socket_fd, offx, offy, offz);
            if(result==CONNECTION_ABORTED || result == -1)
            {
                free_resources();
                return 0;
            }
            printf("Tried to move %i %i %i : ", offx, offy, offz); drone_error(result);
            fflush(stdout);
            sprintf(str_msg, "Drone is taking off for %i spetps before refueling", steps_bef_landing);
            info(&logger, str_msg, 0);
            // Update the drone position
            pos_drone_x = pos_drone_x + offx;
            pos_drone_y = pos_drone_y + offy;
            pos_drone_z = pos_drone_z + offz;
            // Position of the drone along z can't go higher than 9 or lower than 0
            if(pos_drone_z >= 9)
                pos_drone_z = 9;
            if(pos_drone_z <= 0)
                pos_drone_z = 0;
            // Avoid deadlocks and reset the trajectory
            // Since we are in 3D case, deadlock are very rare
            // What we do is simply set the trajectory to 0 so the drone will decide another random path when move message feedback is not SUCCESS
            if(result != 0)
            {
                trajectory_steps = 0;
                printf("OSTACLE ENCOUNTERED: choose another trajectory\n");
                fflush(stdout);
            }
            printf("Drone position [ x:%i | y:%i | z:%i ]\n\n", pos_drone_x, pos_drone_y, pos_drone_z);
            fflush(stdout);
            sprintf(str_msg, "Drone position [ x:%i | y:%i | z:%i ]", pos_drone_x, pos_drone_y, pos_drone_z);
            info(&logger, str_msg, 0);
            // Check and print the battery status
            check_battery_status(steps_bef_landing);
            // Decrease steps regarding the end of the trajectory and before landing
            trajectory_steps --;
            steps_bef_landing --;
            usleep(time_to_wait);
        }
        
        // Drone needs to land and recharge its batteries
        if(steps_bef_landing <= 0 && drone_in_air)
        {
            sprintf(str_msg, "Battery expired, drone is landing...");
            info(&logger, str_msg, 0);
            // Drone is landing
            pos_drone_z = landing(time_to_wait, pos_drone_z);
            // Drone is not in air anymore
            drone_in_air = false;
            // Send to master that drone has landed
            send_land_message(socket_fd, 1);
            printf("\e[1;1H\e[2J");
            printf("Drone has landed\n");
            sprintf(str_msg, "Drone has landed");
            info(&logger, str_msg, 0);
            sleep(1);

            // Drone charges the batteries
            printf("Drone is recharging...\n");
            
            sprintf(str_msg, "Drone is recharging");
            info(&logger, str_msg, 0);
            battery_status = 0;
            printf("battery status: \n");
            fflush(stdout);
            sprintf(str_msg, "battery status: ");
            info(&logger, str_msg, 0);
            printf("[|");
            fflush(stdout);
            
            for (int i=1; i<=11; i++)
            {
                printf(" %d%% |", battery_status);
                fflush(stdout);
                sprintf(str_msg, "%d%%", battery_status);
                info(&logger, str_msg, 0);
                battery_status += 10;
                sleep(1);
            }
            printf("]\n\n");
            fflush(stdout);
            printf("Drone is full charged\n");
            sprintf(str_msg, "Drone is full charged");
            info(&logger, str_msg, 0);
            // Resetting steps before landing after the drone is fully charged
            steps_bef_landing = rand() % (110 + 1 - 90) + 90;
            // Devide battery in four percentiles to allow user to have a feedback on the state of charge
            battery_percentage_steps = round(steps_bef_landing / 4);
            first_p = battery_percentage_steps;
            second_p = 2 * battery_percentage_steps;
            third_p = 3 * battery_percentage_steps;
            // Send to master that the drone is ready to take off
            send_land_message(socket_fd, 0);
        }

        // Drone is not flying and waits for user's command
        while(!drone_in_air)
        {

            unvalid_input = true;
            // Ask user if he wants to change velocity, keep the previously assigned velocity or quit the program
            while(unvalid_input)
            {
                printf("Do you want to change the drone's velocity? [y/n]\n\n[q] to quit\n");
                scanf(" %c", &user_request);
                // User wants to change velocity
                if (user_request == 'y' || user_request == 'Y')
                {
                    sprintf(str_msg, "The user wants to change velocity");
                    info(&logger, str_msg, 0);
                    uncorrect_velocity = true;
                    while(uncorrect_velocity)
                    {
                        check_user_vel();
                    }
                    sprintf(str_msg, "Velocity set by user: %d", user_vel);
                    info(&logger, str_msg, 0);
                    // Setting the drone velocity by changing the sleep time
                    time_to_wait = TIME / user_vel;
                    printf("\nDrone is ready to TAKE OFF\n");
                    sprintf(str_msg, "Drone is ready to TAKE OFF");
                    info(&logger, str_msg, 0);
                    // Make the drone take off 
                    pos_drone_z = take_off(time_to_wait, pos_drone_z);
                    sprintf(str_msg, "Drone took off");
                    info(&logger, str_msg, 0);
                    // Drone is flying
                    drone_in_air = true;
                    unvalid_input = false;
                }
                // User does not want to change velocity
                else if (user_request == 'n' || user_request == 'N')
                {
                    sprintf(str_msg, "The user does not wants to change velocity");
                    info(&logger, str_msg, 0);
                    sprintf(str_msg, "Velocity set by user: %d", user_vel);
                    info(&logger, str_msg, 0);
                    printf("\nDrone is ready to TAKE OFF\n");
                    sprintf(str_msg, "Drone is ready to TAKE OFF");
                    info(&logger, str_msg, 0);
                    // Make the robot take off 
                    pos_drone_z = take_off(time_to_wait, pos_drone_z);
                    sprintf(str_msg, "Drone took off");
                    info(&logger, str_msg, 0);
                    // Drone is flying
                    drone_in_air = true;
                    unvalid_input = false;
                }
                // User wants to quit the program
                else if (user_request == 'q' || user_request == 'Q')
                {
                    printf("\e[1;1H\e[2J");
                    printf("Program is closing\n");
                    fflush(stdout);
                    free_resources();
                    sprintf(str_msg, "The user killed the program, program exiting...");
                    info(&logger, str_msg, 0);
                    sleep(1);
                    kill(pid_process, SIGKILL);
                }
                // Command not valid
                else
                {
                    printf("Command not valid, please insert:\n[y] to change velocity and take off\n[n] to take off\n[q] to exit");
                }
            
            }
        }
    }
    // Freeing resources on termination
    free_resources();
 	return 0;
}


// Closing communication channel
void free_resources()
{
    // Closing socket
    if(close(socket_fd) == -1)
        perror("Closing socket");
}


// Function called when we need to set the new robot velocity
void check_user_vel()
{
    printf("Please, set the velocity of the drone pressing a number from 1 to 10\n10 is very fast ---- 1 is very slow\n\n");
    scanf("%d", &user_vel);
    if (user_vel >= 1 || user_vel <= 10)
    {
        uncorrect_velocity = false;
    }
    else
    {
        printf("Please, insert integer number that goes from 1 to 10\n10 is very fast ---- 1 is very slow\n");
    }
}


// Function used to allow the drone to take off
int take_off(int t, int pos_z)
{
	int take_off = rand() % (9 + 1 - 1) + 1;
	for (int i = 0; i < take_off; i++)
	{
		int result = send_move_message(socket_fd, 0, 0, +1);
        if(result==CONNECTION_ABORTED || result == -1)
        {
            free_resources();
            return 0;
        }
		pos_z ++;
		printf("Taking off: "); drone_error(result);
		if(result == OUT_OF_BOUNDS_POSITION)
			break;
		usleep(t);
	}
	drone_in_air = true;
    return pos_z;
}


// Function used to allow the drone to land on the ground
int landing(int t, int pos_z)
{
	for(int i=0; i<10; ++i)
	{
		int result = send_move_message(socket_fd, 0, 0, -1);
        if(result==CONNECTION_ABORTED || result == -1)
        {
            free_resources();
            return 0;
        }
        pos_z --;
		printf("Tried to move down: "); drone_error(result);
		if(result == OUT_OF_BOUNDS_POSITION)
			break;
		usleep(t);
	}
	drone_in_air = false;
    return pos_z;
}

// Print the battery status while drone is flying
void check_battery_status(int n)
{
    if (n == full_battery)
    {
        printf("battery status: \n");
        printf("[|   25%%   |   50%%   |   75%%   |   100%%   |]\n\n");
        fflush(stdout);
        sprintf(str_msg, "battery status: ");
        info(&logger, str_msg, 0);
        sprintf(str_msg, "[|   25%%   |   50%%   |   75%%   |   100%%   |]");
        info(&logger, str_msg, 0);
    }
    else if (n == third_p)
    {
        printf("battery status: \n");
        printf("[|   25%%   |   50%%   |   75%%   |\n\n");
        fflush(stdout);
        sprintf(str_msg, "battery status: ");
        info(&logger, str_msg, 0);
        sprintf(str_msg, "[|   25%%   |   50%%   |   75%%   |]");
        info(&logger, str_msg, 0);
    }
    else if (n == second_p)
    {
        printf("battery status: \n");
        printf("[|   25%%   |   50%%   |\n\n");
        fflush(stdout);
        sprintf(str_msg, "battery status: ");
        info(&logger, str_msg, 0);
        sprintf(str_msg, "[|   25%%   |   50%%   |]");
        info(&logger, str_msg, 0);
    }
    else if (n == first_p)
    {
        printf("battery status: \n");
        printf("[|   25%%   |\n\n");
        fflush(stdout);
        sprintf(str_msg, "battery status: ");
        info(&logger, str_msg, 0);
        sprintf(str_msg, "[|   25%%   |]");
        info(&logger, str_msg, 0);
    }
    else if (n == 0)
    {
        printf("battery status: \n");
        printf("Battery of the drone is low, LANDING SOON\n\n");
        fflush(stdout);
        sprintf(str_msg, "battery status: ");
        info(&logger, str_msg, 0);
        sprintf(str_msg, "Battery of the drone is low, LANDING SOON");
        info(&logger, str_msg, 0);
    }
}