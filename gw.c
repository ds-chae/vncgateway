// Server side C/C++ program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

int comm_loop(int a1socket, int vnc_socket);
int role_1();
int create_server(int port);
int role_2(int port1, int port2, const char* gwip);

struct sockaddr_in a1;
int a1len = sizeof(a1);
struct sockaddr_in a2;
int a2len = sizeof(a2);
struct sockaddr_in a3;
int a3len = sizeof(a3);

int v = 0;

int role = 0;
int port1 = 5906;
int port2 = 5907;
int port3 = 5908;
int p1socket = -1;
int fromvnc = -1;
int p3socket = -1;

int main(int argc, char const* argv[])
{
    if(argc < 4) {
	    printf("usage: gw role port1 port2 {ip-address}\n");
	    exit(-1);
	}
	
    role = atoi(argv[1]);
	port1 = atoi(argv[2]);
	port2 = atoi(argv[3]);
	if(role < 1 || role > 2) {
			printf("role must be in range 1~4.\n");
			exit(-1);
	}

	if(port1 < 5901 || port1 > 5909){
			printf("port1 %d must be in range 5901~5909\n", port1);
			exit(-1);
	}
	if(port2 < 5901 || port2 > 5909){
			printf("port2 %d must be in range 5901~5909\n", port2);
			exit(-1);
	}
	printf("%d %d %d\n", role, port1, port2);

	if(role == 1 )
		role_1(port1, port2);

	if(role == 2)
		role_2(port1, port2, argv[4]);

	return 0;
}

fd_set read_flags,write_flags; // the flag sets to be used
struct timeval waitd = {120, 0};          // the max wait time for an event
struct timeval r2time = {3600, 0};          // the max wait time for an event
int sel;                      // holds return value for select();


int a1socket = -1, vnc_socket = -1;

void *vnc_accept_thread(void *arg)
{
    fromvnc = create_server(port2);
	printf("Accepting vnc_socket, port=%d\n", port2);
	for(;;) {
		int vnc_temp = accept(fromvnc, (struct sockaddr*)&a2, (socklen_t*)&a2len);
		if(vnc_temp < 0) {
			perror("role_1:2:accept"); // This will not happen...
			exit(EXIT_FAILURE);
		}
		if(vnc_socket == -1) {
			vnc_socket = vnc_temp;
			printf("vnc_socket accepted=%d\n", vnc_socket);
		} else {
			close(vnc_temp);
		}
	}
}

pthread_t vnc_thread;

int role_1()
{
	int maxfd = 0;

	pthread_create(&vnc_thread, NULL, vnc_accept_thread, NULL);
    p1socket = create_server(port1); // Creating socket file descriptor
	for(;;) {
		printf("Accepting VNC client %d\n", port1);
		int t_socket = accept(p1socket, (struct sockaddr*)&a1, (socklen_t*)&a1len);
		if(t_socket < 0) {
			perror("role_1:1:accept"); // This will not happen...also.
			exit(EXIT_FAILURE);
		}
		if(a1socket != -1) {
			close(t_socket);
		} else {
			a1socket = t_socket;
			if(vnc_socket != -1) {
				printf("role_1:1:accept success \n");
				comm_loop(a1socket, vnc_socket);
				printf("closing vnc_socket\n");
				close(vnc_socket); vnc_socket = -1;
			} else {
				printf("vnc not ready\n");
				close(a1socket);
			}
		}
		if(a1socket != -1) {
			printf("closing a1socket\n");
			close(a1socket);
			a1socket = -1;
		}
	}

	return 0;
}

int no_blocking(int s)
{
	int flags = fcntl(s, F_GETFL);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

	return 0;
}


char gw_buf[16384] = { 0 };
char vnc_buf[16384] = { 0 };

char* lchost = "127.0.0.1";

int role_2(int port1, int port2, const char* gwip)
{
	int maxfd = 0;
    int gw_sock = -1;
    int vnc_sock = -1;
    struct sockaddr_in vnc_addr;
    struct sockaddr_in gw_addr;

    int vnc_bytes = 0, gw_bytes = 0;
    int vnc_sent = 0, gw_sent = 0;

	vnc_addr.sin_family = AF_INET;
	vnc_addr.sin_port = htons(port1);

	if (inet_pton(AF_INET, lchost, &vnc_addr.sin_addr) <= 0) {
	    printf( "role_2:Invalid address %s/ Address not supported \n", lchost);
	    return -1;
   	}
	if (inet_pton(AF_INET, gwip, &gw_addr.sin_addr) <= 0) {
	    printf( "role_2:Invalid address %s/ Address not supported \n", gwip);
	    return -1;
   	}

	for(;;) {
		if(vnc_sock > 0) {close(vnc_sock); vnc_sock = -1; }
		if(gw_sock > 0) {close(gw_sock); gw_sock = -1; }

        printf("\n\nrole_2:Connecting gw %d\n", port2);
	    if ((gw_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	        printf("role_2:gw:Socket creation error \n");
	        return -1;
	    }
        printf("role_2:gw:Socket gw_sock =%d\n", gw_sock);
		gw_addr.sin_family = AF_INET;
		gw_addr.sin_port = htons(port2);
		if(connect(gw_sock, (struct sockaddr*)&gw_addr, sizeof(gw_addr)) < 0) {
	        printf("role_2:gw:Connection Failed %s:%d\n", gwip, port2);
			close(gw_sock); gw_sock = -1;
			sleep(2);
			continue;
		}
        printf("role_2:gw:Connection success %s:%d\n", gwip, ntohs(gw_addr.sin_port));
		maxfd = gw_sock;
       	while(0) {
	        printf("role_2:gw:Waiting first byte from %d\n", gw_sock);
			int rb = recv(gw_sock, gw_buf, 1, 0);
			if(rb > 0) {
				gw_buf[rb] = 0;
				printf("recv=%s\n", gw_buf);
				break;
			}
			sleep(1);
		}
		if(1){
	        printf("role_2:gw:Waiting first byte from %d\n", gw_sock);
			printf("call select maxfd=%d\n", maxfd);
	        FD_ZERO(&read_flags); FD_ZERO(&write_flags);
			FD_SET(gw_sock, &read_flags); FD_SET(gw_sock, &write_flags);
			sel = select(maxfd+1, &read_flags, (fd_set*)0, (fd_set*)0, NULL);
			if(sel < 0) { // error on gw_sock
				close(gw_sock); gw_sock = -1;
				printf("Error on gw_sock\n");
				continue;
			}
			if(sel == 0) {
				gw_bytes = recv(gw_sock, gw_buf, 1, 0); // do no test FD_ISSET(gw_sock...)
				if(gw_bytes > 0) {
					printf("select timeout, but recv success\n");
					gw_buf[gw_bytes] = 0;
					printf("recv=%s\n", gw_buf);
				}
				close(gw_sock); gw_sock = -1;
				continue;
			}
			gw_bytes = recv(gw_sock, gw_buf, 1, 0); // do no test FD_ISSET(gw_sock...)
			if(gw_bytes != 1) {
				close(gw_sock); gw_sock = -1;
				printf("Error reading gw_sock 1\n");
				continue;
			}
			printf("gw read 1 byte success\n");
		}
	    if ((vnc_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        	printf("role_2:vnc:Socket creation error \n");
			close(gw_sock); gw_sock = -1;
	        return -1;
	    }

        printf("role_2:vnc:Connecting %d \n", ntohs(vnc_addr.sin_port));
		if(connect(vnc_sock, (struct sockaddr*)&vnc_addr, sizeof(vnc_addr)) < 0) {
	        printf("role_2:vnc:Connection Failed \n");
			close(vnc_sock); vnc_sock = -1;
			close(gw_sock); gw_sock = -1;
			sleep(5);
			continue;
		} else {
	        printf("role_2:vnc:Connection Success %d, fd=%d\n", port1, vnc_sock);
			maxfd = vnc_sock;
			if(maxfd < gw_sock)
				maxfd = gw_sock;
			printf("vnc gw connected maxfd=%d\n", maxfd);
        	while(vnc_sock > 0 && gw_sock > 0) {
		        FD_ZERO(&read_flags); FD_ZERO(&write_flags);
				FD_SET(vnc_sock, &read_flags); FD_SET(vnc_sock, &write_flags);
				FD_SET(gw_sock, &read_flags); FD_SET(gw_sock, &write_flags);
		       	//sel = select(maxfd+1, &read_flags, &write_flags, (fd_set*)0, &waitd);
				//printf("role_2:selecting %d\n", maxfd);
		       	sel = select(maxfd+1, &read_flags, (fd_set*)0, (fd_set*)0, &r2time);
				if(sel <= 0) {
					printf("role_2:selecting %d timeout\n", maxfd);
					if(FD_ISSET(gw_sock, &read_flags)) {
						printf("role_2:gw_sock flag is set");
					}
					if(FD_ISSET(vnc_sock, &read_flags)) {
						printf("role_2:vnc_sock flagis set");
					}
					close(gw_sock); gw_sock = -1;
					close(vnc_sock); vnc_sock = -1;
					break;
				}
				if(FD_ISSET(gw_sock, &read_flags)) {
					gw_bytes = recv(gw_sock, gw_buf, sizeof(gw_buf), 0);
					if(gw_bytes <= 0){
						printf("role_2: gw_sock read 0 bytes, closing.\n");
						close(gw_sock); gw_sock = -1;
						break;
					}
					gw_sent = send(vnc_sock, gw_buf, gw_bytes, 0);
					if(gw_sent != gw_bytes) {
						printf("role_2: vnc_sock write 0 bytes, closing.\n");
						close(vnc_sock); vnc_sock = -1;
					}
				}
				if(FD_ISSET(vnc_sock, &read_flags)) {
					vnc_bytes = recv(vnc_sock, vnc_buf, sizeof(vnc_buf), 0);
					if(vnc_bytes <= 0){
						printf("role_2: vnc_sock read 0 bytes, closing.\n");
						close(vnc_sock); vnc_sock = -1;
						break;
					}
					vnc_sent = send(gw_sock, vnc_buf, vnc_bytes, 0);
					if(vnc_sent != vnc_bytes){
						printf("role_2: gw_sock write 0 bytes, closing.\n");
						close(gw_sock); gw_sock = -1;
						break;
					}
				}
			}

		}
		if(vnc_sock > 0) {
			printf("End loop, closing vnc_sock\n");
			close(vnc_sock); vnc_sock = -1;
		}
		if(gw_sock > 0) {
			printf("End loop, closing gw_sock\n");
			close(gw_sock); gw_sock = -1;
		}
    }

    return 0;
}

int create_server(int port)
{
    int server_fd, new_socket, valread;
    int opt = 1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int flags = -1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("create_server:socket failed");
        return -1;
    }
  
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("create_server:setsockopt");
        return -1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
	printf("create_server port=%d\n", port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("create_server:bind failed");
        return -1;
    }
    if (listen(server_fd, 3) < 0) {
        perror("create_server:listen");
        return -1;
    }

	printf("create_server return %d\n", server_fd);
	return server_fd;
}

char a1buf[16384];
char a2buf[16384];

int comm_loop(int a1socket, int vnc_socket)
{
	int sel = 0;
	int maxfd = a1socket;
	int r1bytes = 0, r2bytes = 0;
	int a1sent = 0, a2sent = 0;

	send(vnc_socket, "q", 1, 0);
	if(maxfd < vnc_socket)
		maxfd = vnc_socket;

	for(;;) {
        FD_ZERO(&read_flags);
		FD_ZERO(&write_flags);
		FD_SET(a1socket, &read_flags); FD_SET(a1socket, &write_flags);
		FD_SET(vnc_socket, &read_flags); FD_SET(vnc_socket, &write_flags);
       	//sel = select(maxfd+1, &read_flags, &write_flags, (fd_set*)0, &waitd);
		if(v) printf("comm_loop:select %d\n", maxfd);
       	sel = select(maxfd+1, &read_flags, (fd_set*)0, (fd_set*)0, &waitd);
		if(sel <= 0) {
				printf("comm_loop: timeout\n");
				return -1;
		}
		if(FD_ISSET(a1socket, &read_flags)) {
			r1bytes = recv(a1socket, a1buf, sizeof(a1buf), 0);
			if(v) printf("a1 ready %d bytes\n", r1bytes);
			else printf("\n");
			if(r1bytes <= 0){
				printf("a1 0, return\n");
				return -1;
			}
			a2sent = send(vnc_socket, a1buf, r1bytes, 0);
			if(a2sent != r1bytes) {
				printf("send to vnc fail\n");
				return -1;
			}
		}
		if(FD_ISSET(vnc_socket, &read_flags)) {
			r2bytes = recv(vnc_socket, a2buf, sizeof(a2buf), 0);
				if(v) printf("a2 ready %d bytes\n", r2bytes);
			if(r2bytes <= 0){
				printf("a2 0, return\n");
				return -1;
			}
			a1sent = send(a1socket, a2buf, r2bytes, 0);
			if(a1sent != r2bytes)
				return -1;
		}
	}

	return 0;
}
