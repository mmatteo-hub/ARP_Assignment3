#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
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

void create_map();
void print_map();
int is_inside_map(int posx, int posy, int posz);
int get_drone(int posx, int posy, int posz);
int get_visible_drone(int posx, int posy);
char drone_to_char(Drone drone);

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
    // Init logger
    create_logger(&logger, "MASTER", argv[1]);
    info(&logger, "Started the initialization phase.", 0);
  
    // So that process can release resources in case of error
    signal(SIGTERM, on_error);
    
    // Init drones and endpoints
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        c_endpoints[i] = -1;
        drones[i] = (Drone){0, -1, -1, -1, 0};
    }
    
    // Init drone_api library
    handle_spawn_message = on_spawn_message;
    handle_move_message = on_move_message;
    handle_land_message = on_land_message;
    
    // Prints an empty map
    create_map();
    print_map();
    
    info(&logger, "Data initialized, initializating socket.", 0);
    
    // Setting server data
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;             // Using protocol IPv4
    serv_addr.sin_port = htons(51234);          // Setting server port
    serv_addr.sin_addr.s_addr = INADDR_ANY;     // Server listens on all available interfaces
    
    // Opening socket
    if((s_endpoint = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        perror_exit(&logger, "Opening socket");
    
    // Binding server address to socket
    if (bind(s_endpoint, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        perror_exit(&logger, "Binding socket");
        
    // Makes the server listen for clients with a maximum queue of 5
    if(listen(s_endpoint, 5) == -1)
        perror_exit(&logger, "Listening on socket");
        
    info(&logger, "Socket initialized, waiting for connections.", 0);
    
    // Set of file descriptors
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
                if(error == EPIPE)
                    disconnect_client(i);
                else if(error == -1)
                    perror_exit(&logger, "Decoding client message");
            }
            
        // Prints the new map
        print_map();
    }
        
    return 0;
}

void free_resources()
{
    // Closing connections with client
    for(int i=0; i<MAX_CLIENTS; ++i)
        if(c_endpoints[i] != -1 && close(c_endpoints[i]) == -1)
            perror_cont(&logger, "Closing client connection");
        
    // Closing socket
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
    
    // Accepting client connection
    int clilen = sizeof(cli_addr);
    int c_endpoint = accept(s_endpoint, (struct sockaddr *) &cli_addr, &clilen);
    if(c_endpoint == -1)
        perror_exit(&logger, "Accepting client connection");
        
    // Checking if server has a index for the client
    int index = -1;
    for(int i=0; i<MAX_CLIENTS; ++i)
        if(c_endpoints[i] == -1)
        {
            index = i;
            break;
        }
        
    // Getting client address
    char address[INET_ADDRSTRLEN];
    inet_ntop(cli_addr.sin_family, &(cli_addr.sin_addr), address, INET_ADDRSTRLEN);
        
    if(index != -1)
    {
        c_endpoints[index] = c_endpoint;
        char* text; asprintf(&text, "Client [%s] connected at index %d.", address, index);
        info(&logger, text, 0);
        free(text);
    }
    else
    {
        close(c_endpoint);
        info(&logger, "Client [%s] cannot connect: maximum capacity reached.", 0);
    }
}

void disconnect_client(int id)
{
    // Removing client file descriptor
    c_endpoints[id] = -1;
    
    // Reinitializing drone
    drones[id] = (Drone){0, -1, -1, 0};    
    
    char* text; asprintf(&text, "Client at index %d disconnected.", id);
    info(&logger, text, 0);
    free(text);
}

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
    
    // Creating drone
    drones[id] = (Drone){1, posx, posy, posz, 0};
    
    return SUCCESS;
}

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
        
    // Getting target position
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
    
    // Moving drone
    drones[id].posx = posx;
    drones[id].posy = posy;
    drones[id].posz = posz;
    
    return SUCCESS;
}


int on_land_message(int id, int landed)
{
    Drone drone = drones[id];
    
    // A not spawned drone cannot land
    if(drone.spawned == 0)
        return DRONE_NOT_SPAWNED;
    if(drone.posz != 0)
        return LAND_ONLY_AT_Z0;
        
    drones[id].landed = landed;
    
    return SUCCESS;
}

// Checks if position is inside the map
int is_inside_map(int posx, int posy, int posz)
{
    return  posx >= 0 && posx < MAP_SIZE_X && 
            posy >= 0 && posy < MAP_SIZE_Y &&
            posz >= 0 && posz < MAP_SIZE_Z;
}

// Gets the drone at (x, y, z)
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

// Gets the drone at (x, y) with the highest z value
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
    // Creating outher walls
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
    
    // Creating walls in center
    for(int i=0; i<10; ++i)
    {        
        map[54][i+03] = '*';
        map[43][i+24] = '*';
        map[i+06][13] = '*';
        map[i+38][33] = '*';
    }
}

void print_map()
{    
    // Setting markers for drones
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        Drone d = drones[i];
        if(d.spawned)
            map[d.posx][d.posy] = 'd';
    }
    
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
    
    // Removing markers
    for(int i=0; i<MAX_CLIENTS; ++i)
    {
        Drone d = drones[i];
        if(d.spawned)
            map[d.posx][d.posy] = ' ';
    }
    
    fflush(stdout);
}

char drone_to_char(Drone drone)
{
    if(!drone.spawned)
        return ' ';
    if(drone.landed)
        return 'X';
    return (char)(drone.posz+48);
}
