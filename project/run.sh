SOURCES_DIR="./sources"
EXES_DIR="./sources/exes"
LOGFILES_DIR="./sources/logfile"

# Terminals are launched for each process
gnome-terminal -- "$EXES_DIR/master" "$LOGFILES_DIR/log_master.txt"
gnome-terminal -- "$EXES_DIR/drone_ms3"
gnome-terminal -- "$EXES_DIR/drone_RM1" "37" "20" "6" "$LOGFILES_DIR/log_drone_RM1.txt"
gnome-terminal -- "$EXES_DIR/drone_MS8" "40" "20" "15"
gnome-terminal -- "$EXES_DIR/drone_DM3" "42" "18" "$LOGFILES_DIR/log_drone_DM3.txt" 

