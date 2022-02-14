# Creates specified folder if not existing
mkdir -p $1

# Unzips into the specified directory
unzip -q project.zip -d $1

# Defining important directories
SOURCES_DIR="$1/sources"
EXES_DIR="$1/sources/exes"
LOGFILES_DIR="$1/sources/logfile"

# Creats folder to store the executable files
mkdir -p $EXES_DIR
# Creating folder to store the log files
mkdir -p $LOGFILES_DIR

# Defining libraries object files locations
DRONE_API="$SOURCES_DIR/drone_api/drone_api.o"
LOGGER_LIB="$SOURCES_DIR/logger/logger.o"

# Compiling libraries
gcc -c $SOURCES_DIR/drone_api/drone_api.c -o $DRONE_API
gcc -c $SOURCES_DIR/logger/logger.c       -o $LOGGER_LIB

# Compiling sources with requires libraries
gcc $SOURCES_DIR/master/master.c       -o $EXES_DIR/master      $DRONE_API $LOGGER_LIB -std=gnu99 
gcc $SOURCES_DIR/drone_ms3/drone_ms3.c -o $EXES_DIR/drone_ms3   $DRONE_API $LOGGER_LIB
gcc $SOURCES_DIR/drone_RM1/drone_RM1.c -o $EXES_DIR/drone_RM1 $DRONE_API
#gcc $SOURCES_DIR/drone_DF21/drone_DF21.c -o $EXES_DIR/drone_DF21 $DRONE_API
#gcc $SOURCES_DIR/drone_MS_8/drone_MS_8.c -o $EXES_DIR/drone_MS_8 $DRONE_API


