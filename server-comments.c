// Cwk2: server.c - multi-threaded server using readn() and writen()

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include "rdwrn.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>
#include <malloc.h>


// thread function
void *client_handler(void *);
static void handler (int sig, siginfo_t *siginfo, void *context);
void send_ip_id(int sockN);
void send_hello(int);
void send_time(int socket);
void send_uname(int socket);
void send_upld_fs(int socket);
time_t get_cur_time();
void sendReqFile(int socket);



time_t start_time;

// you shouldn't need to change main() in the server except the port number
int main(void)
{
	//gets the startup time of the server, necessary for the correct operation of the signal handler
	start_time = get_cur_time();
	
	
    int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50031);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
	perror("Failed to listen");
	exit(EXIT_FAILURE);
    }
    // end socket setup

	//signal handler function call code
	//sigaction used to store info relating to handling of signal
	struct sigaction act; 
	//copies '\0'(null) to the act struct for each value in the struct.
	memset(&act, '\0', sizeof(act));
	
	act.sa_sigaction = &handler;

	act.sa_flags = SA_SIGINFO;
	//sets the sighandler to activate on the receipt of a sig int
	sigaction(SIGINT, &act, NULL);
	//prints and error and halts the program if the signal handler fails to activate
	if (sigaction(SIGTERM, &act, NULL) == 1){
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
	//end sighandler code
	

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");

	pthread_t sniffer_thread;
        // third parameter is a pointer to the thread function, fourth is its actual parameter
	if (pthread_create
	    (&sniffer_thread, NULL, client_handler,
	     (void *) &connfd) < 0) {
	    perror("could not create thread");
	    exit(EXIT_FAILURE);
	}
	//Now join the thread , so that we dont terminate before the thread
	//pthread_join( sniffer_thread , NULL);
	printf("Handler assigned\n");
    }

    // never reached...
    // ** should include a signal handler to clean up
	
    exit(EXIT_SUCCESS);
} // end main()

// thread function - one instance of each for each connected client
// this is where the do-while loop will go
void *client_handler(void *socket_desc)
{
    //Get the socket descriptor
    int connfd = *(int *) socket_desc;

  char input;
do{
	//takes in entered values from client choice menu and usesthem to decide which function should be called
	//does this while value entered is not 6 (exit choice value)
	size_t k;

	char receiveInpt[2];

	readn(connfd, (unsigned char *) &k, sizeof(size_t));
	readn(connfd, (unsigned char *) receiveInpt, k);

	input = receiveInpt[0];

	switch (input) {
	case '1':
		send_ip_id(connfd);
		break;
	case '2':
		send_time(connfd);
		break;
	case '3':
		send_uname(connfd);
		break;
	case '4':
		send_upld_fs(connfd);
		break;
	case '5':
		sendReqFile(connfd);
		break;
	case '6':
		printf("%d\n", input);
		break;
	default:
		break;
  }
  } while (input != '6');

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    // always clean up sockets gracefully
    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}  // end client_handler()
//handler function used on receipt of sigint signal
static void handler(int sig, siginfo_t *siginfo, void *context)
{
	//gets the current time on receipt of sigint signal
	time_t exit_time = get_cur_time();
	//take the start time away from the exit time to find out the total execution time
	time_t tot_exec_time = exit_time - start_time;
	//string used to output total execution time
	char tot_exec_time_str[15];
	//formats the time stored in the string to be displayed in minutes and seconds
	strftime(tot_exec_time_str, 15, "%M:%S", localtime(&tot_exec_time));

	printf("\ntotal execution time in minutes and seconds(mins:secs): %s\n", tot_exec_time_str);
	//indicates to the OS that the program has exited successfully (not due to error)
	exit(EXIT_SUCCESS);
	
}
//gets the current time/now
time_t get_cur_time()
{
	struct timeval tmvl;
	
	time_t curtime;

	gettimeofday(&tmvl, NULL);
	
	curtime=tmvl.tv_sec;
	
	return curtime;	
}

void send_time(int socket)
{
	//tm used to hold time information in a way that can be formatted with asctime
	struct tm *tm;
	//t used to hold time information it is retrieved 
	time_t t;

	//if time fails to get the current time
	if ((t = time(NULL)) == -1) {
		perror("time error");
		exit(EXIT_FAILURE);
	}

	//if formatted t can't be stored in tm
	if ((tm = localtime(&t)) == NULL) {
		perror("localtime error");
		return;
	}

	size_t payload_length = strlen(asctime(tm)) + 1;
	//sends info to client
	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
	writen(socket, (unsigned char *) asctime(tm), payload_length);


}

void send_ip_id(int sockN)
{

	//creates socket diagram file for use in function calls operating on sockets
    int sock_n_IP = socket(AF_INET, SOCK_DGRAM, 0);
	//used to hold socket information (e.g. ip address, port numbers etc)
    struct ifreq ifr;
	//sets socket type
    ifr.ifr_addr.sa_family = AF_INET;
	//set network port being used
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	//format the socket diagram file created by sock_n_IP
    ioctl(sock_n_IP, SIOCGIFADDR, &ifr);
	//closes file opened by sock_n_IP
    close(sock_n_IP);
	//used to store the combo of the server IP and the student ID in one string
    char ip_id[50] =  "IP: ";

    char *ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    strcat(ip_id, ip);

    strcat(ip_id, " Student ID: S1507940");
    size_t payload_length = strlen(ip_id) + 1;
    writen(sockN, (unsigned char *) &payload_length, sizeof(size_t));
    writen(sockN, (unsigned char *) ip_id, payload_length);
}

void send_uname(int socket)
{
	//creates uts structure
	struct utsname uts;
	//creates point for sending uts structure over the socket
	struct utsname *uts_point;
	//if uname function fails to get uts information
	if (uname(&uts) == -1) {
		perror("uname error");
		exit(EXIT_FAILURE);
  }

	//uts_point points to the memory address value of uts
	uts_point = &uts;
	
	size_t payload_length = sizeof(uts);
	
	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
	writen(socket, (unsigned char *) uts_point, payload_length);


	
} // end of send_uname()
//check if a file is a regular file or not
int check_reg(char *file)
{
	//stat struct used to hold file information
    struct stat sb;
	//result used to indicate whether a file is regular or not
    int result;

	//if the file cannot be checked
    if (stat(file, &sb) == -1) {
    	perror("check_reg stat error");
    	exit(EXIT_FAILURE);
    }

	switch (sb.st_mode & S_IFMT) {
	//if the file is regular
	case S_IFREG:
		//1 indicates the file is regular
		result = 1;
		break;
	default:
		//0 indiciates the file is not regular
		result = 0;
		break;
}

	return result;

}

void send_upld_fs(int socket)
{
	//used to hold the individual names of files in the upload directory
	struct dirent **namelist;
	//used to hold the string of files in the upload directory
	char *regfiles;
	int n;
	int add;
	//indicates to the client whether or not the upload directory can be scanned
	char dir_error[4] = "no";
	size_t payload_length;
	//if the upload directory can't be scanned
	if ((n = scandir("./upload", &namelist, NULL, alphasort)) == -1){
		//indicate there is a directory scan error
		strcpy(dir_error, "yes");
		//tells the client there is a dir error
		payload_length = sizeof(dir_error);

		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) dir_error, payload_length);

	}else{
		//gives regfiles a memory allocation size of 15 bytes on the heap. 15 bytes used for testing purposes
		regfiles = malloc(15); 
		strcpy(regfiles, "");
		//tells the client there isn't a dir error
		payload_length = sizeof(dir_error);
		
		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) dir_error, payload_length);
		//while there are still files left to process
		while (n--) {
		
			//filepath used to allow check_reg function to locate file
			char filepath[10] = "./upload/";
			//gets the next file in the upload directory
			char *file = namelist[n]->d_name;
		
			//if add = 1 the file is regular, otherwise the file isn't
			add = check_reg(strcat(filepath, file));
			//if the file is regular
		  	if (add){
				//add the token value for splitting file values
				strcat(file, "|");
				//if the length of the string list of files and the filename is more than the available
				//memory allocated to the regfiles pointer
				if ((int) strlen(regfiles) + (int) strlen(file) >= malloc_usable_size(regfiles)){
					//assigns the value of regfiles to regfiles_resz with double the allocated memory
					char *regfiles_resz = realloc(regfiles, (int) malloc_usable_size(regfiles) * 2);
					//gives the value of regfiles_resz to regfiles
					regfiles = regfiles_resz;
				}
				//joins the filename with the string list of files
				strcat(regfiles, file);
			}

		//frees the current namelist value from the heap
		free(namelist[n]);
		}
	}
	//frees the namelist from the heap
	free(namelist);
	//used to send the string length of regilfes to the client to allow the appropriate amount of memory to
	//the receiving string
	int slength = strlen(regfiles);
	//used to convert the int to a char array/string value as can't cast int to unsigned char *. could have 
	//just made slength of type long however no major effect on function of program so decided to not 
	//implement change. slen_send given 90 bytes of memory which would = 90 digit char array, program should
	//never get to a number this size.
	char slen_send[90]; 
	//converts the int in slength to a char array, the converted value is then stored in slen_send
	sprintf(slen_send, "%d", slength);
	

	payload_length = sizeof(slen_send);

	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
	writen(socket, (unsigned char *) slen_send, payload_length);		

	payload_length = slength;

	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
	writen(socket, (unsigned char *) regfiles, payload_length);
	//memory allocated for regfiles in the heap is released
	free(regfiles);
}

void sendReqFile(int socket){

	int payload_length;
	//fileNm given the input memory size from client, file name size should never be over this value because
	//of this
	char fileNm[128];
	char file_error[4] = "no";
	char filePath[10] = "./upload/";
	//stores the contents of the file read
	char *fileContent;
	long flength;
	//gets the requested file name from the client and stores in fileNm variable
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
    readn(socket, (unsigned char *) fileNm, payload_length);
	//concatenates the filename to the filepath so the correct file can be sourced
	strcat(filePath, fileNm);
	//opens the request file by following the file path from the sever executables working directory
	//"r" tells the program to read the files contents and "b" tells the program to open the file in binary
	//mode, ensure no ASCII transformations etc.
	FILE *file_point = fopen(filePath, "rb");
	

	//if file can be opened
	if (file_point){
		
		payload_length = sizeof(file_error);
		//tells the client there isn't an error with opening the file
		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) file_error, payload_length);
		//finds the end of the file contents and sets the position indicator to this point
		fseek(file_point, 0, SEEK_END);
		//gets the file length through the position indicator
		flength = ftell(file_point);
		//finds the beginning of the file
		fseek(file_point, 0, SEEK_SET);
		//allocates the fileContent string storage variable the size of the file contents length.
		fileContent = malloc(flength);
		//reads file content from the start to the end of the file, this content is then stored in 
		//fileContent variable
		fread(fileContent, 1, flength, file_point);
		//closes the file and free memory allocated to the file_point pointer
		fclose(file_point);
		//sends the file content string length to the client
		payload_length = sizeof(flength);

		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) flength, payload_length);	
		//sends the file content to the client
		payload_length = flength;
				
		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) fileContent, payload_length);	

  	} else {
			//stores the indicator that there is a file error in file_error then sends this to the client
			strcpy(file_error, "yes");

			payload_length = sizeof(file_error);

			writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
			writen(socket, (unsigned char *) file_error, payload_length);
	}
}
