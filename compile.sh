gcc -g -c rt_user.c -o rt_user.o
gcc -g -c notif.c -o notif.o
gcc -g -c publisher.c -o publisher.o
gcc -g -c threaded_subsciber.c -o threaded_subsciber.o
gcc -g rt_user.o publisher.o notif.o threaded_subsciber.o -o exe -lpthread

