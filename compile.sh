gcc -g -c rt.c -o rt.o
gcc -g -c notif.c -o notif.o
gcc -g -c publisher.c -o publisher.o
gcc -g -c utils.c -o utils.o
gcc -g -c threaded_subsciber.c -o threaded_subsciber.o
gcc -g rt.o publisher.o notif.o utils.o rt_notif.o threaded_subsciber.o -o exe -lpthread

