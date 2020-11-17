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



void *client_handler(void *);
static void handler (int sig, siginfo_t *siginfo, void *context);
void send_ip_id(int sockN);
void send_hello(int);
void send_time(int socket);
void send_uname(int socket);
void send_upld_fs(int socket);
time_t get_cur_time();
void sendReqFile(int socket);

typedef struct {
    int id_number;
    int age;
    float salary;
} employee;

time_t start_time;


int main(void)
{
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


	struct sigaction act; 

	memset(&act, '\0', sizeof(act));

	act.sa_sigaction = &handler;

	act.sa_flags = SA_SIGINFO;

	sigaction(SIGINT, &act, NULL);

	if (sigaction(SIGTERM, &act, NULL) == 1){
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	


    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");

	pthread_t sniffer_thread;

	if (pthread_create
	    (&sniffer_thread, NULL, client_handler,
	     (void *) &connfd) < 0) {
	    perror("could not create thread");
	    exit(EXIT_FAILURE);
	}

	printf("Handler assigned\n");
    }
	
    exit(EXIT_SUCCESS);
} 
void *client_handler(void *socket_desc)
{
 
    int connfd = *(int *) socket_desc;

  char input;
do{

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

   
    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}
static void handler(int sig, siginfo_t *siginfo, void *context)
{
	time_t exit_time = get_cur_time();
	time_t tot_exec_time = exit_time - start_time;
	char tot_exec_time_str[15];
	strftime(tot_exec_time_str, 15, "%M:%S", localtime(&tot_exec_time));

	printf("\ntotal execution time in minutes and seconds(mins:secs): %s\n", tot_exec_time_str);

	exit(EXIT_SUCCESS);
	
}

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
  struct tm *tm;
  time_t t;


  if ((t = time(NULL)) == -1) {
    perror("time error");
    exit(EXIT_FAILURE);
  }

  if ((tm = localtime(&t)) == NULL) {
    perror("localtime error");
    return;
    }

  size_t payload_length = strlen(asctime(tm)) + 1;

  writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
  writen(socket, (unsigned char *) asctime(tm), payload_length);


}

void send_ip_id(int sockN)
{


    int sock_n_IP = socket(AF_INET, SOCK_DGRAM, 0);

    struct ifreq ifr;

    ifr.ifr_addr.sa_family = AF_INET;

    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

    ioctl(sock_n_IP, SIOCGIFADDR, &ifr);

    close(sock_n_IP);

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
	struct utsname uts;
	struct utsname *uts_point;

	if (uname(&uts) == -1) {
		perror("uname error");
		exit(EXIT_FAILURE);
  }

	
	uts_point = &uts;
	
	size_t payload_length = sizeof(uts);
	
	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
	writen(socket, (unsigned char *) uts_point, payload_length);


	
} 

int check_reg(char *file)
{

    struct stat sb;
    int result;


    if (stat(file, &sb) == -1) {
    	perror("check_reg stat error");
    	exit(EXIT_FAILURE);
    }

  switch (sb.st_mode & S_IFMT) {
  case S_IFREG:
	 result = 1;
	 break;
  default:
	 result = 0;
	 break;
}

  return result;

}

void send_upld_fs(int socket)
{

  struct dirent **namelist;
  char *regfiles;
  int n;
  int add;
  char dir_error[4] = "no";
  size_t payload_length;
	
  if ((n = scandir("./upload", &namelist, NULL, alphasort)) == -1){
	
    strcpy(dir_error, "yes");

    payload_length = sizeof(dir_error);

    writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
    writen(socket, (unsigned char *) dir_error, payload_length);

  }else{

	regfiles = malloc(15); 
	strcpy(regfiles, "");
    payload_length = sizeof(dir_error);

    writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
    writen(socket, (unsigned char *) dir_error, payload_length);

    while (n--) {
		

		char filepath[10] = "./upload/";
		char *file = namelist[n]->d_name;
		
		
		add = check_reg(strcat(filepath, file));
	
      	if (add){
			strcat(file, "|");

			if ((int) strlen(regfiles) + (int) strlen(file) >= malloc_usable_size(regfiles)){
				char *regfiles_resz = realloc(regfiles, (int) malloc_usable_size(regfiles) * 2);
				regfiles = regfiles_resz;
		}
			strcat(regfiles, file);
	}


      free(namelist[n]);
    }
  }
	free(namelist);

	int slength = strlen(regfiles);
	char slen_send[90]; 

	sprintf(slen_send, "%d", slength);
	

	payload_length = sizeof(slen_send);

	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
	writen(socket, (unsigned char *) slen_send, payload_length);		

	payload_length = slength;

	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
	writen(socket, (unsigned char *) regfiles, payload_length);
	
	free(regfiles);
}

void sendReqFile(int socket){

	int payload_length;
	char fileNm[128];
	char file_error[4] = "no";
	char filePath[10] = "./upload/";
	char *fileContent;
	long flength;
	
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
    readn(socket, (unsigned char *) fileNm, payload_length);

	strcat(filePath, fileNm);

	FILE *file_point = fopen(filePath, "rb");
	
	if (file_point){
		
		payload_length = sizeof(file_error);

		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) file_error, payload_length);

		fseek(file_point, 0, SEEK_END);
		flength = ftell(file_point);
		fseek(file_point, 0, SEEK_SET);
		
		fileContent = malloc(flength);
		
		fread(fileContent, 1, flength, file_point);
		
		fclose(file_point);
		
		payload_length = sizeof(flength);

		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) flength, payload_length);	
				
		payload_length = flength;
				
		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) fileContent, payload_length);	

  	} else {
		strcpy(file_error, "yes");

		payload_length = sizeof(file_error);

		writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
		writen(socket, (unsigned char *) file_error, payload_length);
	}
}
