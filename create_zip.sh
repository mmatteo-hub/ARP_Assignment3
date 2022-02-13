# Copies the project contents inside a temporary folder ./temp
cp -r ./project ./temp

# Removes unnecessary directories (recursively)
rm -r ./temp/sources/exes
rm -r ./temp/sources/logfile
# Removes unnecessary library object files
rm ./temp/sources/drone_api/drone_api.o
rm ./temp/sources/logger/logger.o
# Removes the specified files (used only for developing)
rm ./temp/compile_and_run.sh

# Moves inside of ./temp (necessary to zip)
cd ./temp
# Zips the contents of ./temp into a temporary project.zip
zip -r -q ../project.zip ./*
cd ../

# Removes ./temp
rm -r ./temp

# Zips the project.zip files + install.sh in assignment.zip
zip -r -q ./assignment.zip ./project.zip ./install.sh

# Removes the temporary project.zip files
rm -r ./project.zip
