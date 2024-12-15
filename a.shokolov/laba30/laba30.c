#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>

#define SOCKFILE_NAME "./30.socket"

void myexit(const char *msg, int sd) {
    perror(msg);
    unlink(SOCKFILE_NAME);
    close(sd);
    exit(EXIT_FAILURE);
}

struct sockaddr_un buildsockname() {
    struct sockaddr_un sockname;
    sockname.sun_family = PF_UNIX;
    strcpy(sockname.sun_path, SOCKFILE_NAME);
    return sockname;
}

int sockcreate() {
    errno = 0;
    int sd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(sd == -1) {
        myexit("socket isn't created", -1);
    }
    return sd;
}

void sockconnect(int sd) {
    struct sockaddr_un sockname = buildsockname();

    errno = 0;
    if(connect(sd, (struct sockaddr *)&sockname, sizeof(sockname)) == -1) {
        myexit("connection failed", sd);
    }
}

void sockwritemsg(int sd, char *msg) {
    errno = 0;
    if(write(sd, msg, strlen(msg) + 1) == -1) {
        myexit("writing failed", sd);
    }
}

void sockbind(int sd) {
    struct sockaddr_un sockname = buildsockname();

    errno = 0;
    if(bind(sd, (struct sockaddr *)&sockname, sizeof(sockname)) == -1) {
        myexit("socket binding failed", sd);
    }
}

void socklisten(int sd) {
    errno = 0;
    if(listen(sd, 1) == -1) {
        myexit("listen failed", sd);
    }
}

int sockaccept(int sd) {
    errno = 0;
    int clientsd = accept(sd, NULL, NULL);
    if(clientsd == -1) {
        myexit("accept failed", sd);
    }
    return clientsd;
}

void msguppercase(int sd) {
    char buff[255];
    errno = 0;
    while(read(sd, buff, 255) != 0) {
        printf("A message \"%s\" was read\n", buff);
        char *ptr;
        for(ptr = buff; *ptr; ptr++) {
            *ptr = toupper(*ptr);
        }
        printf("The message was uppercased: \"%s\"\n", buff);
    }
}

void intclose(int sig) {
    unlink(SOCKFILE_NAME);
}

int main(int argc, char *argv[]) {

    //--------------------------Client-----------------------------
    if(argc > 1 && !strcmp(argv[1], "client")) {
        int clientsd = sockcreate();
        printf("Socket is created\n");

        sockconnect(clientsd);
        printf("Connected to a server\nPrint the message:\n");

        char msg[256] = "Default message";
        gets(msg);

        sockwritemsg(clientsd, msg);
        printf("Message \"%s\" is sent\n", msg);

        close(clientsd);
        printf("client socket is closed\n");
    }

    //--------------------------Server-----------------------------

    else {
        int sockdes = sockcreate();
        printf("Socket is created\n");

        sockbind(sockdes);
        printf("Socket is bound\n");

        signal(SIGINT, intclose);

        socklisten(sockdes);
        printf("Program is listening for a socket. Waiting for connections...\n");

        int clientsd = sockaccept(sockdes);
        printf("Got a connection\n");

        msguppercase(clientsd);

        unlink(SOCKFILE_NAME);
        printf("30.socket is removed\n");

        close(clientsd);
        printf("Reading socket is closed\n");

        close(sockdes);
        printf("The whole socket is closed\n");

    }

    return 0;

}
