#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

using namespace std;

#define PORT 8080
#define DONT_USE "wy=$?9q=z3QARXsQ"

int main()
{
    // declare variables
    int count;
    int sockopt = 1;
    int clientSize = 30;
    int sockfd , addrlen , new_socket , client_socket[clientSize] , activity, i , readMessage , sock, max_sd;
    struct sockaddr_in address;
    string users[clientSize];

    for (int i = 0; i < clientSize; ++i) {
        users[i] = DONT_USE;
    }

    //data buffer
    char buffer[1025];

    //set of socket descriptors
    fd_set socketdescriptor;

    //initialise all client_sockets
    for (i = 0; i < clientSize; i++)
    {
        client_socket[i] = 0;
    }

    //create a socket
    if( (sockfd = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("creating socket failure");
        exit(EXIT_FAILURE);
    }

    // set socket options
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sockopt,
                   sizeof(sockopt)) < 0 )
    {
        perror("setsockopt failure");
        exit(EXIT_FAILURE);
    }

    //specify attributes for socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //bind the socket to port
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failure");
        exit(EXIT_FAILURE);
    }

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(sockfd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port: %d \n", PORT);
    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Ready for connections");

    while(true)
    {
        //clear the set
        FD_ZERO(&socketdescriptor);

        //add socket to set
        FD_SET(sockfd, &socketdescriptor);
        max_sd = sockfd;

        for ( i = 0 ; i < clientSize ; i++)
        {
            //socket descriptor
            sock = client_socket[i];

            //if valid socket descriptor then add
            if(sock > 0)
                FD_SET( sock , &socketdescriptor);

            //highest file descriptor number
            if(sock > max_sd)
                max_sd = sock;
        }

        //wait for an activity of sockets
        activity = select( max_sd + 1 , &socketdescriptor , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        if (FD_ISSET(sockfd, &socketdescriptor))
        {
            // accept incoming connections
            if ((new_socket = accept(sockfd,
                                     (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept failure");
                exit(EXIT_FAILURE);
            }

            //print the new connection of a socket
            printf("New incoming connection. Socket fd: %d , ip: %s , port: %d\n" , new_socket ,
                    inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //add new socket to array
            for (i = 0; i < clientSize; i++)
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    if(count == clientSize-1){
                        string isBusy = "BUSY\n";
                        send(new_socket, isBusy.c_str(), strlen(isBusy.c_str()), 0);
                    }
                    else{
                        client_socket[i] = new_socket;
                        count++;
                        printf("Add socket on index: %d\n" , i);
                    }
                    break;
                }
            }
        }

        for (i = 0; i < clientSize; i++)
        {
            sock = client_socket[i];

            if (FD_ISSET( sock , &socketdescriptor))
            {
                // read incoming message
                if ((readMessage = read( sock , buffer, 1024)) == 0)
                {
                    // show disconnected host
                    getpeername(sock , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected. ip: %s. port: %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //remove username from server
                    users[sock] = DONT_USE;

                    //Close the socket and mark as 0 in list for reuse
                    close(sock);
                    client_socket[i] = 0;
                }

                //Handle requests from hosts
                else
                {
                    // set null on end of message
                    buffer[readMessage] = '\0';

                    // declare variables
                    string result = buffer;
                    string username;
                    const char* messageFromServer = "";
                    string message = "WHO-OK ";
                    const char  *isInUse = "IN-USE\n";
                    const char *badBody = "BAD-RQST-BODY\n";
                    const char *badHeader = "BAD-RQST-HEADER\n";
                    string chatMessage;
                    string toUser = "";
                    int sdTo = 0;

                    // first handshake
                    if (result.find("HELLO-FROM") != std::string::npos) {
                        for (int k = 0; k < clientSize; k++) {
                            // checks if username is already used
                            if(result.find(users[k]) != std::string::npos) {
                                messageFromServer = isInUse;
                                break;
                            }
                            // username cannot contain mentioned character
                            if(result.find("%") != std::string::npos) {
                                messageFromServer = badBody;
                                break;

                            }
                        }

                        if (messageFromServer == isInUse) {
                            messageFromServer = (isInUse);
                        }
                        else if (messageFromServer == badBody) {
                            messageFromServer = badBody;
                        }
                        else {
                            username = result.substr(11, result.length());
                            username.pop_back();
                            messageFromServer = ("HELLO " + username + "\n").c_str();
                            users[sock] = username;
                        }
                    }

                    // when hosts request to see all hosts on the server
                    else if (result.find("WHO\n") != std::string::npos) {
                        for (int j = 0; j < clientSize; j++) {
                            if(users[j] != DONT_USE) {
                                message += users[j] + ", ";
                            }
                        }
                        printf("message: %s\n",message.c_str());
                        message.pop_back();
                        message.pop_back();
                        message += "\n";
                        messageFromServer = message.c_str();
                    }


                    // when hosts wants to send a message to another host
                    else if (result.find("SEND ") != std::string::npos) {
                        chatMessage = result.substr((5), result.size());
                        for (int k = 0; k < clientSize; k++) {
                            if(chatMessage.find(users[k]) != std::string::npos) {
                                toUser = users[k];
                                sdTo = k;
                            }
                        }

                        if(users[sdTo] != DONT_USE){
                            chatMessage = "DELIVERY " + users[sock] + " " + chatMessage.substr(toUser.length() + 1, chatMessage.length());
                            send(sdTo, chatMessage.c_str(), strlen(chatMessage.c_str()), 0);
                            string sendOk = "SEND-OK\n";
                            messageFromServer = sendOk.c_str();
                        }
                        // if client is not logged in
                        else{
                            string isUnknown = "UNKNOWN\n";
                            messageFromServer = isUnknown.c_str();

                        }
                    }

                    // all other commands will cause a bad request
                    else if(result.find("falsecommand")  != std::string::npos) {
                        messageFromServer = badHeader;
                    }

                    // send back message that is suitable
                    printf("%s\n",buffer);
                    send(sock , messageFromServer , strlen(messageFromServer) , 0 );
                }
            }
        }
    }
} 
