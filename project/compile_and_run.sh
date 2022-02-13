# Defining important directories
SOURCES_DIR="./sources"
EXES_DIR="./sources/exes"
LOGFILES_DIR="./sources/logfile"

# Defining libraries object files locations
DRONE_API="$SOURCES_DIR/drone_api/drone_api.o"
LOGGER_LIB="$SOURCES_DIR/logger/logger.o"

# Compiling libraries
gcc -c $SOURCES_DIR/drone_api/drone_api.c -o $DRONE_API
gcc -c $SOURCES_DIR/logger/logger.c -o $LOGGER_LIB

# Compiling sources with requires libraries and commands
gcc $SOURCES_DIR/master/master.c -o $EXES_DIR/master -std=gnu99 $DRONE_API $LOGGER_LIB
gcc $SOURCES_DIR/drone_DF21/drone_DF21.c -o $EXES_DIR/drone_DF21 $DRONE_API
gcc $SOURCES_DIR/drone_ms3/drone_ms3.c -o $EXES_DIR/drone_ms3 $DRONE_API
gcc $SOURCES_DIR/drone_MS_8/drone_MS_8.c -o $EXES_DIR/drone_MS_8 $DRONE_API
gcc $SOURCES_DIR/drone_RM_1/drone_RM_1.c -o $EXES_DIR/drone_RM_1 $DRONE_API

sh ./run.sh
