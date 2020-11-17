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


void get_id_ip(int socket)
{
  char ip_id[32];
  size_t k;

  readn(socket, (unsigned char *) &k, sizeof(size_t));
  readn(socket, (unsigned char *) ip_id, k);

  printf("%s\n", ip_id);
}

void get_time(int socket)
{
  char time_str[32];
  size_t k;

  readn(socket, (unsigned char *) &k, sizeof(size_t));
  readn(socket, (unsigned char *) time_str, k);

  printf("%s\n", time_str);
}

void get_uname(int socket)
{
	
	struct utsname *uts = 0;

	size_t payload_length;
	
	uts = malloc(1000);
	
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
 	readn(socket, (unsigned char *) uts, payload_length);


	printf("Node name:     %s\n", uts->nodename);
	printf("System name:   %s\n", uts->sysname);
	printf("Release:       %s\n", uts->release);
	printf("Version:       %s\n", uts->version);
	printf("Machine:       %s\n", uts->machine);

	free(uts);

	
}

void get_upld_fs(int socket){
  char *regfiles;
  const char tokrep[2] = "|";
  char *tok;
  char dir_error[4];
  size_t payload_length = 0;

  readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
  readn(socket, (unsigned char *) dir_error, payload_length);



  	if (!strcmp(dir_error, "yes")){
   	 printf("error: no upload directory exists on the server\n");
  	}else{
    	printf("Regular files in the servers upload dir:\n");
    	int slength = 0;
		char slen_get[90];
		
		readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
		readn(socket, (unsigned char *) slen_get, payload_length);

		slength = atoi(slen_get);

		
		regfiles = malloc(slength);
		payload_length = slength;
		

      	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
      	readn(socket, (unsigned char *) regfiles, payload_length);

      if(slength == 0){
        printf("Error: No regular files present in the upload directory"
        " or the regular files are unreadable\n");
        } 
      else{
      tok = strtok(regfiles, tokrep);

        while(tok != NULL){

          printf( " %s\n", tok );
          tok = strtok(NULL, tokrep);
        }
      }
    }
  }


void get_file(int socket){
	
	struct file_info{
	char fileNm[INPUTSIZE];
	char *fileContents;
	};

	struct file_info fl_get;
	char file_error[4];
	
	printf("Enter the name of the file to retreive: ");
	
	fgets(fl_get.fileNm, INPUTSIZE, stdin);

	fl_get.fileNm[strcspn(fl_get.fileNm, "\n")] = 0;

	int payload_length = sizeof(fl_get.fileNm);

  	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
  	writen(socket, (unsigned char *) fl_get.fileNm, payload_length);

	payload_length = 0;

	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
    readn(socket, (unsigned char *) file_error, payload_length);
	

	if (!(strcmp(file_error, "yes"))){
		printf("file doesn't exist or is unreadable\n");
	} else {
		long flength = 0;
		
		readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
		readn(socket, (unsigned char *) flength, payload_length);


		fl_get.fileContents = malloc(flength);
		
		readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
		readn(socket, (unsigned char *) fl_get.fileContents, payload_length);

		FILE * file_point = fopen(fl_get.fileNm, "w+");
	
		fputs(fl_get.fileContents, file_point);

		fclose(file_point);
	}

}

void displaymenu()
{
    printf("1. Display server IP and student ID\n");
    printf("2. Display server time\n");
    printf("3. Display uname information\n");
    printf("4. Display server upload dir files\n");
	printf("5. Get specified file from server and save in client dir\n");
    printf("6. Exit\n");
}


int main(void)
{

    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Error - could not create socket");
	exit(EXIT_FAILURE);
}

    serv_addr.sin_family = AF_INET;


    serv_addr.sin_port = htons(50031);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	perror("Error - connect failed");
	exit(1);
    } else
       printf("Connected to server...\n");

 


    char name[INPUTSIZE];
    char input;
    do {
        displaymenu();
      	printf("option> ");
      	fgets(name, INPUTSIZE, stdin);	

      	name[strcspn(name, "\n")] = 0;
        input = name[0];
      	if (strlen(name) > 1)
      	    input = 'x';	
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

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    exit(EXIT_SUCCESS);
} 
