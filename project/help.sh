cat \
project_description.txt <(echo) \
sources/drone_api/drone_api.txt <(echo) \
sources/logger/logger.txt <(echo) \
sources/master/master.txt <(echo) \
| more -d -c -p
