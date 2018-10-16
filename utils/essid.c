#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <fcntl.h>
#include <errno.h>
#include <linux/wireless.h>

#define INTERVAL   5
#define INTERFACE  "wlan0"
#define FORMAT     "WII;%s"

char *format = FORMAT;
int interval = INTERVAL;
char name[IW_ESSID_MAX_SIZE + 1] = {0};
char name_old[IW_ESSID_MAX_SIZE + 1] = {0};


int get_status(int fd, struct iwreq *rqt){
	rqt->u.essid.pointer = name;
	rqt->u.essid.length = IW_ESSID_MAX_SIZE + 1;
	if (ioctl(fd, SIOCGIWESSID, rqt) == -1) {
		perror("ioctl");
		return 0;
	}
	return 1;
}

int status_change(int fd, struct iwreq *rqt){
	if (get_status(fd, rqt)){
		if (strcmp(name, name_old) ==0) return 0;
		memcpy(name_old, name, IW_ESSID_MAX_SIZE + 1);
		return 1;
	}
	return 0;	
}
void put_status(int fd, struct iwreq *rqt)
{
	if (status_change(fd, rqt)){
		if (*name == '\0')
			printf(format, "Disconnected");
		else
			printf(format, name);
		printf("\n");
		fflush(stdout);
	}
}
int main(int argc, char *argv[])
{
    char *interface = INTERFACE;
    bool snoop = false;

    int opt;
    while ((opt = getopt(argc, argv, "hsf:i:w:")) != -1) {
        switch (opt) {
            case 'h':
                printf("essid [-h|-s|-i INTERVAL|-f FORMAT|-w INTERFACE]\n");
                exit(EXIT_SUCCESS);
                break;
            case 'i':
                interval = atoi(optarg);
                break;
            case 's':
                snoop = true;
                break;
            case 'f':
                format = optarg;
                break;
            case 'w':
                interface = optarg;
                break;
        }
    }

    struct iwreq request;
    int sock_fd;
    memset(&request, 0, sizeof(struct iwreq));
    sprintf(request.ifr_name, interface);

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (snoop)
        while (true) {
            put_status(sock_fd, &request);
            sleep(interval);
            name[0] = '\0';
        }
    else
        put_status(sock_fd, &request);

    close(sock_fd);
    if (strlen(name) > 0)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
