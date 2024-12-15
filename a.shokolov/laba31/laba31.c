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
#include <poll.h>
#include <sys/mman.h>

#define SOCKFILE_NAME "./30.socket"
#define CONNCNT 15

#define flag char

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

void socklisten(int sd, int cnt) {
    errno = 0;
    if(listen(sd, cnt) == -1) {
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

char msguppercase(int sd) {
    char buff[BUFSIZ] = { 0 };
    flag isempty = 1;
    int msglen;
    // do {
    msglen = read(sd, buff, BUFSIZ - 1);
    if(msglen != 0) isempty = 0;
    printf("A message \"%s\" was received\n", buff);
    char *ptr;
    for(ptr = buff; *ptr; ptr++) {
        *ptr = toupper(*ptr);
    }
    printf("The message was uppercased: \"%s\"\n", buff);
    // } while(msglen == BUFSIZ);
    if(isempty) {
        return -1;
    }
    return 0;
}

void intclose(int sig) {
    unlink(SOCKFILE_NAME);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

    //--------------------------Client-----------------------------
    if(argc > 1 && !strcmp(argv[1], "client")) {
        int clientsd = sockcreate();
        printf("Socket is created\n");

        sockconnect(clientsd);
        printf("Connected to a server\nPrint the message:\n");

        char msg[256] = "Default message";
        while(gets(msg)) {
            sockwritemsg(clientsd, msg);
            printf("Message \"%s\" is sent\n", msg);
        }

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

        socklisten(sockdes, CONNCNT);
        printf("Program is listening for a socket. Waiting for connections...\n");

        int fdslen = CONNCNT + 2;
        struct pollfd fds[fdslen];
        fds[0].fd = sockdes;
        fds[0].events = POLLIN;
        fds[1].fd = fileno(stdin);
        fds[1].events = POLLIN;

        flag loop = 1;
        int fdcnt = 2;
        while(loop) {
            poll(fds, fdcnt, -1);
            for(int i = 0; i < fdcnt; i++) {
                if(fds[i].revents & POLLIN) {
                    if(fds[i].fd == sockdes) { 
                        fds[fdcnt].fd = sockaccept(sockdes);
                        fds[fdcnt].events = POLLIN;
                        fdcnt++;
                        printf("Got a connection: fdcnt:%d fd:%d\n", fdcnt, fds[fdcnt - 1].fd);
                        break;
                    }
                    else if(fds[i].fd == fileno(stdin)) {
                        char buf[BUFSIZ];
                        if(read(fds[i].fd, buf, BUFSIZ) == 0) loop = 0;
                    }
                    else {
                        printf("Some message is received\n");
                        if(msguppercase(fds[i].fd) == -1) {
                            close(fds[i].fd);
                            fdcnt--;
                            printf("Client %d disconnected. Current fdcnt is %d\n", fds[i].fd, fdcnt);
                            memmove(fds + i, fds + i + 1, (fdcnt - i) * sizeof(struct pollfd));
                            break;
                        }
                    }
                }
            }
        }

        for(int i = 0; i < fdcnt; i++) {
            close(fds[i].fd);
        }
        printf("All descriptors are closed\n");

        unlink(SOCKFILE_NAME);
        printf("30.socket is removed\n");

    }

    return 0;

}
