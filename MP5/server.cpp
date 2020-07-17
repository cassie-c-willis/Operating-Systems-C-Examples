/* Cassie Willis
 *
 * CS 4414 - Operating Systems
 * Spring 2018
 * 5-1-18
 *
 * MP5 - Simple FTP
 *
 * The purpose of this project is to become familiar with remote 
 * procedure calls, remote file systems, and FTP. The FTP processes
 * all basic user commands along with LIST. The return is any error
 * or return codes as specified in FTP protocol.
 * Refer to the writeup for complete details.
 *
 * Compile with MAKE
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <iostream>
using namespace std;

//the server-PI interprets commands, sends replies and directs its DTP to set up the data connection and transfer the data
//The Server has one command-line parameter: the port number upon which to listen and
//accept incoming control connections from Clients. 

//Server will include barebones of section 5.1 in FTP documentation except:
// In order to make FTP workable without needless error messages, the
//   following minimum implementation is required for all servers:
//         ~ TYPE - BINARY
//         ~ MODE - Stream
//         ~ STRUCTURE - File
//         ~ COMMANDS - USER, QUIT, PORT, TYPE, MODE, STRU, RETR, STOR, NOOP, LIST

int main(int argc, char *argv[])
{
  struct sockaddr_in sa; 
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //Create a socket for endpoint communication
  if (SocketFD == -1) { //Creation failed
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&sa, 0, sizeof sa);

  sa.sin_family = AF_INET;
  sa.sin_port = htons(atoi(argv[1]));  //host to network short -> user input >1024
  sa.sin_addr.s_addr = htonl(INADDR_ANY); //only accept from local host

  if (bind(SocketFD,(struct sockaddr *)&sa, sizeof sa) == -1) { //Bind the socket to an address
    perror("bind failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  if (listen(SocketFD, 10) == -1) { //Prepare for incoming connections (this has 10 max connections)
    perror("listen failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  char* buff = new char[256];
  char* command = new char[4];
  bool typeI = false;
  int SocketFD1;

  for (;;) {
    int ConnectFD = accept(SocketFD, NULL, NULL); //creates a new socket for each connection (for data) and removes is from listen queue 

    if (0 > ConnectFD) {
      perror("accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    else printf("%s\n", "220 Service ready for new user.");

    send(ConnectFD, "220\n", 4, 0); //Initial connection
    while(1) {
      recv(ConnectFD, buff, 256, 0);
      memcpy(&command, &buff, 4);

      //determine which command was called and handle the commands
      //Return Codes:
      // 200 -> successful command
      // 125 -> file status okay
      // 230 -> user is logged in
      // 220 -> ready for new user
      // 226 -> file action successful
      // 451 -> action aborted
      // 500 -> syntax error, command did not occur
      // 504 -> command not implemented for that parameter
      // Everything sends 200 on success, 500 on failure unless otherwise noted

      //USER is always valid, send 230
      if(strncmp("USER", command, 4) == 0) {
        send(ConnectFD, "230\n", 4, 0); 
      }
      //QUIT closes all client sockets, terminating command connection, send 221 because logout
      else if(strncmp("QUIT", command, 4) == 0) {
        send(ConnectFD, "221\n", 4, 0);
        break;
      }
      //PORT specifies the host and port to which the server should connect for the next file transfer, 200 on success
      //Syntax: "PORT a1,a2,a3,a4,p1,p2" is interpreted as IP address a1.a2.a3.a4, port p1*256+p2. 
      else if(strncmp("PORT", command, 4) == 0) {
        //separate the arguments
        char* tokens;
        vector<string> arguments;
        tokens = strtok(buff, " ");
        while(tokens != NULL) {
          arguments.push_back(tokens);
          tokens = strtok(NULL, ",");
        }
        // string newIP = arguments[1] + "." + arguments[2] + "." + arguments[3] + "." + arguments[4];
        // const char* ip = newIP.c_str();
        int port1 = atoi(arguments[5].c_str());
        int port2 = atoi(arguments[6].c_str());
        int overallPort = port1 * 256 + port2;

        struct sockaddr_in sa1;

        SocketFD1 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //Create a socket (an endpoint for communication)
        //Socket creation failed
        if (SocketFD1 == -1) {
          perror("cannot create socket");
          exit(EXIT_FAILURE);
        }

        memset(&sa1, 0, sizeof sa1);

        sa1.sin_family = AF_INET;
        sa1.sin_port = htons(overallPort);

        if (connect(SocketFD1, (struct sockaddr *)&sa1, sizeof sa1) == -1) { //connect to server
          perror("connect failed");
          close(SocketFD);
          exit(EXIT_FAILURE);
        }
        send(ConnectFD, "200\n", 4, 0);
        
      }
      //TYPE supports TYPE I command when "binary" types into prompt, send 200 if I, else send 504
      else if(strncmp("TYPE", command, 4) == 0) {
        if(strncmp("I", &buff[5], 1) == 0) {
          send(ConnectFD, "200\n", 4, 0);
          typeI = true;
        }
        else send(ConnectFD, "504 Command not implemented for that parameter\n", 47, 0);
      }
      //MODE should be stream, send 200 if S (stream), 504 otherwise
      else if(strncmp("MODE", command, 4) == 0) {
        if(strncmp("S", &buff[5], 1) == 0) send(ConnectFD, "200\n", 4, 0);
        else send(ConnectFD, "504 Command not implemented for that parameter\n", 47, 0);
      }
      //STRU with file (F) send 200, STRU R (else) returns “504 Command not implemented for that parameter”
      else if(strncmp("STRU", command, 4) == 0) {
        if(strncmp("F", &buff[5], 1) == 0) send(ConnectFD, "200\n", 4, 0);
        else send(ConnectFD, "504 Command not implemented for that parameter\n", 47, 0);
      }
      //RETR Begins transmission of a file from the remote host, send “451 Requested action aborted: local error in processing” if not in image mode
      //550 file not found error code
      else if(strncmp("RETR", command, 4) == 0) {
        if(typeI == false) send(ConnectFD, "451 Requested action aborted: local error in processing\n", 56, 0);
        else {
          send(ConnectFD, "125\n", 4, 0);

          char* tokens;
          vector<string> arguments;
          tokens = strtok(buff, " ");
          while(tokens != NULL) {
            arguments.push_back(tokens);
            tokens = strtok(NULL, " ");
          }

          char result[1000000];

          FILE *fd = fopen(arguments[1].c_str(), "r");
          if(fd == NULL) {
            send(ConnectFD, "550 File Not Found\n", 19, 0);
            close(SocketFD1);
            send(ConnectFD, "226\n", 4, 0);
            break;
          } 
          else {
            int i = 0;
            int c;
            while((c = fgetc(fd)) != EOF){
              if(c == '\n'){
                result[i++] = '\n';
                send(SocketFD1, result, i, 0);
                i = 0;
              }
              else result[i++] = c;
            }
            if(i > 0) {
              send(SocketFD1, result, i, 0);
            }    
          }
          fclose(fd);
          close(SocketFD1);
          send(ConnectFD, "226\n", 4, 0);
        }
      }
      //STOR Begins transmission of a file to the remote site, send “451 Requested action aborted: local error in processing” if not in image mode
      else if(strncmp("STOR", command, 4) == 0) {
        if(typeI == false) send(ConnectFD, "451 Requested action aborted: local error in processing\n", 56, 0);
        else {
          send(ConnectFD, "125\n", 4, 0); 

          char* tokens;
          vector<string> arguments;
          tokens = strtok(buff, " ");
          while(tokens != NULL) {
            arguments.push_back(tokens);
            tokens = strtok(NULL, " ");
          }

          char result[1000000];

          string open = arguments[1];
          FILE *fd = fopen(open.c_str(), "w");

          int info = recv(SocketFD1, result, 10000000, 0);

          for(int i = 0; i < info; i++) {
            fputc(result[i], fd);
          }
 
          fclose(fd);
          close(SocketFD1);
          send(ConnectFD, "226\n", 4, 0);
        }
      }
      //NOOP does nothing but return a response 200
      else if(strncmp("NOOP", command, 4) == 0) {
        send(ConnectFD, "200\n", 4, 0);
      }
      //LIST lists remote files (LS), 200, 500
      else if(strncmp("LIST", command, 4) == 0) {
        send(ConnectFD, "125\n", 4, 0); 
        char result[1000];
        FILE *ls = popen("ls -l", "r");

        int i = 0;
        int c;
        while((c = getc(ls)) != EOF){
          if(c == '\n'){
            result[i++] = '\r';
            result[i++] = '\n';
            send(SocketFD1, result, i, 0);
            i = 0;
          }
          else result[i++] = c;
        }    
        pclose(ls);
        close(SocketFD1);
        send(ConnectFD, "226\n", 4, 0);
      }
      //Not implemented, send 504
      else send(ConnectFD, "504 Command not implemented for that parameter\n", 47, 0);
      //clear the buffers for the next command
      memset(buff, 0, 256);
      memset(command, 0, 5);
    }

    if (shutdown(ConnectFD, SHUT_RDWR) == -1) {
      perror("shutdown failed");
      close(ConnectFD);
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    close(ConnectFD);
  }

  close(SocketFD);
  return EXIT_SUCCESS;  
}
