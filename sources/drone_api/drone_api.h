#ifndef ASSIGNEMNT3_DRONE_API
#define ASSIGNEMNT3_DRONE_API

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>

#define SPAWN_MESSAGE 0
#define MOVE_MESSAGE 1
#define LAND_MESSAGE 2

#define SUCCESS 0
#define OUT_OF_BOUNDS_POSITION 1
#define OCCUPIED_POSITION_WALL 2
#define OCCUPIED_POSITION_DRONE 3
#define DRONE_NOT_SPAWNED 4
#define DRONE_ALREADY_SPAWNED 5
#define DRONE_IS_LANDED 6
#define ONE_CELL_AT_A_TIME 7
#define LAND_ONLY_AT_Z0 8

typedef struct{
    int spawned;
    int posx;
    int posy;
    int posz;
    int landed;
}Drone;

// A function that the server can use to decode the client
// message and automatically call the corresponding handler
// with already parsed data
int decode_client_message(int id, int client_fd);

// A function that the client can use to spawn a drone
int send_spawn_message(int master_fd, int posx, int posy, int posz);
int (*handle_spawn_message)(int, int, int, int);

// A function that the client can use to move a drone
int send_move_message(int master_fd, int offx, int offy, int offz);
int (*handle_move_message)(int, int, int, int);

// A function that the client can use to land or take off the drone
int send_land_message(int master_fd, int landed);
int (*handle_land_message)(int, int);

// A function that prints a string in the default console
// the string contains some human-readable information about the error
void drone_error(int error);

#endif //ASSIGNEMNT3_DRONE_API
