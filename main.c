#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXSIZE 512
#define NAMESIZE 24

enum methods {
    JOIN,
    SEND,
    EXIT
};

struct messagepdu {
    enum methods method;
    char name[NAMESIZE];
    char message[MAXSIZE];
};

void sendMessage(struct messagepdu *msg, char **argv, int argc) {
    int csocket, i;
    struct sockaddr_in servaddr;

    if ((csocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    //start by 3 to avoid receiving duplicate messages
    for (i = 3; i < argc; i++) {

        memset(&servaddr, 0, sizeof(servaddr));

        // Filling server information
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(argv[i]);
        servaddr.sin_port = htons(atoi(argv[++i]));


        sendto(csocket, msg, sizeof(struct messagepdu),
               0, (const struct sockaddr *) &servaddr,
               sizeof(servaddr));
    }

    close(csocket);
}

//buffer: stdin, name : stdin, argv: classparams
void createAndSendMessage(char *buffer, char *name, char **argv, int argc) {
    short type;

    //check message type
    if (strncmp(buffer, "!JOIN", 5) == 0) type = JOIN;
    else if (strncmp(buffer, "!EXIT", 5) == 0) type = EXIT;
    else type = SEND;

    //create messagepdu
    struct messagepdu message;
    message.method = type;
    strncpy(message.name, name, sizeof(message.name));
    strncpy(message.message, buffer, sizeof(message.message));

    //send Message
    sendMessage(&message, argv, argc);
}

/*
 * Addresses have to inherit the following structures in order to be used by the application:
 * {a.b.c.d} {xxxxx} where the first parameter is the ipv4 address and the second parameter
 * is defining the desired port.
 * The first pair will be used for your own Peer.
 */
int main(int argc, char **argv) {
    int ssocket, retval;
    char buffer[MAXSIZE] = {0};
    char name[NAMESIZE];
    struct messagepdu receiveBuffer;
    struct sockaddr_in si_me;
    char *nameptr = malloc(NAMESIZE * sizeof(char));

    //check parameter count
    if (argc < 3) {
        printf("invalid amount of arguments\n");
        return EXIT_FAILURE;
    }

    //create udp socket
    if ((ssocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("udp socket binding failed\n");
        return EXIT_FAILURE;
    }

    //get username
    printf("Please enter a name to join the chat:\n");
    scanf("%s", nameptr);
    printf("%s joined the chat!\n", nameptr);

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(atoi(argv[2]));
    si_me.sin_addr.s_addr = inet_addr(argv[1]);

    //bind socket
    if (bind(ssocket, (struct sockaddr *) &si_me, sizeof(si_me)) == -1) {
        perror("bind failed\n");
        return EXIT_FAILURE;
    }

    //send join
    struct messagepdu joinmsg;
    joinmsg.method = JOIN;
    strncpy(joinmsg.name, name, sizeof(joinmsg.name));
    strncpy(joinmsg.message, "", sizeof(joinmsg.message));
    createAndSendMessage((char *) &joinmsg, name, argv, argc);

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
            return EXIT_FAILURE;
        } else if (retval == 0) {
            perror("timeout occured\n");
        } else {
            //Message received

            //messages received from udp
            if (FD_ISSET(ssocket, &s_rd)) {
                //udp package
                recvfrom(ssocket, &receiveBuffer, sizeof(struct messagepdu), 0, (struct sockaddr *) &si_me, 0);

                //handle message types
                if (strncmp(buffer, "!EXIT", 5) == 0) {
                    printf("%s ... has left the conversation.\n", receiveBuffer.name);
                } else if (strncmp(buffer, "!JOIN", 5) == 0) {
                    printf("%s ... has joined the conversation.\n", receiveBuffer.name);
                } else {
                    if (strlen(receiveBuffer.message) > 2)
                        printf("%s ... was send by %s.\n", receiveBuffer.message, receiveBuffer.name);
                }
            }

            //monitor keyboard input
            if (FD_ISSET(fileno(stdin), &s_rd)) {
                //Console input
                fgets(buffer, sizeof(buffer), stdin);
                if (strlen(buffer) > 1) {
                    printf("%s ... was read from stdin.\n", buffer);
                    if (strncmp(buffer, "!EXIT", 5) == 0) {
                        //EXIT conversation
                        break;
                    } else {
                        //SEND message
                        createAndSendMessage(buffer, nameptr, argv, argc);
                    }
                }
            }
        }
    }

    printf("Leaving conversation.");

    //send exit
    struct messagepdu exitmsg;
    exitmsg.method = EXIT;
    strncpy(exitmsg.name, name, sizeof(joinmsg.name));
    strncpy(exitmsg.message, "", sizeof(joinmsg.message));
    createAndSendMessage((char *) &exitmsg, name, argv, argc);

    close(ssocket);

    return EXIT_SUCCESS;
}

