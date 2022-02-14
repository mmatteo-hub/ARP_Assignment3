#ifndef ASSIGNEMNT3_DRONE_API
#define ASSIGNEMNT3_DRONE_API

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>

#define SPAWN_MESSAGE 0
#define MOVE_MESSAGE 1
#define LANDING_MESSAGE 2

#define SUCCESS 0
#define OUT_OF_BOUNDS_POSITION 1
#define OCCUPIED_POSITION_WALL 2
#define OCCUPIED_POSITION_DRONE 3
#define DRONE_NOT_SPAWNED 4
#define DRONE_IS_LANDED 5

typedef struct{
    int spawned;
    int posx;
    int posy;
    int posz;
    int landed;
}Drone;

int decode_client_message(int id, int client_fd);

int send_spawn_message(int master_fd, int posx, int posy, int posz);
int (*handle_spawn_message)(int, int, int, int);

int send_move_message(int master_fd, int offx, int offy, int offz);
int (*handle_move_message)(int, int, int, int);

int send_landing_message(int master_fd, int landed);
int (*handle_landing_message)(int, int);

void drone_error(int error);

#endif //ASSIGNEMNT3_DRONE_API
