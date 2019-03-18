//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux 
#include <stdio.h>
#include <string.h> //strlen 
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close 
#include <arpa/inet.h> //close 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <string>
#include <cstring>

using namespace std;

#define TRUE 1
#define PORT 8080
#define DONT_USE "wy=$?9q=z3QARXsQ"

int main(int argc , char *argv[])
{
    int count;
    int opt = TRUE;
    int max_clients = 30;
    int master_socket , addrlen , new_socket , client_socket[max_clients] , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;
    string users[max_clients];

    for (int l = 0; l < max_clients; ++l) {
        users[l] = DONT_USE;
    }
    
    char buffer[1025]; //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;


    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    //bind the socket to localhost port 8080
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while(TRUE)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++)
        {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket,
                                     (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
                    (address.sin_port));

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    if(count == max_clients-1){
                        string isBusy = "BUSY\n";
                        send(new_socket, isBusy.c_str(), strlen(isBusy.c_str()), 0);
                    }
                    else{
                        client_socket[i] = new_socket;
                        count++;
                        printf("Adding to list of sockets as %d\n" , i);
                    }
                    break;
                }
            }
        }

        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];


            if (FD_ISSET( sd , &readfds))
            {
                //Check if it was for closing , and also read the
                //incoming message
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //remove username from server
                    users[sd] = DONT_USE;


                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }

                //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end
                    //of the data read
                    buffer[valread] = '\0';

                    string result = buffer;
                    string username;
                    const char* messageFromServer = "";
                    string message = "WHO-OK ";

                    string chatMessage;
                    string toUser = "";
                    int sdTo = 0;

                    if (result.find("HELLO-FROM ") != std::string::npos) {

                        string isInUse = "IN-USE\n";
                        for (int k = 0; k < max_clients; k++) {
                            if(result.find(users[k]) != std::string::npos) {
                                messageFromServer = isInUse.c_str();
                                k = max_clients;
                            }
                        }

                        if (strncmp("IN-USE\n", messageFromServer, 6) != 0) {
                            username = result.substr(11, result.length());

                            // remove '\n' from username
                            username.pop_back();
                            messageFromServer = ("HELLO " + username + "\n").c_str();
                            users[sd] = username;
                        }
                    }


                    else if (result.find("WHO\n") != std::string::npos) {
                        for (int j = 0; j < max_clients; j++) {
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


                    else if (result.find("SEND ") != std::string::npos) {
                        chatMessage = result.substr((5), result.size());
                        for (int k = 0; k < max_clients; k++) {
                            if(chatMessage.find(users[k]) != std::string::npos) {
                                toUser = users[k];
                                sdTo = k;
                            }
                        }

                        if(users[sdTo] != DONT_USE){
                            chatMessage = "DELIVERY " + users[sd] + " " + chatMessage.substr(toUser.length() + 1, chatMessage.length());
                            send(sdTo, chatMessage.c_str(), strlen(chatMessage.c_str()), 0);
                            string sendOk = "SEND-OK\n";
                            messageFromServer = sendOk.c_str();
                        }
                        // if client is not logged in.
                        else{
                            string isUnknown = "UNKNOWN\n";
                            messageFromServer = isUnknown.c_str();

                        }
                    }

                    else{
                        messageFromServer = "BAD-RQST-BODY\n";
                    }

                    printf("%s\n",buffer);
                    send(sd , messageFromServer , strlen(messageFromServer) , 0 );
                }
            }
        }
    }
} 
