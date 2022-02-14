# find the folder to run the executable
cd -- "$(find . -iname exes -type d)"

#run the two processes in two different consoles
gnome-terminal -- sh -c "./master ./../logfile/log_master.txt"

# chose to run the drone ms3 with a starting position 10 10
gnome-terminal -- sh -c "./drone_ms3"
