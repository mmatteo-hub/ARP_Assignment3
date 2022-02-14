#ifndef ASSIGNEMNT3_DRONE_API
#define ASSIGNEMNT3_DRONE_API

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>

#define SPAWN_MESSAGE 0
#define MOVE_MESSAGE 1
#define LAND_MESSAGE 2


#define SUCCESS 0                   // The request has been completed without errors.
#define INVALID_MESSAGE 1           // The contents of the message are not valid.
#define OUT_OF_BOUNDS_POSITION 2    // The requested position is outside the map.
#define OCCUPIED_POSITION_WALL 3    // The requested position is occupied by a wall.
#define OCCUPIED_POSITION_DRONE 4   // The requested position is occupied by a drone.
#define DRONE_NOT_SPAWNED 5         // The drone needs to be spawned.
#define DRONE_ALREADY_SPAWNED 6     // The drone is already spawned in map.
#define DRONE_IS_LANDED 7           // The drone needs to not be landed.
#define ONE_CELL_AT_A_TIME 8        // The drone requested to move more than 1 cell.
#define LAND_ONLY_AT_Z0 9           // The drone requested to land when not at z=0.
#define CONNECTION_ABORTED 10       // The other socket-end aborted the connection.       

typedef struct{
    // True if the drone spawned in map
    int spawned;
    // True if the drone is landed
    int landed;
    // Current position of the drone
    int posx;
    int posy;
    int posz;
}Drone;

// Returns 0 in case of no error occurring and other end alive
// Returns CONNECTION_ABORTED if the other end aborted the connection
// Returns -1 in case of some other error occurring
int is_alive(int socket_fd);

// A function that the server can use to decode the client
// message and automatically call the corresponding handler
// without dealing with parsing data
int decode_client_message(int id, int client_fd);

// A function that a drone can use to send a spawn request
int send_spawn_message(int master_fd, int posx, int posy, int posz);
// Handler for spawn request
int (*handle_spawn_message)(int, int, int, int);

// A function that a drone can use to send a move request
int send_move_message(int master_fd, int offx, int offy, int offz);
// Handler for move request
int (*handle_move_message)(int, int, int, int);

// A function that the drone can use to send land or take off request
int send_land_message(int master_fd, int landed);
// Handler for land request
int (*handle_land_message)(int, int);

// A function that prints the error in console in a
// human-readable english string
void drone_error(int error);

#endif //ASSIGNEMNT3_DRONE_API
