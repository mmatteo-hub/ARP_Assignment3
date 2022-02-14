#include "./drone_api.h"

int is_alive(int socket_fd)
{
    int error = 0;
    socklen_t len = sizeof(error);
    if(getsockopt (socket_fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
        return -1;
    if(error == EPIPE)
        return CONNECTION_ABORTED;
    return error;
}

int decode_client_message(int id, int client_fd)
{
    // Before using the file_descriptor, checks if the client is still connected
    int error = is_alive(client_fd);
    if(error != SUCCESS)
        return error;
    
    // If the message is not correcly decoded, this value is not changed
    // and the drone receives an INVALID_MESSAGE error
    int result = INVALID_MESSAGE;
    
    // Gets the message type
    int message;
    if(read(client_fd, &message, sizeof(int)) != -1)
    {
        // Decodes the message and calls the correct handler
        switch(message)
        {
            case SPAWN_MESSAGE:
            {
                int posx, posy, posz;
                if(read(client_fd, &posx, sizeof(int)) == -1)
                    break;
                if(read(client_fd, &posy, sizeof(int)) == -1)
                    break;
                if(read(client_fd, &posz, sizeof(int)) == -1)
                    break;
                    
                result = handle_spawn_message(id, posx, posy, posz);
                break;
            }
            case MOVE_MESSAGE:
            {
                int offx, offy, offz;
                if(read(client_fd, &offx, sizeof(int)) == -1)
                    break;
                if(read(client_fd, &offy, sizeof(int)) == -1)
                    break;
                if(read(client_fd, &offz, sizeof(int)) == -1)
                    break;
                    
                result = handle_move_message(id, offx, offy, offz);
                break;
            }
            case LAND_MESSAGE:
            {
                int landing;
                if(read(client_fd, &landing, sizeof(int)) == -1)
                    break;
                    
                result = handle_land_message(id, landing);
                break;
            }
            default:
                break;
        }
    }
        
    // Sends the result to the drone
    if(write(client_fd, &result, sizeof(int)) == -1)
        return -1;
        
    return 0;
}

int send_spawn_message(int master_fd, int posx, int posy, int posz)
{
    int error = is_alive(master_fd);
    if(error != SUCCESS)
        return error;
        
    int message = SPAWN_MESSAGE;
    if(write(master_fd, &message, sizeof(int)) == -1)
        return -1;
    if(write(master_fd, &posx, sizeof(int)) == -1)
        return -1;
    if(write(master_fd, &posy, sizeof(int)) == -1)
        return -1;
    if(write(master_fd, &posz, sizeof(int)) == -1)
        return -1;
        
    int result;
    if(read(master_fd, &result, sizeof(int)) == -1)
        return -1;
    
    return result;
}

int send_move_message(int master_fd, int offx, int offy, int offz)
{
    int error = is_alive(master_fd);
    if(error != SUCCESS)
        return error;
        
    int message = MOVE_MESSAGE;
    if(write(master_fd, &message, sizeof(int)) == -1)
        return -1;
    if(write(master_fd, &offx, sizeof(int)) == -1)
        return -1;
    if(write(master_fd, &offy, sizeof(int)) == -1)
        return -1;
    if(write(master_fd, &offz, sizeof(int)) == -1)
        return -1;
        
    int result;
    if(read(master_fd, &result, sizeof(int)) == -1)
        return -1;
        
    return result;
}

int send_land_message(int master_fd, int landed)
{
    int error = is_alive(master_fd);
    if(error != SUCCESS)
        return error;
        
    int message = LAND_MESSAGE;
    if(write(master_fd, &message, sizeof(int)) == -1)
        return -1;
        
    if(write(master_fd, &landed, sizeof(int)) == -1)
        return -1;
        
    int result;
    if(read(master_fd, &result, sizeof(int)) == -1)
        return -1;
        
    return result;
}

void drone_error(int error)
{
    switch(error)
    {
        case SUCCESS:
            printf("Success\n");
            break;
        case INVALID_MESSAGE:
            printf("Message contents are not valid\n");
            break;
        case OUT_OF_BOUNDS_POSITION:
            printf("Target position is not inside map\n");
            break;
        case OCCUPIED_POSITION_WALL:
            printf("Target position is occupied by wall\n");
            break;
        case OCCUPIED_POSITION_DRONE:
            printf("Target position is occupied by drone\n");
            break;
        case DRONE_NOT_SPAWNED:
            printf("Drone is not yet spawned in map\n");
            break;
        case DRONE_ALREADY_SPAWNED:
            printf("Drone is already spawned in map\n");
            break;
        case DRONE_IS_LANDED:
            printf("Drone is currently landed\n");
            break;
        case ONE_CELL_AT_A_TIME:
            printf("Drone needs to move one cell at a time\n");
            break;
        case LAND_ONLY_AT_Z0:
            printf("Drone can land only at z=0\n");
            break;
        case CONNECTION_ABORTED:
            printf("The other socket-end aborted the connection\n");
            break;
        default:
            printf("Print not implemented for %d\n", error);
    }
}
