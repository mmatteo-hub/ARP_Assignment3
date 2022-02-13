SOURCES_DIR="./sources"
EXES_DIR="./sources/exes"
LOGFILES_DIR="./sources/logfile"

# Terminals are launched for each process
gnome-terminal -- "$EXES_DIR/master" "$LOGFILES_DIR/log_master.txt"
gnome-terminal -- "$EXES_DIR/drone_ms3" "$LOGFILES_DIR/log_master.txt"
