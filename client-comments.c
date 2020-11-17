// Cwk2: client.c - message length headers with variable sized payloads
//  also use of readn() and writen() implemented in separate code module

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "rdwrn.h"
#include <sys/utsname.h>




#define INPUTSIZE 128

//function used to print the servers Student ID and IP information
void get_id_ip(int socket)
{
	//ip_id used to store received ip and id string combo, allocated 32 bytes
	char ip_id[32];
	//used to get the size of payload coming from the server
	size_t k;
	//gets Student ID and IP information from the server and assigns to ip_id 
	readn(socket, (unsigned char *) &k, sizeof(size_t));
	readn(socket, (unsigned char *) ip_id, k);
	//prints set ip_id string
	printf("%s\n", ip_id);
}
//function used to print the servers date and time information
void get_time(int socket)
{
	//time_str used to store received time string, allocated 32 bytes of memory on stack
	char time_str[32];
	//used to get the size of payload coming from the server
	size_t k;
	//gets time information string from the server and assigns to time_str 
	readn(socket, (unsigned char *) &k, sizeof(size_t));
	readn(socket, (unsigned char *) time_str, k);
	//prints set time string
	printf("%s\n", time_str);
}
//function used to print the servers uname information
void get_uname(int socket)
{
	//pointer to utsname struct used to hold the utsname received from the server
	struct utsname *uts = 0;
	//used to get the size of payload coming from the server
	size_t payload_length;
	//assigns 1000 bytes of allocated memory to uts on heap, this should be more than enough for a
	//populated utsname struct
	uts = malloc(1000);
	//gets utsname struct from the server and assigns to uts pointer 
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
 	readn(socket, (unsigned char *) uts, payload_length);

	//prints utsname struct information
	printf("Node name:     %s\n", uts->nodename);
	printf("System name:   %s\n", uts->sysname);
	printf("Release:       %s\n", uts->release);
	printf("Version:       %s\n", uts->version);
	printf("Machine:       %s\n", uts->machine);
	//frees memory occupied by utsname within the heap
	free(uts);

	
}
//function used to print the files present within the servers upload directory
void get_upld_fs(int socket){
	char *regfiles;
	//defines the token that will be used to split each individual file while outputting the file list
	const char tokrep[2] = "|";
	char *tok;
	char dir_error[4];
	//used to get the size of payload coming from the server
	size_t payload_length = 0;
	//gets info on whether or not the upload directory exists within the server directory
	//this info is then sotred in dir_error, can either be "yes" or "no"
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
	readn(socket, (unsigned char *) dir_error, payload_length);


  	//if dir_error == yes
  	if (!strcmp(dir_error, "yes")){
		printf("error: no upload directory exists on the server\n");
  	}else{
    	printf("Regular files in the servers upload dir:\n");
		//slength used to store the length of the string being received from the server
    	int slength = 0;
		//used to get the string length value in char array/string format, used this as 
		//there were issues with using the slength int variable. the length of the string
		//should never be over or reach 90 so 90 bytes allocated on the stack should be more
		//than enough
		char slen_get[90];
		//gets string length information in string format
		readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
		readn(socket, (unsigned char *) slen_get, payload_length);
		//converts string length information to int and stores in slength int variable
		slength = atoi(slen_get);

		//regfiles used to store the list of files string, allocated the total length value of the string
		//being received on the heap. 1 char = 1 byte so should always be enough to hold the entire file
		//string
		regfiles = malloc(slength);
		//payload length will be the slength value as 1 char = 1 byte
		payload_length = slength;
		

      	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
      	readn(socket, (unsigned char *) regfiles, payload_length);

	//if no files are present
	if(slength == 0){
		printf("Error: No regular files present in the upload directory"
        " or the regular files are unreadable\n");
	} else{
		//used to hold individual file strings, split by tokrep
		tok = strtok(regfiles, tokrep);
		//while the token value is still present in the overall string
		while(tok != NULL){
			//prints off the current value held in tok
			printf( " %s\n", tok );
			//changes value in tok to next value in file string
			tok = strtok(NULL, tokrep);
			}
		}
	}
}

//function used to get a specified file from the servers upload directory
void get_file(int socket){
	//struct used to store received file information
	struct file_info{
	char fileNm[INPUTSIZE];
	char *fileContents;
	};
	//creates a new file_info struct
	struct file_info fl_get;
	//used to store values representing whether or not the file was found
	char file_error[4];
	
	printf("Enter the name of the file to retreive: ");
	//get the file name from the user and stores in the fileNm variable in the fl_get struct
	fgets(fl_get.fileNm, INPUTSIZE, stdin);
	//replaces the \n automatically inserted through user input with null terminator
	fl_get.fileNm[strcspn(fl_get.fileNm, "\n")] = 0;
	
	int payload_length = sizeof(fl_get.fileNm);
	//sends the fl_get.fileNm value to the server
  	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
  	writen(socket, (unsigned char *) fl_get.fileNm, payload_length);
	//resets payload length value
	payload_length = 0;
	//gets information on whether file exists and stores in file_error variable
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
    readn(socket, (unsigned char *) file_error, payload_length);
	
	//if file_error == "yes"
	if (!(strcmp(file_error, "yes"))){
		printf("file doesn't exist or is unreadable\n");
	} else {
		//used to indiciate the length of the files string contents. Long used as type can be cast to
		//unsigned char *
		long flength = 0;
		
		readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
		readn(socket, (unsigned char *) flength, payload_length);

		//allocates fileContents variable in fl_get the length of the file string contents in bytes
		//1 char = 1 byte
		fl_get.fileContents = malloc(flength);
		
		readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
		readn(socket, (unsigned char *) fl_get.fileContents, payload_length);
		//creates a new file with the received file name value. "w" indicates to the fopen function 
		//that the program wants to write to the file (overwrites any existing content) 
		//while "+" indicates to the fopen function that it should automatically create the file specified
		//if it doesn't already exist
		FILE * file_point = fopen(fl_get.fileNm, "w+");
		//writes the received fileContents string to the open file
		fputs(fl_get.fileContents, file_point);
		//closes the open file. double free error received when trying to free file_point after calling
		//fclose so assumption made that fclose also frees file_point pointer
		fclose(file_point);
	}

}
//displays the function choice menu
void displaymenu()
{
    printf("1. Display server IP and student ID\n");
    printf("2. Display server time\n");
    printf("3. Display uname information\n");
    printf("4. Display server upload dir files\n");
	printf("5. Get specified file from server and save in client dir\n");
    printf("6. Exit\n");
}

//first half controls server connection, most relevant comments for this section left in.
int main(void)
{
    // *** this code down to the next "// ***" does not need to be changed except the port number
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Error - could not create socket");
	exit(EXIT_FAILURE);
}

    serv_addr.sin_family = AF_INET;

    // IP address and port of server we want to connect to
    serv_addr.sin_port = htons(50031);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // try to connect...
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	perror("Error - connect failed");
	exit(1);
    } else
       printf("Connected to server...\n");

    // *** make sure sockets are cleaned up


    char name[INPUTSIZE];
    char input;
    do {
        displaymenu();
      	printf("option> ");	
		// get the menu choice from input, then store in name variable
      	fgets(name, INPUTSIZE, stdin);
		//remove the newline character from name
      	name[strcspn(name, "\n")] = 0;
		//input = the first character entered when inputting menu choice
        input = name[0];
      	if (strlen(name) > 1)
			// set invalid if input more than 1 char
      	    input = 'x';	
		//sets the null terminator for sendInpt. Changed during commentary from 2 to 1 which was
		//used out of error. Still 2 in original as doesn't effect functionality of the overall program
        char sendInpt[1] = "\0";
        sendInpt[0] = input;
        size_t n = sizeof(char);

        writen(sockfd, (unsigned char *) &n, sizeof(size_t));
        writen(sockfd, (unsigned char *) sendInpt, n);


        switch (input) {
        	case '1':
              get_id_ip(sockfd);
              break;
          case '2':
              get_time(sockfd);
              break;
          case '3':
              get_uname(sockfd);
              break;
          case '4':
              get_upld_fs(sockfd);
              break;
		  case '5':
			  get_file(sockfd);
			  break;
          case '6':
              printf("Goodbye!\n");
              break;
		  default:
        	  printf("Invalid choice - choose an option from 1 to 6!\n");
        	  break;
      	}
} while (input != '6');
	//closes the connection to the server
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    exit(EXIT_SUCCESS);
} // end main()
