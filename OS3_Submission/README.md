README
1. Compiling the admin  : make server
gcc -g3 -O0 admin.c cal-new.c types.h -lpthread -o server
2. Compiling the client  : make client
gcc -g3 -O0 client.c types.h -o client
server-name, server-host, client-id is to be passed as command line arguments

Files included for compiling various executables:
Server : admin.c cal-new.c types.h
client : client.c types.h
