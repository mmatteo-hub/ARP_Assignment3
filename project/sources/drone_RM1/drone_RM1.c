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
#include <math.h>
#include "./../drone_api/drone_api.h"

int fd_sock;

void error_exit(char* s);
void close_socket();
void write_log(char* file_path, char* msg);

int main(int argc, char *argv[])
{

  if (argc < 5){
    error_exit("\x1b[31m" "argv[] insufficient number of arguments provided\n" "\x1b[0m");
  }

  srand(time(NULL));

 	// Open the socket
 	if((fd_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
 		error_exit("\x1b[31m" "Error while opening the socket\n" "\x1b[0m");
  }

  // Get the host by name
 	struct hostent *server;
	server = gethostbyname("127.0.0.1");
 	if (server == NULL){
 		error_exit("\x1b[31m" "Error while getting the host by name\n" "\x1b[0m");
  }
  
	// Set the server data and the port number (51234)
  struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
 	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
 	serv_addr.sin_port = htons(51234);

	// Make a connection request to the server
	if (connect(fd_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
 	  error_exit("\x1b[31m" "Error while connecting to the server\n" "\x1b[0m");
  }

  // Initialize an istance of the struct Drone
  Drone my_drone;

  // Retrieve the spawn coordinates
  my_drone.posx = atoi(argv[1]);
  my_drone.posy = atoi(argv[2]);
  my_drone.posz = 0;

  // Retrieve the drone velocity
  int vel = atoi(argv[3]);

  // Limit drone's velocity in the interval 1-10
  if(vel < 1){
    vel = 1;
  }
  if(vel > 10){
    vel = 10;
  }

  // Define the time required to execute a step both when the drone moves and when it lands
  int step_move_time = floor(2000000/vel);
  int step_land_time = floor(3000000/vel);

  // Send spawning request to the master
  while(1){
    int result = send_spawn_message(fd_sock, my_drone.posx, my_drone.posy, my_drone.posz);
    if(result != SUCCESS){
        if(result == OUT_OF_BOUNDS_POSITION){
          printf("\x1b[31m"  "Cannot spawn the drone:" "\x1b[0m" " the requested position is outside the map\n");
          fflush(stdout);
        }
        else if (result == OCCUPIED_POSITION_WALL){
          printf("\x1b[31m"  "Cannot spawn the drone:" "\x1b[0m" " the requested position is occupied by a wall\n");
          fflush(stdout);
        }
        else if (result == OCCUPIED_POSITION_DRONE){
          printf("\x1b[31m"  "Cannot spawn the drone:" "\x1b[0m" " the requested position is occupied by a drone\n");
          fflush(stdout);
        }
        else{
          error_exit("\x1b[31m" "Unknown error while spawning the drone\n" "\x1b[0m");
        }

        printf("-> Recalculating the spawn position...\n\n");
        fflush(stdout);

        my_drone.posx = rand()%11 + 35; // random position along the x-axis in the "safe spawning zone"
        my_drone.posy = rand()%11 + 15; // random position along the y-axis in the "safe spawning zone"
        my_drone.posz = 0;
    }
    else{
      printf("\x1b[32m" "Drone successfully spawned\n\n" "\x1b[0m");
      fflush(stdout);
      break;
    }
  }
  sleep(1);

  // Get the relative path to reach the drone's log file
  char * log_path = argv[4];
  char msg[80];         // string where to save messages to be printed

  // Clear the log file used by the drone, if it doesn't exists it creates one
  //FILE * fp;
  fclose(fopen(log_path, "w+"));

  // Initialize the number of consequent moves that the drone can do before
  // running out of battery
  int battery = rand()%11 + 40;  // random number between 40 and 50 (battery)

  // Initialize half the battery and a quarter of the battery
  int half_battery = ceil(battery/2); // retrieve a half of the battery
  int quarter_battery = ceil(battery/4); // retrieve a quarter of the battery

  // Initialize variables to keep track of the current moving direction
  int steps = 0;
  int offx, offy, offz = 0;

  // Initialize variables to keep track of the previous moving direction
  int prev_offx, prev_offy, prev_offz = 0;

  // Variable to keep track of the consequent failures in choosing the moving direction
  int failures = 0;

  // Print on stdout
  printf("\x1b[32m" "Taking off...\n\n" "\x1b[0m");
  fflush(stdout);

  // Print on LOG file
  sprintf(msg, "Taking off...");
  write_log(log_path, msg);

  while(1)
  {
    // Flag to determine the execution of an "intelligent" navigation algorithm
    // to explore a plane
    int nav_alg = 1;

    // Flag to determine the execution of the landing procedure
    int landing = 0;

    // Implement the "intelligent" navigation algorithm to avoid the drone
    // from returning back from where it has just arrived
    while(nav_alg)
    {
      // Determine randomly a moving direction on the plane x-y
      steps = rand() % 6 + 5;
      offx = rand()%3 - 1;
      offy = rand()%3 - 1;
      offz = 0;

      // Every once in a while make the drone move along the z-axis
      if(rand()%3 == 0)
      {
        offz = rand()%2 ? 1 : -1;
        steps = rand()%4 + 2;
      }

      // If the determined direction is not (0,0,0) put the flag to 0 to eventually
      // exit from the while loop
      if(offx != 0 || offy != 0 || offz != 0)
        nav_alg = 0;

      // Continue looping and determining a new moving direction if the determined
      // direction is one of the three opposite directions to the previous one:

      // 1st case: previous moving direction along x-axis
      if(abs(prev_offx) == 1 && prev_offy == 0)
      {
        if(offx == -prev_offx)
          nav_alg = 1;
      }

      // 2nd case: previous moving direction along y-axis
      if(abs(prev_offy) == 1 && prev_offx == 0)
      {
        if(offy == -prev_offy)
          nav_alg = 1;
      }

      // 3rd case: previous moving direction along a diagonal
      if(abs(prev_offx) == 1 && abs(prev_offy) == 1)
      {
        if(offx == -prev_offx && offy == -prev_offy)
          nav_alg = 1;
        if(offx == 0 && offy == -prev_offy)
          nav_alg = 1;
        if(offx == -prev_offx && offy == 0)
          nav_alg = 1;
      }

      // If the drone is stuck do not care about the navigation algorithm and therefore exit
      // the while loop
      if(failures >= 15){
        nav_alg = 0;
      }
    }

    // Moving in the selected direction
    while(--steps > 0)
    {
      int result = send_move_message(fd_sock, offx, offy, offz);
      printf("Tried to move along [%i %i %i] direction: ", offx, offy, offz);
      drone_error(result);
      // If the drone cannot move successfully in the desired direction exit the
      // loop and increment the number of consequent failures
      if(result != SUCCESS)
      {
        failures++;
        break;
      }

      // Update the current position of the drone
      failures = 0;
      my_drone.posx += offx;
      my_drone.posy += offy;
      my_drone.posz += offz;

      // Save the last successful planar moving direction
      if(offx != 0 || offy != 0)
      {
        prev_offx = offx;
        prev_offy = offy;
      }

      // Decrease the number of moves available
      --battery;

      // Print on stdout
      printf("Current position [%i %i %i]\n", my_drone.posx, my_drone.posy, my_drone.posz);
      printf("Residual battery: %i \n\n", battery);
      fflush(stdout);

      // Print on LOG file
      sprintf(msg, "Moved along the [%i %i %i] direction", offx, offy, offz);
      write_log(log_path, msg);
      sprintf(msg, "Current position [%i %i %i]", my_drone.posx, my_drone.posy, my_drone.posz);
      write_log(log_path, msg);

      // Display battery information
      if(battery == half_battery)
      {
        // Print on stdout
        printf("\x1b[33m" "Half of the battery left\n\n" "\x1b[0m");
        fflush(stdout);

        // Print on the LOG file
        sprintf(msg, "Half of the battery left");
        write_log(log_path, msg);
      }

      if(battery == quarter_battery)
      {
        // Print on stdout
        printf("\x1b[33m" "A quarter of the battery left\n\n" "\x1b[0m");
        fflush(stdout);

        // Print on the LOG file
        sprintf(msg, "A quarter of the battery left");
        write_log(log_path, msg);
      }

      // Wait before taking the next step
      usleep(step_move_time);

      // If the current battery is only enough to guarantee the landing (and no
      // other move) before it runs out, exit and start the landing procedure.
      //
      // Notice that the condition battery == my_drone.posz is not sufficient,
      // since there might be situations in which it is not met.
      // For instance:
      //    step_i: battery = 9     --->    step_i+1: battery = 8
      //    step_i: pos_z = 8       --->    step_i+1: pos_z = 9
      // If the situation above happens, the drone won't be able to land safely
      if(battery - my_drone.posz <= 1)
      {
        // Trigger the landing procedure
        landing = 1;

        // Print on stdout
        printf("\x1b[31m" "Battery is running out! Starting the landing manoeuvre to recharge\n" "\x1b[0m");
        fflush(stdout);

        // Print on the LOG file
        sprintf(msg, "Battery is running out! Starting the landing manoeuvre to recharge");
        write_log(log_path, msg);

        break;
      }
    }

    // If the battery is low, execute the landing procedure
    if(landing)
    {
      printf("\n");
      while(my_drone.posz > 0)
      {
        int result = send_move_message(fd_sock, 0, 0, -1);
        printf("\x1b[34m" "Landing... " "\x1b[0m" "tried to move down: ");
        drone_error(result);
        if(result == SUCCESS){
          my_drone.posz--;
          battery--;
        }

        // Print on the LOG file
        sprintf(msg, "Landing...");
        write_log(log_path, msg);

        // Wait before taking the next step
        usleep(step_land_time);
      }

      // Notify the master that the drone landed
      send_land_message(fd_sock, 1);

      // Print on stdout
      printf("\x1b[33m" "Recharging battery...\n" "\x1b[0m");
      fflush(stdout);

      // Print on the LOG file
      sprintf(msg, "Recharging battery...");
      write_log(log_path, msg);

      // Randomly charge the drone and update both the half and the quarter of the battery
      battery = rand()%11 + 40;
      half_battery = ceil(battery/2);
      quarter_battery = ceil(battery/4);

      // Sleep a certain time required for recharging
      usleep(5000000);

      // Print on stdout
      printf("\x1b[32m" "Battery recharged! Taking off...\n\n" "\x1b[0m");
      fflush(stdout);

      // Print on the LOG file
      sprintf(msg, "Battery recharged! Taking off...");
      write_log(log_path, msg);

      // Notify the master that the drone is ready for taking off
      send_land_message(fd_sock, 0);
    }
  }

  // Close the socket and return (if the program works properly it will never arrive here)
  close_socket();
 	return 0;
}



void error_exit(char* s)
{
    // Close the socket
    close_socket();

    // Print the error message
    printf("%s", s);
    fflush(stdout);

    exit(-1);
}

void close_socket()
{
    // Close the socket
    if(close(fd_sock) == -1){
        printf("\x1b[31m" "Error while closing the socket\n" "\x1b[0m");
        fflush(stdout);
        exit(-1);
    }
}

void write_log(char* file_path, char* msg)
{
  // Pointer to the log file
  FILE *fp;

  // Get the current time for log
  char t[9];
  strftime(t, 9, "%X", localtime(&(time_t){time(NULL)}));

  fp = fopen(file_path, "a");
  fprintf(fp, "%s [DRONE_RM1] %s\n", t, msg);
  fclose(fp);
}
