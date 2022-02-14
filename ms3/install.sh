#unzip the folder
unzip -q -d $1/ src.zip

# create 2 folder to organise the files
mkdir -p $1/exes
mkdir -p $1/logfile

# move the master into the exec folder
mv $1/src/master $1/exes

#compile the solution
gcc $1/src/source/drone_ms3.c -o $1/exes/drone_ms3 $1/src/drone_api/drone_api.o $1/src/logger/logger.o

# print a message
echo installation succeded