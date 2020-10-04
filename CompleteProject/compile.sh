rm *o
rm *exe
gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
gcc -g -c rt.c -o rt.o
gcc -g -c notif.c -o notif.o
gcc -g -c publisher.c -o publisher.o
gcc -g -c utils.c -o utils.o
gcc -g -c threaded_subsciber.c -o threaded_subsciber.o
gcc -g -c msgq_subs.c -o msgq_subs.o
gcc -g -c skt_subscriber.c -o skt_subscriber.o
gcc -g rt.o publisher.o notif.o utils.o threaded_subsciber.o gluethread/glthread.o -o exe -lpthread
gcc -g msgq_subs.o notif.o utils.o gluethread/glthread.o -o msgq_subs.exe
gcc -g skt_subscriber.o notif.o utils.o  gluethread/glthread.o -o skt_subscriber.exe -lpthread

