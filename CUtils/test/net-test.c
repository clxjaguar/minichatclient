#include <stdio.h>
#include <unistd.h>

#include "../net.h"

#define bool int
#define true 1
#define false 0

bool cb(int socketd) {
	char buff[81];

	net_read(socketd, buff, 80);
	buff[81] = '\0';
	printf("Rec: <%s>\n", buff);

	close(socketd);

	return false;
}

int net_test(int argc, char *argv[]) {
	int socketd, socket;
	char BUFFER[81];
	int s;

    net_init();

	if (argc > 1) {
		printf("This programme does not support argument.\nYou passed: %s\n",
				argv[1]);
		return 1;
	}

	printf("Testing output...\n");

	socketd = net_connect("localhost", 6565);

	printf("socketd: %i\n", socketd);

	if (socketd >= 0) {
		net_write(socketd, "Hello world!", 13);
		net_close_socketd(socketd);
	}

	printf("Testing input...\n");

    printf("Now accepting connection on 6566...\n");
    socketd = net_listen(6566, 1);
    printf("listening socketd: %i\n", socketd);

	if (socketd >= 0) {
        socket = net_accept(socketd);
        s = net_read(socket, BUFFER, 80);
        if (s < 81)
            BUFFER[s] = '\0';
        BUFFER[81] = '\0';
        printf("We read: <%s>", BUFFER);
	}
    net_cleanup();

	return 0;
}
