rm *o
rm *exe
gcc -g -c tcp_client.c -o tcp_client.o
gcc -g tcp_client.o -o tcp_client.exe
gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
gcc -g -c rt.c -o rt.o
gcc -g -c notif.c -o notif.o
gcc -g -c publisher.c -o publisher.o
gcc -g -c utils.c -o utils.o
gcc -g -c threaded_subsciber.c -o threaded_subsciber.o
gcc -g -c msgq_subs.c -o msgq_subs.o
gcc -g -c skt_subscriber.c -o skt_subscriber.o
gcc -g -c tcp_skt_subscriber.c -o tcp_skt_subscriber.o
gcc -g -c network_utils.c -o network_utils.o
gcc -g rt.o publisher.o notif.o utils.o threaded_subsciber.o gluethread/glthread.o network_utils.o -o exe -lpthread
gcc -g msgq_subs.o notif.o utils.o gluethread/glthread.o network_utils.o -o msgq_subs.exe -lpthread
gcc -g skt_subscriber.o notif.o utils.o  gluethread/glthread.o network_utils.o -o skt_subscriber.exe -lpthread
gcc -g tcp_skt_subscriber.o notif.o utils.o  gluethread/glthread.o network_utils.o -o tcp_skt_subscriber.exe -lpthread
gcc -g -c tcp_server.c -o tcp_server.o
gcc -g tcp_server.o notif.o utils.o network_utils.o gluethread/glthread.o -o tcp_server.exe -lpthread

