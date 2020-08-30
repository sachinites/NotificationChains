gcc -g -c rt_user.c -o rt_user.o
gcc -g -c notif.c -o notif.o
gcc -g -c test.c -o test.o
gcc -g rt_user.o test.o notif.o -o exe
