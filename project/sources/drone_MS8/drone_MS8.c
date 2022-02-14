#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "./../drone_api/drone_api.h"

#define BUFFSIZE 80

//ASCII ESCAPES
#define RED   "\x1B[31m"
#define BLU "\e[0;34m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define CYN   "\x1B[36m"
#define BLUB "\e[0;104m"
#define CYNB "\e[46m"
#define MAGHB "\e[0;105m"

#define BLKB "\e[40m"
#define RESET "\x1B[0m"
#define CLEAR "\e[1;1H\e[2J"

//port number of the socket server
#define PORT_NUMBER 51234 

//battery capacity of the drone
#define BATTERY_CAPACITY 420

//map dimensions
#define MAPX 80
#define MAPY 40

//information strings for printing
char position_string[BUFFSIZE];
char movement_string [BUFFSIZE];
char battery_string [BUFFSIZE];
char status_string [BUFFSIZE];
char charge_string[BUFFSIZE];

//file descriptor of the socket
int sockfd;

//file for logging
FILE *log_file;

//declaring functions
void error(char *msg);
void connect_to_master();
void get_input();
void spawn_drone();
char *decode_result(int result);
int move_drone(int x_shift,int y_shift, int z_shift);
void refresh_position(int x_shift, int y_shift, int z_shift);
void take_off(int height);
void land();
void charge_battery();
void update_screen();
void exit_routine(int signo);

//matrix to print xyz direction verbose
char *verb[3][3] = {{" left", "", " right"},
                    {" backward", "", " forward"},
                    {" down", "", " up"}};

//variables to spawn and keep trace of the position
int x_pos, y_pos, z_pos;
int x_spawn, y_spawn;

//char array to represent the drone's internal map
char map [MAPY] [MAPX];
int x,y,k;

//battery of the drone
float battery = 0;

//speed of the drone (steps per second)
int speed = 0;

//boolean to check if master has terminated
int master_terminated = 0;

int main(int argc, char *argv[])
{
    //registering signal for program termination
    struct sigaction sa1;
    memset(&sa1, 0 , sizeof(sa1));
    sa1.sa_handler = &exit_routine;
    sigaction(SIGINT, &sa1, NULL);
    sigaction(SIGTERM, &sa1, NULL);

    //emptying out all strings and char arrays
    for(y=0;y<MAPY;y++){        
        for(x=0;x<MAPX;x++){
            map[y][x]=' ';
        }
    }

    sprintf(position_string," ");
    sprintf(movement_string," ");
    sprintf(battery_string," ");
    sprintf(charge_string," ");
    sprintf(status_string," ");

    //opening log file
    log_file = fopen("./sources/logfile/log_drone_MS8.txt", "w");    
    if (log_file == NULL)
        error("[MS8] ERROR while opening logfile");

    //spawning the robot
    //if there is not already, make user input coordinates and speed
    if(argc!=4)
        get_input();
    else{
        x_spawn = atoi(argv[1]);
        y_spawn = atoi(argv[2]);
        speed = atoi(argv[3]);
    }

    //opening the socket and connecting to the master (server)
    connect_to_master();

    //spawning the drone
    spawn_drone();

    //initializating position
    x_pos = x_spawn;
    y_pos = y_spawn;
    z_pos = 0;

    //driving variables
    srand(time(NULL));
    int x_shift, y_shift, z_shift, n_steps;

    //charge the battery before taking off
    charge_battery();
    
    //taking off
    take_off(rand()%4+1);

    while(1)
    {
        //randomly choose number of steps and direction, avoiding to get x=0, y=0, z=0
        n_steps = rand() % 30 + 10;

        //chosing direction in 3D space
        do {
            x_shift = rand() % 3 - 1;
            y_shift = rand() % 3 - 1;
            z_shift = rand() % 3 - 1;
        } while (x_shift == 0 && y_shift == 0 && z_shift == 0);
        
        int i = 0;

        //repeat the movement in the choosen direction for n_steps
        while(i < n_steps) {
            
            //shift z only for 3 steps max
            if (z_shift != 0 && i > 3)
                z_shift = 0;

            //avoid touching the floor and the ceiling
            if(((z_shift == -1 && z_pos == 1) || (z_shift == 1 && z_pos == 9))
                && (x_shift != 0 || y_shift != 0))
                break;

            //move the drone in the desired direction
            if (move_drone(x_shift, y_shift, z_shift) == 0)
                break;

            i++;

            //if battery is low, exit
            if(battery < z_pos+1)
                break;
    

        }// end of n_steps while loop
        
        //land to recharge when remains only the battery to land
        //take off after battery charge
        if(battery < z_pos+1) {

            sprintf(status_string, RED " ==   BATTERY LOW !!   Emergency landing   == " RESET);
            fprintf(log_file, "\n ==   BATTERY LOW !!   Emergency landing   == \n");

            land();
            
            charge_battery();
            
            take_off(rand()%4+1);

        }
    }//end of infinite loop
    
    //close the socket connection 
    if(close(sockfd) == -1)
        perror("[MS8] ERROR while closing socket");
    
 	return 0;

}//end of main

//function to charge the battery
void charge_battery(){
    sprintf(status_string,CYN "    .. Charging battery ..." RESET);
    fprintf(log_file, "\n       .. Charging battery ...\n");
    sprintf(battery_string," ");
    for(k=0;k<BUFFSIZE;k++)
        charge_string[k]=' ';

    int i=0;
    charge_string[0]='[';
    charge_string[53]=']';
    while (battery<BATTERY_CAPACITY)
    {
       
        battery += 1;
        if((int)(battery)%8==0){
            i+=1;
            charge_string[i]='|';
            update_screen();
        }   

        usleep(15000);
    }


    sprintf(status_string, CYN "     ==   BATTERY FULL   ==" RESET);
    update_screen();
    usleep(500000);
    sprintf(charge_string," ");
    fprintf(log_file, "\n    ==   BATTERY FULL   ==\n\n");
}

//function to land correctly
void land(){
    

    update_screen();
    usleep(20000);

    while(height-- > 0){
        move_drone(0,0,-1);
        usleep(1000000/speed);
    }
    
    sprintf(status_string, CYN "     Landed succesfully " RESET);
    fprintf(log_file, "\n     Landed succesfully \n");
    sprintf(movement_string," ");
    update_screen();
    usleep(800000);
    
    if (send_land_message(sockfd, 1) == CONNECTION_ABORTED) {
        master_terminated = 1;
        exit_routine(SIGTERM);
    }

}

//function to take off with a height specified in the input parameter
void take_off(int height){

    if (send_land_message(sockfd, 0) == CONNECTION_ABORTED){
        master_terminated = 1;
        exit_routine(SIGTERM);
    }

    sprintf(status_string,CYN " TAKING OFF  ...." RESET);
    fprintf(log_file, "\n\n TAKING OFF  ....\n\n");
    usleep(1500000);

    while(height > 0){
        if (move_drone(0,0,1) == 1)
            height--;
            usleep(1000000/speed);
    }

    fprintf(log_file,"\n\n TAKE OFF COMPLETED\n\n");
    sprintf(status_string,CYN " TAKE OFF COMPLETED " RESET);
    usleep(800000);

    sprintf(status_string," ");

}

//function to move the drone in the space with a shift specified in the input parameters
//returns 1 in case of success, 0 in case of no moving
int move_drone(int x_shift, int y_shift, int z_shift){

    int result = send_move_message(sockfd, x_shift, y_shift, z_shift);
    
    if (result == CONNECTION_ABORTED){
        master_terminated = 1;
        exit_routine(SIGTERM);
    }

    if(result == OCCUPIED_POSITION_WALL || result == OUT_OF_BOUNDS_POSITION || result == OCCUPIED_POSITION_DRONE
        || result == DRONE_IS_LANDED || result == DRONE_NOT_SPAWNED){
        
        if (result == OCCUPIED_POSITION_WALL)
                map[y_pos+y_shift][x_pos+x_shift]= '#';        
        return 0;
    }

    sprintf(position_string, "Actual position: [ x = %d  |  y = %d  |  z = %d ] ", x_pos, y_pos, z_pos);
    sprintf(movement_string, "Trying to move%s%s%s : %s ",verb[0][x_shift+1],verb[1][y_shift+1],verb[2][z_shift+1], decode_result(result));
    if (battery>2*BATTERY_CAPACITY/3)
        sprintf(battery_string, GRN "Battery level = %.0f %%" RESET, battery/BATTERY_CAPACITY * 100);
    else if (battery>BATTERY_CAPACITY/3)
        sprintf(battery_string, YEL "Battery level = %.0f %%" RESET, battery/BATTERY_CAPACITY * 100);    
    else
        sprintf(battery_string, RED "Battery level = %.0f %%" RESET, battery/BATTERY_CAPACITY * 100);
    fprintf(log_file, "\n Actual position: [ x = %d  |  y = %d  |  z = %d ] \n", x_pos, y_pos, z_pos);
    fprintf(log_file, "Trying to move%s%s%s : %s \n" , verb[0][x_shift+1],verb[1][y_shift+1],verb[2][z_shift+1], decode_result(result));
    fprintf(log_file, "\t Battery level = %.0f %%\n", battery/BATTERY_CAPACITY * 100);

    refresh_position(x_shift, y_shift, z_shift);
    if (z_pos == 0)
        sprintf(position_string, "Actual position: [ x = %d  |  y = %d  |  z = %d ] ", x_pos, y_pos, z_pos);
    battery--;

    update_screen();

    usleep(1000000/speed);
    return 1;

}

//function to update the position with the shift specified in the input parameters
void refresh_position(int x_shift, int y_shift, int z_shift){
    
    x_pos += x_shift;
    y_pos += y_shift;
    z_pos += z_shift; 

    //marking current planar position on the drone's internal map
    if (z_pos > (map[y_pos][x_pos] - '0'))
        map[y_pos][x_pos] = z_pos+'0';
}

//prints the error message and close the socket connection
void error(char *msg) {
    
    // closing the socket
    if(close(sockfd) == -1)
        perror("ERROR while closing socket");
    perror(msg);
    exit(1);
} 

//function to connect to the socket server of the master
void connect_to_master() {

    sprintf(status_string, "====     Connecting to the master  ....");
    fprintf(log_file, "\n====     Connecting to the master  ....");

    //socket variables
    int n;
    struct sockaddr_in serv_addr;
    struct hostent *server; 

    //opening socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR while opening socket");

    //getting the server hostname
    server = gethostbyname("127.0.0.1");
    if (server == NULL)
        error("[MS8] ERROR while getting hostname\n"); 

	// Setting server data and port number (51234)
	bzero((char *) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
        error("[MS8] ERROR, invalid address or Address not supported \n");
    serv_addr.sin_port = htons(PORT_NUMBER); 

    //connecting to the server
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("[MS8] ERROR while connecting");

    sprintf(status_string, "   SUCCESS     ====");
    fprintf(log_file, "   SUCCESS     ====\n\n\n");
}

//function to decode the result returned by "drone_api.h" functions 
char *decode_result(int result){
    switch (result)
    {
    case 0:
        return "SUCCESS";
    case 1:
        return "TARGET POSITION OUT OF BOUNDS";
    case 2:
        return "WALL IN THE TARGET POSITION";
    case 3:
        return "DRONE IN THE TARGET POSITION";
    case 4:
        return "DRONE NOT SPAWNED";
    case 5:
        return "DRONE LANDED";
    }
    return NULL;
}

//function to spawn the drone in the position specified in the global variables x_spawn, y_spawn
//make user input new spawn coordinates in case of error
void spawn_drone(){
    int result;
    while(1){
        fprintf(stdout, "\nTrying to spawn the drone in the position [ x = %d |  y = %d |  z = 0 ] .. ", x_spawn, y_spawn);
        fprintf(log_file, "\nTrying to spawn the drone in the position [ x = %d |  y = %d |  z = 0 ] .. ", x_spawn, y_spawn);
        result = send_spawn_message(sockfd, x_spawn, y_spawn, 0);

        if (result == CONNECTION_ABORTED){
            master_terminated = 1;
            exit_routine(SIGTERM);
        }

        if (result == 0) {
            sprintf(status_string, "%s", strcat(status_string,decode_result(result)));
            fprintf(log_file, " %s\n", decode_result(result));
            return;
        }
        fprintf(stdout, "\n Cannot spawn the drone in the desired position : %s \n\n", decode_result(result));
        fprintf(log_file, "\n Cannot spawn the drone in the desired position : %s \n\n", decode_result(result));
        get_input();
    }
}

//function to get new spawn coordinates from the user and the speed
void get_input(){
    char buffer[80];
    fprintf(stdout, "Insert a valid initial coordinate: \n \t X : "); fflush(stdout);
    fgets(buffer, 80, stdin);
    x_spawn = atoi(buffer);
    fprintf(stdout, "\t Y : "); fflush(stdout);
    fgets(buffer, 80, stdin);
    y_spawn = atoi(buffer);
    fprintf(stdout, "Insert a speed value between 1 and 25: \t"); fflush(stdout);
    fgets(buffer, 80, stdin);
    speed = atoi(buffer) + 5;
}

//function to print on terminal the explored space and info on drone's movements
void update_screen(){
    
    //clears the screen
    printf(CLEAR);

    for(y=MAPY-1; y>-1; y--){
        for(x=0; x<MAPX; x++){
            if(y==y_pos && x==x_pos)
                fprintf(stdout,CYNB "X" RESET);
            else if (map[y][x] == '#')
                fprintf(stdout,  MAGHB "%c" RESET , map[y][x]);
            else if (abs(y_pos-y)<2 && abs(x_pos-x)<2)
                fprintf(stdout,  CYN "-" RESET );
            else if (map[y][x] == '#')
                fprintf(stdout,  BLUB "%c" RESET , map[y][x]);        
            else if ((map[y][x]  -'0') > 6)
                fprintf(stdout, GRN "%c" RESET, map[y][x]);
            else if ((map[y][x]  - '0') > 3)
                fprintf(stdout, YEL "%c" RESET, map[y][x]);
            else
                fprintf(stdout, RED "%c" RESET, map[y][x]);     
        }
	    if (y == 37)
            fprintf(stdout, CYN "   \t [ DRONE MS8 ] " RESET);
        else if (y == 33)
            fprintf(stdout, CYN "   EXPLORED SPACE " RESET);
        else if (y == 31)
            fprintf(stdout, CYN "   \t    with" RESET);
        else if (y == 30)
            fprintf(stdout,CYN "   Max Height Reached" RESET);
        fprintf(stdout,"\n");
    }
    fprintf(stdout,"\n");
    
    fprintf(stdout,"%s\n",status_string);
    fprintf(stdout,"%s\n",position_string);
    fprintf(stdout,"%s\n",movement_string);
    fprintf(stdout,"\t %s\n",battery_string);
    fprintf(stdout,BLU "  %s\n\n" RESET,charge_string); 

}

//function to close the socket connection before terminating
void exit_routine(int signo){

    if (master_terminated){
        fprintf(stdout, "\n\n MASTER IS UNAVAILABLE \n");
        fprintf(log_file, "\n\n MASTER IS UNAVAILABLE \n");
    }

    fprintf(stdout, "\n Terminating the drone's program     ...     ");
    fprintf(log_file, "\n Terminating the drone's program    ...     ");

    // closing the socket
    if(close(sockfd) == -1)
        perror("[MS8] ERROR while closing socket");

    fprintf(stdout, "SUCCESS \n");
    fprintf(log_file, "SUCCESS \n");
    
    usleep(2000000);
    exit(0);
}
