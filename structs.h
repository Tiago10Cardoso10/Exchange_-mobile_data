/*
	Tiago Rafael Cardoso Santos,2021229679
	Tomás Cunha da Silva,2021214124
*/

// Defines
#define DEBUG 0
#define MAX 512
#define LOGS "log.txt"
#define USER_PIPE "USER_PIPE"
#define BACK_PIPE "BACK_PIPE"

// Includes
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

typedef struct Phone_input {
    int id_user;
    int plafond_initial;
	int plafond_atual;
}Phone_input;

typedef struct stats {
    int nr_requests_video;
    int nr_requests_musica;
    int nr_requests_social;
	int video_total;
	int musica_total;
	int social_total; 
}stats;
	

typedef struct Config{
    int MAX_MOBILE_USERS;
    int QUEUE_POS;
    int AUTH_SERVERS_MAX;
    int AUTH_PROC_TIME;
    int MAX_VIDEO_WAIT;
    int MAX_OTHERS_WAIT;
}ConfigStruct;

typedef struct sharedmemory {
    Phone_input* phone;
    stats* est;
    int* auth_list;
}sharedmemory;

// Mensague queue
typedef struct {
    int id_user;
    char message[MAX];
} msgq;

// Informacao sobre as Video_Streaming_Queue e Others_Services_Queue
typedef struct queue {
    char* msg;
    int temp_wait;
    struct queue* next;
} queue;


// Variaveis globais
FILE* logs;
int terminate_threads;

// semaforos
sem_t* sem_log;
sem_t* sem_shm;
sem_t* sem_read;
sem_t* sem_authorization;

// threads
pthread_t receiver;
pthread_t sender;
pthread_t mobile_user_msgq_listener_thread;

int(*unnamed_pipes)[2];

//pids
pid_t pid_system_manager;
pid_t pid_mobile_user;
pid_t pid_backoffice;


// named pipes
int fd_user_pipe;
int fd_back_pipe;

// Message queue
int msgq_id;	
key_t key;

// Mutexes
pthread_mutex_t mutex_video_queue;
pthread_mutex_t mutex_others_queue;

 //memoria partilhada
int shmid;

ConfigStruct conf_file;
sharedmemory *smemory;
queue *video_queue;
queue *others_queue;

//Funcoes
void inicia_programa();
void mobile(int inputs[],int id);
void back_office(int id);
void termina_programa();
void escrever_arquivo(char* msg);
void system_manager(char* config_file,int id);
void inicia_memoria();
void sigint();
void sigtstp();
