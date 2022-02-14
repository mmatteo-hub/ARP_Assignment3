#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../drone_api/drone_api.h"
#include "../logger/logger.h"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

#define MAX_CLIENTS 6
#define MAP_SIZE_X 80
#define MAP_SIZE_Y 40
#define MAP_SIZE_Z 10

void free_resources();
void on_error(int signo);

void accept_client();
void disconnect_client(int index);

int is_inside_map(int posx, int posy, int posz);
int get_drone(int posx, int posy, int posz);
int get_visible_drone(int posx, int posy);

void create_map();
void print_map();
char drone_to_char(Drone drone);

int random_between(int min, int max);

int on_spawn_message(int id, int posx, int posy, int posz);
int on_move_message(int id, int offx, int offy, int offz);
int on_land_message(int id, int landing);

char* colors[] = {RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};

Logger logger = {"", "", -1};

int s_endpoint;
int c_endpoints[MAX_CLIENTS];

char map[MAP_SIZE_X][MAP_SIZE_Y];
Drone drones[MAX_CLIENTS];

int main(int argc, char *argv[])
{
    // Inits the random seed
    srand(time(NULL)); 
    
    // Inits logger
    create_logger(&logger, "MASTER", argv[1]);
    info(&logger, "Started the initialization phase.", 0);
  
    // Inits signal to release resources in case of error
    signal(SIGTERM, on_error);
    
    // Inits drones and endpoints
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        c_endpoints[i] = -1;
        drones[i] = (Drone){0, 0, -1, -1, -1};
    }
    
    // Inits drone_api library
    handle_spawn_message = on_spawn_message;
    handle_move_message = on_move_message;
    handle_land_message = on_land_message;
    
    // Prints an empty map
    create_map();
    print_map();
    
    info(&logger, "Data initialized, initializating socket.", 0);
    
    // Sets server data
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;             // Using protocol IPv4
    serv_addr.sin_port = htons(51234);          // Setting server port
    serv_addr.sin_addr.s_addr = INADDR_ANY;     // Server listens on all available interfaces
    
    // Opens socket
    if((s_endpoint = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        perror_exit(&logger, "Opening socket");
    
    // Binds server address to socket
    if (bind(s_endpoint, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        perror_exit(&logger, "Binding socket");
        
    // Makes the server listen for clients with a maximum queue of 5
    if(listen(s_endpoint, 5) == -1)
        perror_exit(&logger, "Listening on socket");
        
    info(&logger, "Socket initialized, waiting for connections.", 0);
    
    // Sets of file descriptors
    fd_set fds;
    
    while(1)
    {
        // Sets the server and clients fds to the set
        FD_ZERO(&fds);
        FD_SET(s_endpoint, &fds);
        for(int i=0; i<MAX_CLIENTS; ++i)
            if(c_endpoints[i] != -1)
                FD_SET(c_endpoints[i], &fds);
        
        // Waits for one of the file descriptors to become ready
        int activity = select(FD_SETSIZE+1, &fds, NULL, NULL, NULL);
        if(activity == -1)
            perror_exit(&logger, "Selecting a file descriptor");
            
        // If activity was on s_endpoint, a new client wants to connect
        if(FD_ISSET(s_endpoint, &fds))
            accept_client();
        
        // If activity was on one of the the c_endpoints, new data is available
        for(int i=0; i<MAX_CLIENTS; ++i)
            if(FD_ISSET(c_endpoints[i], &fds))
            {
                int error = decode_client_message(i, c_endpoints[i]);
                
                // If the client is not connected any more, disconnects it
                if(error == CONNECTION_ABORTED)
                    disconnect_client(i);
                else if(error == -1)
                    perror_exit(&logger, "Decoding client message");
            }
            
        // Prints the new map
        system("clear");
        print_map();
    }
        
    return 0;
}

void free_resources()
{
    // Closes connections with client
    for(int i=0; i<MAX_CLIENTS; ++i)
        if(c_endpoints[i] != -1 && close(c_endpoints[i]) == -1)
            perror_cont(&logger, "Closing client connection");
        
    // Closes socket
    if(close(s_endpoint) == -1)
        perror_cont(&logger, "Closing socket");
}

void on_error(int signo)
{
    if(signo == SIGTERM)
    {
        free_resources();
        exit(1);
    }
}

void accept_client()
{
    struct sockaddr_in cli_addr;
    
    // Accepts client connection
    int clilen = sizeof(cli_addr);
    int c_endpoint = accept(s_endpoint, (struct sockaddr *) &cli_addr, &clilen);
    if(c_endpoint == -1)
        perror_exit(&logger, "Accepting client connection");
        
    // Checks if server has a index for the client
    int index = -1;
    for(int i=0; i<MAX_CLIENTS; ++i)
        if(c_endpoints[i] == -1)
        {
            index = i;
            break;
        }
               
    // Gets client address
    char address[INET_ADDRSTRLEN];
    inet_ntop(cli_addr.sin_family, &(cli_addr.sin_addr), address, INET_ADDRSTRLEN);
    
    // If there is no space available
    if(index == -1)
    {
        // The connection is closed
        close(c_endpoint);
        // Prints info on log file
        char* text; asprintf(&text, "Client [%s] cannot connect: maximum capacity reached.", address);
        info(&logger, text, 0);
        free(text);
        return;
    }
    
    // Adds the client to the array
    c_endpoints[index] = c_endpoint;
    // Prints info on log file
    char* text; asprintf(&text, "Client [%s] connected at index %d.", address, index);
    info(&logger, text, 0);
    free(text);
}

void disconnect_client(int id)
{
    // Removes client file descriptor
    c_endpoints[id] = -1;
    
    // Reinits the drone
    drones[id] = (Drone){0, 0, -1, -1, -1};    
    
    char* text; asprintf(&text, "Client at index %d disconnected.", id);
    info(&logger, text, 0);
    free(text);
}

// Handler for a spawn request
int on_spawn_message(int id, int posx, int posy, int posz)
{
    // Basic checks
    if(drones[id].spawned == 1)
        return DRONE_ALREADY_SPAWNED;
    if(!is_inside_map(posx, posy, posz))
        return OUT_OF_BOUNDS_POSITION;
    if(map[posx][posy] == '*')
        return OCCUPIED_POSITION_WALL;
    if(get_drone(posx, posy, posz) != -1)
        return OCCUPIED_POSITION_DRONE;
    
    // Creates drone
    drones[id] = (Drone){1, 0, posx, posy, posz};
    
    return SUCCESS;
}

// Handler for a move request
int on_move_message(int id, int offx, int offy, int offz)
{
    Drone drone = drones[id];
    
    // Basic checks
    if(drone.spawned == 0)
        return DRONE_NOT_SPAWNED;
    if(drone.landed == 1)
        return DRONE_IS_LANDED;
        
    // Drone needs to move at one cell at a time
    if(abs(offx)>1 || abs(offy)>1 || abs(offz)>1)
        return ONE_CELL_AT_A_TIME;
        
    // Gets target position
    int posx = drone.posx + offx;
    int posy = drone.posy + offy;
    int posz = drone.posz + offz;

    // Basic checks
    if(!is_inside_map(posx, posy, posz))
        return OUT_OF_BOUNDS_POSITION;
    if(map[posx][posy] == '*')
        return OCCUPIED_POSITION_WALL;
    if(get_drone(posx, posy, posz) != -1)
        return OCCUPIED_POSITION_DRONE;
    
    // Moves drone
    drones[id].posx = posx;
    drones[id].posy = posy;
    drones[id].posz = posz;
    
    return SUCCESS;
}

// Handler for a land request
int on_land_message(int id, int landed)
{
    Drone drone = drones[id];
    
    // A not spawned drone cannot land
    if(drone.spawned == 0)
        return DRONE_NOT_SPAWNED;
    // A drone can land only at z=0
    if(drone.posz != 0)
        return LAND_ONLY_AT_Z0;
        
    drones[id].landed = landed;
    
    return SUCCESS;
}

// Checks if a position is inside the map
int is_inside_map(int posx, int posy, int posz)
{
    return  posx >= 0 && posx < MAP_SIZE_X && 
            posy >= 0 && posy < MAP_SIZE_Y &&
            posz >= 0 && posz < MAP_SIZE_Z;
}

// Gets the drone at (posx, posy, posz)
// Returns the index of the drone in the array
int get_drone(int posx, int posy, int posz)
{
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        Drone d = drones[i];
        if(d.posx == posx && d.posy == posy && d.posz == posz)
            return i;
    }
    
    return -1;
}

// Gets the drone at (posx, posy) with the highest z value
// Returns the index of the drone in the array
int get_visible_drone(int posx, int posy)
{
    int posz = -1;
    int index = -1;
    
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        Drone d = drones[i];
        if(d.posx == posx && d.posy == posy && d.posz > posz)
            index = i;
    }
    
    return index;
}

void create_map()
{
    // Creates outer walls of the map
    for(int x=0; x<MAP_SIZE_X; ++x)
    {
        for(int y=0; y<MAP_SIZE_Y; ++y)
        {
            if(x==0 || x==MAP_SIZE_X-1 || y==0 || y==MAP_SIZE_Y-1)
                map[x][y] = '*';
            else
                map[x][y] = ' ';
        }
    }
    
    // Creates random walls on the map
    int walls = random_between(10, 15);
    for(int i=0; i<walls; ++i)
    {
        int length = random_between(4, 12);
        int x = random_between(0, MAP_SIZE_X-length-1);
        int y = random_between(0, MAP_SIZE_Y-length-1);
        
        int multx = random_between(0, 1);
        int multy = (multx+1)%2;
        
        for(int j=0; j<length; ++j)
            map[(x+multx*j)][(y+multy*j)] = '*';
    }
    
    // Create a safe after for drones to spawn
    // The are is 10x10 cells at the centre of the map
    for(int x=MAP_SIZE_X/2-5; x<=MAP_SIZE_X/2+5; ++x)
        for(int y=MAP_SIZE_Y/2-5; y<=MAP_SIZE_Y/2+5; ++y)
            map[x][y] = ' ';
    
}

int random_between(int min, int max)
{
    return min + rand()%(max+1-min);
}

void print_map()
{    
    // Sets markers for drones to avoid checking if 
    // in each position of the map there is a drone
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        Drone d = drones[i];
        if(d.spawned)
            map[d.posx][d.posy] = 'd';
    }
    
    // Prints map so that x axis is the length
    // and y axis is the width of the console
    for(int y=MAP_SIZE_Y-1; y>=0; --y)
    {
        for(int x=0; x<MAP_SIZE_X; ++x)
        {
            if(map[x][y]=='d')
            {
                int id = get_visible_drone(x, y);
                printf("%s%c" RESET, colors[id], drone_to_char(drones[id]));
            }
            else
                printf("%c", map[x][y]);
            
        }
        printf("\n");
    }
    
    // Removes markers
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        Drone d = drones[i];
        if(d.spawned)
            map[d.posx][d.posy] = ' ';
    }
    
    fflush(stdout);
}

// Function that decides the char to represent the drone
char drone_to_char(Drone drone)
{
    if(!drone.spawned)
        return ' ';
    if(drone.landed)
        return 'X';
    return (char)(drone.posz+48);
}
