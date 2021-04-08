#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define IPADRESS 127.0.0.1
#define PORT 8080
#define MAXSIZE 512

enum methods {
    JOIN = 0,
    SEND = 1,
    EXIT = 2
};

typedef struct sendPDU {
    enum methods method;
    char name[24];
    char message[512];
};


struct sockaddr_in si_me, si_other;

void sendMessage(struct sendPDU *msg, char **argv) {
    int csocket;
    struct sockaddr_in servaddr;

    if ((csocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sendto(csocket, msg, sizeof(struct sendPDU),
           0, (const struct sockaddr *) &servaddr,
           sizeof(servaddr));

    close(csocket);
}

//buffer: stdin, name : stdin, argv: classparams
int createAndSendMessage(char *buffer, char *name, char **argv) {
    short type;

    //check message type
    if (strncmp(buffer, "!JOIN", 5) == 0) type = JOIN;
    else if (strncmp(buffer, "!EXIT", 5) == 0) type = EXIT;
    else type = SEND;

    //create sendPDU
    struct sendPDU message;
    message.method = type;
    strncpy(message.name, name, sizeof(message.name));
    strncpy(message.message, buffer, sizeof(message.message));

    //send Message
    sendMessage(&message, argv);
}

int main(int argc, char **argv) {
    //check parameter count
    if (argc != 3) {
        printf("invalid amount of arguments\n");
        exit(0);
    }

    int ssocket, csocket, i, retval, blen, slen = sizeof(si_other);
    int sockopval = 1;
    char buffer[MAXSIZE] = {0};
    struct sendPDU receiveBuffer;
    char name[24];
    char *nameptr = malloc(24 * sizeof(char));
    char *msgptr = malloc(512 * sizeof(char));

    /*SERVER*/

    //create udp socket
    if ((ssocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("udp socket binding failed\n");
    }

    printf("Please enter a name to join the chat:\n");
    scanf("%s", nameptr);
    printf("%s joined the chat!\n", nameptr);

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(atoi(argv[1]));
    si_me.sin_addr.s_addr = inet_addr("127.0.0.1");

    //bind socket
    if (bind(ssocket, (struct sockaddr *) &si_me, sizeof(si_me)) == -1) {
        perror("bind failed\n");
        exit(1);
    }

    //send join
    struct sendPDU joinmsg;
    joinmsg.method = JOIN;
    strncpy(joinmsg.name, name, sizeof(joinmsg.name));
    strncpy(joinmsg.message, "", sizeof(joinmsg.message));
    createAndSendMessage(&joinmsg, name, argv);

    //set filedescriptors
    fd_set s_rd;
    while (1) {
        //set fds for stdin and udp
        FD_SET(ssocket, &s_rd); //UDP
        FD_SET(fileno(stdin), &s_rd);   //STDIN
        retval = select(ssocket + 1, &s_rd, NULL, NULL, NULL);

        //print out result from select
        if (retval == -1) {
            perror("select failed\n");
            return 0;
        } else if (retval == 0) {
            perror("timeout occured\n");
        } else {
            //Message received

            if (FD_ISSET(ssocket, &s_rd)) {
                //udp package
                recvfrom(ssocket, &receiveBuffer, sizeof(struct sendPDU), 0, (struct sockaddr *) &si_me, 0);
                printf("%s ... was read from udp socket.\n", receiveBuffer.message);
            }

            if (FD_ISSET(fileno(stdin), &s_rd)) {
                //Console input
                fgets(buffer, sizeof(buffer), stdin);
                if (strlen(buffer) > 1) {
                    printf("%s ... was read from stdin.\n", buffer);
                    createAndSendMessage(buffer, nameptr, argv);
                }
            }
        }
    }

    close(ssocket);

    return 0;
}

