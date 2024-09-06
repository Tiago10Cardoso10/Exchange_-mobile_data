/*
	Tiago Rafael Cardoso Santos,2021229679
	Tomás Cunha da Silva,2021214124
*/

#include "structs.h"

// inicializa o programa
void inicia_programa() {

	terminate_threads = 0;
	
    // abertura do ficheiro log
    logs = fopen(LOGS, "a");

    // inicializacao do semaforo para o fich log
    sem_unlink("SEM_LOG");
    sem_log = sem_open("SEM_LOG", O_CREAT | O_EXCL, 0700, 1);
    
    if (sem_log == SEM_FAILED) {
        escrever_arquivo("OPENING SEMAPHORE FOR AUTHORIZATION FAILED");
        termina_programa();
    }
    

	// inicializacao do semaforo para a memoria partilhada
    sem_unlink("SEM_SHM");
    sem_shm = sem_open("SEM_SHM", O_CREAT | O_EXCL, 0700, 1);
    
    if (sem_shm == SEM_FAILED) {
        escrever_arquivo("OPENING SEMAPHORE FOR AUTHORIZATION FAILED");
        termina_programa();
    }


#if DEBUG
    if (sem_log == SEM_FAILED)
        escrever_arquivo("[DEBUG] OPENING SEMAPHORE FOR LOG FAILED");
    else
        escrever_arquivo("[DEBUG] SEMAPHORE FOR LOG CREATED");

    if (sem_shm == SEM_FAILED)
        escrever_arquivo("[DEBUG] OPENING SEMAPHORE FOR SHARED MEMORY FAILED");
    else
        escrever_arquivo("[DEBUG] SEMAPHORE FOR SHARED MEMORY CREATED");
#endif

    // inicializacao do semaforo para a leitura do arquivo
    sem_unlink("SEM_READ");
    sem_read = sem_open("SEM_READ", O_CREAT | O_EXCL, 0700, 1);
    
    if (sem_read == SEM_FAILED) {
        escrever_arquivo("OPENING SEMAPHORE FOR AUTHORIZATION FAILED");
        termina_programa();
    }

    // inicializacao do semaforo para os unnamed pipes para cada Authorization Engine
    sem_unlink("SEM_AUTHORIZATION");
    sem_authorization = sem_open("SEM_AUTHORIZATION", O_CREAT | O_EXCL, 0700, 1);

    if (sem_authorization == SEM_FAILED) {
        escrever_arquivo("OPENING SEMAPHORE FOR AUTHORIZATION FAILED");
        termina_programa();
    }
    
     // inicializacao dos mutexes
    mutex_video_queue = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    mutex_others_queue = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    
    // inicializacao USER_PIPE
    if ((mkfifo(USER_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
        perror("mkfifo() failed");
        exit(1);
    }
    if ((fd_user_pipe = open(USER_PIPE, O_RDWR)) == -1) {
        escrever_arquivo("ERROR OPENING USER PIPE");
        exit(1);
    }

    // inicializacao BACK_PIPE
    if ((mkfifo(BACK_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
        escrever_arquivo("ERROR CREATING  PIPE");
        exit(1);
    }

    if ((fd_back_pipe = open(BACK_PIPE, O_RDWR)) == -1) {
        escrever_arquivo("ERROR OPENING BACK PIPE");
        exit(1);
    }
    
    inicia_memoria();
}

// inicializa a memoria partilhada
void inicia_memoria() {
    // inicializacao da memoria partilhada
    int size = sizeof(sharedmemory) + sizeof(Phone_input) * conf_file.MAX_MOBILE_USERS + sizeof(stats) + sizeof(int*) * (conf_file.AUTH_SERVERS_MAX + 3) ;
   
    if ((shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777)) == -1) {
        escrever_arquivo("ERROR CREATING SHARED MEMORY");
        exit(1);
    }

    // attach da memoria partilhada
    if ((smemory = (sharedmemory*) shmat(shmid, NULL, 0)) == (sharedmemory*) -1) {
        escrever_arquivo("SHMAT ERROR");
        exit(1);
    }
    
#if DEBUG
    escrever_arquivo("[DEBUG] SHARED MEMORY CREATED");
#endif
}

// funcao para Receção de SIGINT - CTRL + C
void sigint(){
	printf("\n");
    escrever_arquivo("SIGNAL SIGINT RECEIVED");
    termina_programa();
}

void sigint_mb(){
	printf("\n");
    escrever_arquivo("SIGNAL SIGINT RECEIVED");
    exit(0);
}

// escreve no ficheiro log e no ecra
void escrever_arquivo(char* msg) {
    char* text = NULL;
    char* current_time = NULL;
    time_t t;
    time(&t);
    
    // obter tempo atual no formato "DIA_SEMANA MM DD HH:MM:SS YYYY"
    current_time = (char*) malloc(strlen(ctime(&t)) * sizeof(char) + 1);
    strcpy(current_time, ctime(&t));
    strtok(current_time, "\n");
    

    // obter a hora atual no formato "HH:MM:SS"
    char* hour = strtok(current_time, " ");
    for (int i = 0; i < 3; i++) {
        hour = strtok(NULL, " ");
    }

    // escrever no ficheiro log
    text = (char*) malloc((strlen(hour) + strlen(msg)) * sizeof(char) + 1);
    sprintf(text, "%s %s", hour, msg);
    sem_wait(sem_log);
    fprintf(logs, "%s\n", text);
    fflush(logs);
    sem_post(sem_log);
    printf("%s\n", text);

    // libertar memoria
    free(current_time);
    free(text);
}

// funcao que le o ficheiro de configuracao
void read_config(char* config_file) {
    FILE* file;
    char* text;
    long file_size;

    // abrir ficheiro de configuracao
    if ((file = fopen(config_file, "r")) == NULL) {
        perror("Error opening config file");
        exit(1);
    }

    // ir para o fim do ficheiro
    fseek(file, 0, SEEK_END);
    // obter o tamanho do ficheiro
    file_size = ftell(file);
    // voltar ao inicio do ficheiro
    rewind(file);

    // alocar memoria para o texto escrito no ficheiro de configuracao
    text = (char*) malloc(file_size + 1);

    // ler o ficheiro de configuracao
    fread(text, file_size, 1, file);
    // fechar o ficheiro de configuracao
    fclose(file);
    // adicionar o caracter de fim de string
    text[file_size] = '\0';

    // inicializar o semaforo
    sem_wait(sem_read);

    // obter cada linha do texto lido do ficheiro de configuracao e verificar se e um numero inteiro
    char* token = strtok(text, "\n\r");
    
    // guardar o numero de mobile users
    conf_file.MAX_MOBILE_USERS = atoi(token);

    // obter cada linha do texto lido do ficheiro de configuracao e verificar se e um numero inteiro
    int i = 0;
    while (token != NULL) {
        token = strtok(NULL, "\n\r");
        if (i == 0) {
            // guardar o numero de slots na fila
            conf_file.QUEUE_POS = atoi(token);
        }
        else if (i == 1) {
            // guardar o numero de Authorization Engines
            conf_file.AUTH_SERVERS_MAX = atoi(token);
        }
        else if (i == 2) {
            // guardar o periodo (em ms) que o Authorization Engine
            conf_file.AUTH_PROC_TIME = atoi(token);
        }
        else if (i == 3) {
            // guardar o tempo máximo (em ms) que os pedidos de video de autorização podem aguardar
            conf_file.MAX_VIDEO_WAIT = atoi(token);
        }
        else if (i == 4) {
            // guardar o tempo máximo (em ms) que os outros pedidos de autorização podem aguardar
            conf_file.MAX_OTHERS_WAIT = atoi(token);
        }
        i++;
    }
    // libertar o semaforo
    sem_post(sem_read);

    // libertar memoria
    free(text);
}

// Funcões para as Queues

// cria um novo no na internal queue
queue* create_new_Queue(char* msg, int temp_wait) {
    queue* temp = (queue*) malloc(sizeof(queue));
    temp->msg = (char*) malloc((strlen(msg) + 1) * sizeof(char));
    strcpy(temp->msg, msg);
    temp->temp_wait = temp_wait;
    temp->next = NULL;

    return temp;
}

// insere um novo no na internal queue
void push(queue** message_queue, char* msg, int temp_wait) {
    queue* start = (*message_queue);
    queue* temp = create_new_Queue(msg, temp_wait);

    //printf("PUSH: %s\n", msg);

    if ((*message_queue)->temp_wait > temp_wait) {
        temp->next = *message_queue;
        (*message_queue) = temp;
    }
    else {
        while (start->next != NULL && start->next->temp_wait < temp_wait) {
            start = start->next;
        }

        temp->next = start->next;
        start->next = temp;
    }
}

// retorna o tamanho da internal queue
int size(queue* message_queue) {
    int count = 0;
    queue* aux = message_queue;

    while (aux != NULL) {
        count++;
        aux = aux->next;
    }
    return count;
}

// verifica se a internal queue esta vazia
int is_empty(queue** message_queue) {
    return (*message_queue) == NULL;
}

// devolve a mensgaem que está naquela posicao
char* msg_in_queue(queue** message_queue) {
    return (*message_queue)->msg;
}

// apaga un nó da fila
void deleteNode(queue** message_queue) {
    if (message_queue == NULL || *message_queue == NULL)
        return;

    queue *temp = *message_queue;
    *message_queue = (*message_queue)->next;
    free(temp->msg);
    free(temp);
}
// Fim das Funções para Queue

// termina o programa
void termina_programa() {
    escrever_arquivo("5G_AUTH_PLATFORM SIMULATOR CLOSING");
    
    escrever_arquivo("PRINTING VIDEO QUEUE:");
    while (!is_empty(&video_queue)) {
        printf("%s\n", msg_in_queue(&video_queue));
        deleteNode(&video_queue);
    }
    escrever_arquivo("FINISHED PRINTING VIDEO QUEUE");
    
    escrever_arquivo("PRINTING OTHERS QUEUE:");
    while (!is_empty(&others_queue)) {
        printf("%s\n", msg_in_queue(&others_queue));
        deleteNode(&others_queue);
    }
    escrever_arquivo("FINISHED OTHRES OTHERS QUEUE");

	terminate_threads = 1;
	
    // terminar processos
    wait(NULL);

    // terminar semaforo do log
    sem_close(sem_log);
    sem_unlink("SEM_LOG");

    // terminar semaforo da memoria partilhada
    sem_close(sem_shm);
    sem_unlink("SEM_SHM");
    
    // terminar semaforo de leitura
    sem_close(sem_read);
    sem_unlink("SEM_READ");

	// terminar semaforo de authorization
    sem_close(sem_authorization);
    sem_unlink("SEM_AUTHORIZATION");

    // terminar memoria partilhada
    shmdt(smemory);
    shmctl(shmid, IPC_RMID, NULL);
    
    // fechar pipes
    close(fd_user_pipe);
    unlink(USER_PIPE);

    close(fd_back_pipe);
    unlink(BACK_PIPE);
    
    free(unnamed_pipes);
    
    // destroi mutexes
    pthread_mutex_destroy(&mutex_video_queue);
    pthread_mutex_destroy(&mutex_others_queue);
    
    // fechar message queue
    msgctl(msgget(ftok("msgfile", 'A'), 0666 | IPC_CREAT), IPC_RMID, NULL);
    
    // fechar ficheiro log
    fclose(logs);

    exit(0);
}

// funcao para o back_office
void back_office(int id) {
	if (getppid() == pid_backoffice) {
        printf("BACK OFFICE ALREADY RUNNING\n");
        exit(1);
    } else {
    	printf("BACK OFFICE RUNNING\n");
    }

    char command[MAX];
    char formatted_command[MAX];
    int fd_back_pipe;
    
    
    key_t key = ftok("msgfile", 'A');
    int msgq_id = msgget(key, 0666);
    msgq message;

    if (msgq_id == -1) {
        fprintf(stderr, "Erro ao recuperar a fila de mensagens.\n");
        exit(EXIT_FAILURE);
    }

    int max_msg_size = MAX;
    
    signal(SIGINT, sigint_mb);
    

    while (1) {
    	signal(SIGINT, sigint_mb);
        memset(command, 0, sizeof(command));
        memset(formatted_command, 0, sizeof(formatted_command));
        
        
        printf("> "); 
        fflush(stdout);
        if (fgets(command, sizeof(command), stdin) == NULL) {
            fprintf(stderr, "Error reading input.\n");
            continue;
        }
        
        command[strcspn(command, "\n")] = '\0';
        
        // Verificar se há mensagens na fila de mensagens
            	if (msgrcv(msgq_id, &message, max_msg_size, 0,IPC_NOWAIT) != -1) {
                	if (message.id_user == id) {
                		fprintf(stderr,"%s\n", message.message);
            		}
            	} 

        // Verificar o comando inserido
        if (strcmp(command, "reset") == 0 || strcmp(command, "data_stats") == 0) {
            // Limitar o tamanho máximo do comando formatado
            snprintf(formatted_command, sizeof(formatted_command), "1#%.*s", (int)(sizeof(formatted_command) - 3), command);

            if ((fd_back_pipe = open(BACK_PIPE, O_WRONLY | O_NONBLOCK)) == -1) {
                perror("Error opening back pipe");
                exit(EXIT_FAILURE);
            }
            if (write(fd_back_pipe, formatted_command, strlen(formatted_command) + 1) != -1) {
                // Verificar se há mensagens na fila de mensagens
            	if (msgrcv(msgq_id, &message, max_msg_size, 0,0) != -1) {
                	if (message.id_user == id) {
                		fprintf(stderr,"%s\n", message.message);
            		}
            	} 
            }

        } else {
            printf("Invalid command. Please enter 'reset' or 'data_stats'.\n");
        }
    }
}

//funcao para mobile
void mobile(int inputs[], int id) {
    int plafond_inicial = inputs[0];
    int max_requests = inputs[1];
    int video_interval = inputs[2];
    int music_interval = inputs[3];
    int social_interval = inputs[4];
    int data_to_reserve = inputs[5];

    char reg_msg[MAX];
    sprintf(reg_msg, "%d#%d", id, plafond_inicial);
    if ((fd_user_pipe = open(USER_PIPE, O_WRONLY)) == -1) {
        perror("Error opening user pipe");
        exit(EXIT_FAILURE);
    }
    if (write(fd_user_pipe, reg_msg, strlen(reg_msg) + 1) == -1) {
        perror("Error writing to user pipe");
        exit(EXIT_FAILURE);
    }
    close(fd_user_pipe);
    
    printf("ENVIADO: %s\n", reg_msg);

    int request_count = 0;
    int timer = 1;
    
    
    key_t key = ftok("msgfile", 'A');
    int msgq_id = msgget(key, 0666);
    msgq message;

    if (msgq_id == -1) {
        fprintf(stderr, "Erro ao recuperar a fila de mensagens.\n");
        exit(EXIT_FAILURE);
    }

	signal(SIGINT, sigint_mb);
	int max_msg_size = MAX;

    while (request_count < max_requests && plafond_inicial >= data_to_reserve) {
    	signal(SIGINT, sigint_mb);
        
        // Verificar se há mensagens na fila de mensagens
        if (msgrcv(msgq_id, &message, max_msg_size, 0,IPC_NOWAIT) != -1) {
            if (message.id_user == id) {
            	fprintf(stderr,"%s\n", message.message);
        	}
        }

        // Pedido de autorização para vídeo
        if (timer % video_interval == 0) {
            char video_msg[MAX];
            sprintf(video_msg, "%d#VIDEO#%d", id, data_to_reserve);
            if ((fd_user_pipe = open(USER_PIPE, O_WRONLY | O_NONBLOCK)) == -1) {
                perror("Error opening user pipe for video");
                exit(EXIT_FAILURE);
            }
            if (write(fd_user_pipe, video_msg, strlen(video_msg) + 1) == -1) {
                perror("Error writing to user pipe for video");
                exit(EXIT_FAILURE);
            } else {
                request_count++;
                plafond_inicial -= data_to_reserve; // Deduz o valor da data reservada do plafond inicial
            }
            close(fd_user_pipe);
            printf("ENVIADO: %s\n", video_msg);
        }

        // Pedido de autorização para música
        if (timer % music_interval == 0) {
            char music_msg[MAX];
            sprintf(music_msg, "%d#MUSIC#%d", id, data_to_reserve);
            if ((fd_user_pipe = open(USER_PIPE, O_WRONLY | O_NONBLOCK)) == -1) {
                perror("Error opening user pipe for music");
                exit(EXIT_FAILURE);
            }
            if (write(fd_user_pipe, music_msg, strlen(music_msg) + 1) == -1) {
                perror("Error writing to user pipe for music");
                exit(EXIT_FAILURE);
            } else {
                request_count++;
                plafond_inicial -= data_to_reserve; // Deduz o valor da data reservada do plafond inicial
            }
            close(fd_user_pipe);
            printf("ENVIADO: %s\n", music_msg);
        }

        // Pedido de autorização para redes sociais
        if (timer % social_interval == 0) {
            char social_msg[MAX];
            sprintf(social_msg, "%d#SOCIAL#%d", id, data_to_reserve);
            if ((fd_user_pipe = open(USER_PIPE, O_WRONLY | O_NONBLOCK)) == -1) {
                perror("Error opening user pipe for social");
                exit(EXIT_FAILURE);
            }
            if (write(fd_user_pipe, social_msg, strlen(social_msg) + 1) == -1) {
                perror("Error writing to user pipe for social");
                exit(EXIT_FAILURE);
            } else {
                request_count++;
                plafond_inicial -= data_to_reserve; // Deduz o valor da data reservada do plafond inicial
            }
            close(fd_user_pipe);
            printf("ENVIADO: %s\n", social_msg);
        }

        sleep(1); // Espera 1 segundo
        timer++; // Incrementa o contador de tempo
    }
	if(request_count >= max_requests){
		printf("YOU REACHED TOTAL REQUESTS\n");
	} else {
		printf("YOU REACHED TOTAL PLAFOND\n");
	}
}

// função para controlo de estados
void ver_stats(){
	fprintf(stderr,"N Video: %d - T Video: %d\n",	smemory->est->nr_requests_video,    smemory->est->video_total);
	fprintf(stderr,"N Musica: %d - T Musica: %d\n",	smemory->est->nr_requests_musica,    smemory->est->musica_total);
	fprintf(stderr,"N SOCIAL: %d - T SOCIAL: %d\n",	smemory->est->nr_requests_social,    smemory->est->social_total);
}


// thread receiver
void* receiver_func() {
    int received_length;
    char received[MAX];

	if (fd_user_pipe == -1 || fd_back_pipe == -1) {
        perror("Erro ao abrir os named pipes");
        exit(EXIT_FAILURE);
    }
	
    escrever_arquivo("THREAD RECEIVER CREATED");

    while (!terminate_threads) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd_user_pipe, &read_fds);
        FD_SET(fd_back_pipe, &read_fds);

        // Use select para monitorar os descritores de arquivo
        int max_fd = (fd_user_pipe > fd_back_pipe) ? fd_user_pipe : fd_back_pipe;
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            if (errno != EINTR) {
                perror("Erro em select");
                exit(EXIT_FAILURE);
            }
        }
        
        // Verifique se há atividade no descritor de arquivo do user
        if (FD_ISSET(fd_user_pipe, &read_fds)) {
            // Leitura do pedido do named pipe do user
            received_length = read(fd_user_pipe, received, sizeof(received));
            received[received_length - 1] = '\0';

            printf("User Pipe: %s\n",received);
            
            // Analise o pedido com base no formato
    		char *token = strtok(received, "#");
            int id = atoi(token);
            token = strtok(NULL, "#");
		
            if (atoi(token)) {
        		// rocessar pedido de login (345#800)
                int plafond = atoi(token);
                char others_msg[50]; 
                sprintf(others_msg, "%d#%d", id,plafond);
    
                pthread_mutex_lock(&mutex_others_queue);

                if (size(others_queue) > conf_file.QUEUE_POS) {
                    char text[MAX + 30];
                    sprintf(text, "QUEUE FULL ORDER %s DELETED", others_msg);
                    escrever_arquivo(text);
                } else {
                    if (!is_empty(&others_queue)) {
                        push(&others_queue, others_msg, 0);
                    } else {
                        others_queue = create_new_Queue(others_msg, 0);
                    }
                }

                pthread_mutex_unlock(&mutex_others_queue);

            } else if (strcmp(token, "VIDEO") == 0) {
        		// Processar pedido de vídeo (345#VIDEO#40)
                token = strtok(NULL, "#");
                int data_to_reserve = atoi(token);

                char video_msg[50]; 
                sprintf(video_msg, "%d#VIDEO#%d", id, data_to_reserve);

                pthread_mutex_lock(&mutex_video_queue);

                if (size(video_queue) > conf_file.QUEUE_POS) {
                    char text[MAX + 30];
                    sprintf(text, "QUEUE FULL ORDER %s DELETED", received);
                    escrever_arquivo(text);
                } else {
                    if (!is_empty(&video_queue)) {
                        push(&video_queue, video_msg, 0);
                    } else {
                        video_queue = create_new_Queue(video_msg, 0);
                    }
                }

                pthread_mutex_unlock(&mutex_video_queue);

            } else if (strcmp(token, "MUSIC") == 0 || strcmp(token, "SOCIAL") == 0) {
    			// Processar pedido de vídeo (345#MUSIC#40 || 345#SOCIAL#40 )
                char type[20];
                if(strcmp(token, "MUSIC")==0){
                    sprintf(type,"MUSIC");
                } else {
                    sprintf(type,"SOCIAL");
                }

                token = strtok(NULL, "#");
                int data_to_reserve = atoi(token);

                char others_msg[50]; 
                sprintf(others_msg, "%d#%s#%d", id, type,data_to_reserve);

                pthread_mutex_lock(&mutex_others_queue);

                if (size(others_queue) > conf_file.QUEUE_POS) {
                    char text[MAX + 30];
                    sprintf(text, "QUEUE FULL ORDER %s DELETED", others_msg);
                    escrever_arquivo(text);
                } else {
                    if (!is_empty(&others_queue)) {
                        push(&others_queue, others_msg, 0);
                    } else {
                        others_queue = create_new_Queue(others_msg, 0);
                    }
                }

                pthread_mutex_unlock(&mutex_others_queue);
            }
        }

        // Verifique se há atividade no descritor de arquivo de back office
        if (FD_ISSET(fd_back_pipe, &read_fds)) {
            // Leitura do pedido do named pipe do back office
            received_length = read(fd_back_pipe, received, sizeof(received));
            received[received_length - 1] = '\0';

            printf("Back Pipe: %s\n", received);

            pthread_mutex_lock(&mutex_others_queue);
        
            if (size(others_queue) > conf_file.QUEUE_POS) {
                char text[MAX + 30];
                sprintf(text, "QUEUE FULL ORDER %s DELETED", received);
                escrever_arquivo(text);
            } else {
                if (!is_empty(&others_queue)) {
                    push(&others_queue, received, 0);
                } else {
                    others_queue = create_new_Queue(received, 0);
                }
            }

            pthread_mutex_unlock(&mutex_others_queue);
        }
    }
    pthread_exit(NULL);
    return NULL;
}

// funcao de controlo para ver dados de cada phone
void ver(){
	for(int i = 0;i<conf_file.MAX_MOBILE_USERS;i++){
        fprintf(stderr,"Mobile %d: id-%d pla_i-%d plaf_a-%d\n",i,smemory->phone[i].id_user,smemory->phone[i].plafond_initial,smemory->phone[i].plafond_atual);
    }
}

// thread sender

// Verificar problema do auth não dar ocupado
void* sender_func() {
    escrever_arquivo("THREAD SENDER CREATED");
    
    while (!terminate_threads) {	
        pthread_mutex_lock(&mutex_video_queue);       
        while (!is_empty(&video_queue)) {    
            for (int i = 0; i < conf_file.AUTH_SERVERS_MAX; i++) {
                if (smemory->auth_list[i] == 1) {
                    char* text = NULL;
                    text = (char*) malloc((strlen("SENDER : AUTHORIZATION_ENGINE BUSY") + sizeof(i)) * sizeof(char) + 1);
                    sprintf(text, "SENDER : AUTHORIZATION_ENGINE %d BUSY", i);
                    escrever_arquivo(text);
                    free(text);
                } else {
                    char text[MAX + 100];
                    sprintf(text, "SENDER: VIDEO AUTHORIZATION SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d - %s", i, msg_in_queue(&video_queue));
                    
                    if (write(unnamed_pipes[i][1], msg_in_queue(&video_queue), strlen(msg_in_queue(&video_queue)) + 1) == -1) {
                        escrever_arquivo("ERROR WRITING TO AUTHORIZATION_ENGINE PIPE");
                        exit(0);
                    } else {
                        escrever_arquivo(text);
                        deleteNode(&video_queue);
                    }
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex_video_queue);

        pthread_mutex_lock(&mutex_others_queue);
        while (!is_empty(&others_queue)) {
            for (int i = 0; i < conf_file.AUTH_SERVERS_MAX; i++) {
                if (smemory->auth_list[i] == 1) {
                    char* text = NULL;
            		text = (char*) malloc((strlen("SENDER : AUTHORIZATION_ENGINE BUSY") + sizeof(i)) * sizeof(char) + 1);
                    sprintf(text, "SENDER : AUTHORIZATION_ENGINE %d BUSY", i);
                    escrever_arquivo(text);
                    free(text);
                } else {
                    char text[MAX + 100];
                    char* msg = msg_in_queue(&others_queue);
                    if (msg != NULL) {
                        if (strstr(msg, "MUSIC") != NULL) {
                            sprintf(text, "SENDER: MUSIC AUTHORIZATION SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d - %s", i, msg);
                        } else if (strstr(msg, "SOCIAL") != NULL) {
                            sprintf(text, "SENDER: SOCIAL AUTHORIZATION SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d - %s", i, msg);
                        } else if (strstr(msg, "data_stats") != NULL){
                            sprintf(text, "SENDER: data_stats AUTHORIZATION SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d - %s", i, msg);
                        } else if (strstr(msg, "reset") != NULL){
                            sprintf(text, "SENDER: reset AUTHORIZATION SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d - %s", i, msg);
                        } else {
                            sprintf(text, "SENDER: LOGIN AUTHORIZATION SENT FOR PROCESSING ON AUTHORIZATION_ENGINE %d - %s", i, msg);
                        }

                        if (write(unnamed_pipes[i][1], msg, strlen(msg) + 1) == -1) {
                            escrever_arquivo("ERROR WRITING TO AUTHORIZATION_ENGINE PIPE");
                            exit(0);
                        } else {
                            escrever_arquivo(text);
                            deleteNode(&others_queue);
                        }
                        break;
                    }
                }
            }
		}
		pthread_mutex_unlock(&mutex_others_queue);
    }
    pthread_exit(NULL);
    return NULL;
}

// processo authorization_engine

// MEMORIA PARTILHADA NÃO FICA COM OS DADOS IGUAIS EM TODOS OS LADOS
void* authorization_engine(int id){
    char* textx = (char*) malloc((strlen("AUTHORIZATION_ENGINE  READY") + sizeof(id)) * sizeof(char) + 1);
    sprintf(textx, "AUTHORIZATION_ENGINE %d READY", id);
    escrever_arquivo(textx);
    free(textx);

    char buffer[MAX];
    
    key_t key = ftok("msgfile", 'A');
    int msgq_id = msgget(key, 0666 | IPC_CREAT);
    msgq message;
    
    char msg_queue[MAX];
    char msg_stat[MAX];
    int max_msg_size = MAX; 
    
    while(1){
        msg_queue[0] = '\0';
        msg_stat[0] = '\0';

    	// read [0] || write [1]
        if (read(unnamed_pipes[id][0], buffer, MAX) < 0) {
            printf("ERROR READING FROM PIPE");
            termina_programa();
        }
        
        sem_wait(sem_shm);
        smemory->auth_list[id] = 1;
		sem_post(sem_shm);
		
		sem_wait(sem_authorization);
		
        char* textxx = (char*) malloc((strlen("AUTHORIZATION_ENGINE: BUSY") + sizeof(id)) * sizeof(char) + 1);
        sprintf(textxx, "AUTHORIZATION_ENGINE %d: BUSY - %s", id,buffer);
        escrever_arquivo(textxx);
        free(textxx);
        
        char boas[50];
        strcpy(boas,buffer);

    	char *token = strtok(buffer, "#");
        int id_user = atoi(token);
        token = strtok(NULL, "#");

        char msg[MAX];
        msg[0] = '\0';
		
		
        if (atoi(token)) {
        	//fprintf(stderr,"Entrei LOGIN\n");
        	// Processar pedido de login(345#800)

            int plafond = atoi(token);

            for (int i = 0; i < conf_file.MAX_MOBILE_USERS; i++) {
                if (smemory->phone[i].id_user == -1) {
                    smemory->phone[i].id_user = id_user;
                    smemory->phone[i].plafond_initial = plafond;
                    break;
                }
            }	
            
            sprintf(msg,"AUTHORIZATION_ENGINE %d :LOGIN AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED",id,id_user) ;
            
        } else if (strcmp(token, "VIDEO") == 0) {
    		//fprintf(stderr,"Entrei VIDEO\n");
        	// Processar pedido de vídeo (345#VIDEO#40)

            token = strtok(NULL, "#");
            int data_to_reserve = atoi(token);

            int check = 0;

            for (int i = 0; i < conf_file.MAX_MOBILE_USERS; i++) {
                if (smemory->phone[i].id_user == id_user) {
                    if(smemory->phone[i].plafond_atual + data_to_reserve < smemory->phone[i].plafond_initial){
                        smemory->phone[i].plafond_atual += data_to_reserve;
                        smemory->est->nr_requests_video += 1;
                        smemory->est->video_total += data_to_reserve;
                        check = 1;
                    }
                    break;
                }
            }
            if(check == 1){
            	//ver();
            	//ver_stats();
                sprintf(msg,"AUTHORIZATION_ENGINE %d :VIDEO AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED",id,id_user) ;	
            } else {
            	//ver();
            	//ver_stats();
                sprintf(msg,"AUTHORIZATION_ENGINE %d :VIDEO AUTHORIZATION REQUEST (ID = %d) PROCESSING INCOMPLETED NO MORE PLAFOND",id,id_user) ;	
            }
            
        } else if (strcmp(token, "MUSIC") == 0 ) {
    		//fprintf(stderr,"Entrei MUSIC\n");
        	// Processar pedido de musica (345#MUSIC#40)

            token = strtok(NULL, "#");
            int data_to_reserve = atoi(token);

            int check = 0;
            
            for (int i = 0; i < conf_file.MAX_MOBILE_USERS; i++) {
                if (smemory->phone[i].id_user == id_user) {
                    if(smemory->phone[i].plafond_atual + data_to_reserve < smemory->phone[i].plafond_initial){
                        smemory->phone[i].plafond_atual += data_to_reserve;
                        smemory->est->nr_requests_musica += 1;
                        smemory->est->musica_total += data_to_reserve;
                        check = 1;
                    }
                    break;
                }
            }
            if(check == 1){
            	//ver();
            	//ver_stats();
                sprintf(msg,"AUTHORIZATION_ENGINE %d :MUSIC AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED",id,id_user) ;	
            } else {
            	//ver();
            	//ver_stats();
                sprintf(msg,"AUTHORIZATION_ENGINE %d :MUSIC AUTHORIZATION REQUEST (ID = %d) PROCESSING INCOMPLETED NO MORE PLAFOND",id,id_user) ;	
            }

        } else if (strcmp(token, "SOCIAL") == 0){
        	//fprintf(stderr,"Entrei SOCIAL\n");
        	// Processar pedido de social (345#SOCIAL#40)

            token = strtok(NULL, "#");
            int data_to_reserve = atoi(token);

            int check = 0;
            
            for (int i = 0; i < conf_file.MAX_MOBILE_USERS; i++) {
                if (smemory->phone[i].id_user == id_user) {
                    if(smemory->phone[i].plafond_atual + data_to_reserve < smemory->phone[i].plafond_initial){	
                        smemory->phone[i].plafond_atual += data_to_reserve;
                        smemory->est->nr_requests_social += 1;
                        smemory->est->social_total += data_to_reserve;
                        check=1;
                    }
                    break;
                }
            }
            
            if(check == 1){
            	//ver();
            	//ver_stats();
                sprintf(msg,"AUTHORIZATION_ENGINE %d :SOCIAL AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED",id,id_user) ;	
            } else {
            	//ver();
            	//ver_stats();
                sprintf(msg,"AUTHORIZATION_ENGINE %d :SOCIAL AUTHORIZATION REQUEST (ID = %d) PROCESSING INCOMPLETED NO MORE PLAFOND",id,id_user) ;	
            }
            
        } else if (strcmp(token, "reset") == 0){
        	//fprintf(stderr,"Entrei reset\n");
        	// Processar pedido de reset (1#reset)

            smemory->est->nr_requests_video = 0;
            smemory->est->video_total = 0;
            smemory->est->nr_requests_musica = 0;
            smemory->est->musica_total = 0;
            smemory->est->nr_requests_social = 0;
            smemory->est->social_total = 0;

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.id_user = id_user;

            sprintf(msg_stat, "ATUAL TASK\nDATA RESETED");

            strcat(msg_queue, msg_stat);
            strncpy(message.message, msg_queue, max_msg_size);
            
			if (msgsnd(msgq_id, &message, max_msg_size, 0) == 1) {
				escrever_arquivo(msg_stat);
            } 

            sprintf(msg,"AUTHORIZATION_ENGINE %d :reset AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED",id,id_user);

        } else if (strcmp(token, "data_stats") == 0){
        	//fprintf(stderr,"Entrei data_stats\n");
        	// Processar pedido de data_stats (1#data_stats)

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.id_user = id_user;

            sprintf(msg_stat, "ATUAL TASK\nSERVICE  TOTAL DATA  AUTH REQUESTS\n""VIDEO    %d          %d\n""MUSIC    %d          %d\n""SOCIAL   %d          %d\n",smemory->est->video_total, smemory->est->nr_requests_video,smemory->est->musica_total, smemory->est->nr_requests_musica,smemory->est->social_total, smemory->est->nr_requests_social);

            strcat(msg_queue, msg_stat);
            strncpy(message.message, msg_queue, max_msg_size);

            if (msgsnd(msgq_id, &message, max_msg_size, 0) == 1) {
				escrever_arquivo(msg_stat);
            } 
            
            sprintf(msg,"AUTHORIZATION_ENGINE %d :data_stats AUTHORIZATION REQUEST (ID = %d) PROCESSING COMPLETED",id,id_user);
        }

        usleep(conf_file.AUTH_PROC_TIME * 10000);
        escrever_arquivo(msg);
        
		
		sem_post(sem_authorization);
		
        sem_wait(sem_shm);
        smemory->auth_list[id] = 0;
		sem_post(sem_shm);

    	textxx = (char*) malloc((strlen("AUTHORIZATION_ENGINE: READY AGAIN") + sizeof(id)) * sizeof(char) + 1);
        sprintf(textxx, "AUTHORIZATION_ENGINE %d: READY AGAIN", id);
        escrever_arquivo(textxx);
        free(textxx);
    }
    return NULL;
}

// processo monitor_engine
void monitor_engine(){
	key_t key = ftok("msgfile", 'A');
    int msgq_id = msgget(key, 0666 | IPC_CREAT);
    msgq message;

    if (msgq_id == -1) {
        perror("Erro ao criar ou recuperar a fila de mensagens");
        exit(EXIT_FAILURE);
    }

    char msg[MAX];
    char msg_alert[MAX];
    int max_msg_size = MAX; 
    int tempo = 1;
	int i = 0;
    while(!terminate_threads || i < conf_file.MAX_MOBILE_USERS){
        msg[0] = '\0';
        msg_alert[0] = '\0';

        if (msgq_id == -1) {
            perror("Erro ao criar ou recuperar a fila de mensagens");
            exit(EXIT_FAILURE);
        }
        

		if(smemory->phone[i].id_user != -1){
			if(((smemory->phone[i].plafond_initial * 0.80) >= smemory->phone[i].plafond_atual) && ((smemory->phone[i].plafond_initial * 0.90) < smemory->phone[i].plafond_atual)){
            	message.id_user = smemory->phone[i].id_user;
            	sprintf(msg_alert, "REACHED 80%% OF THE INITIAL PLAFOND");
	
            	strcat(msg, msg_alert);
            	strncpy(message.message, msg, max_msg_size);
            	
            	char arquivo[MAX];
	
            	if (msgsnd(msgq_id, &message, max_msg_size, 0) != -1) {
                	sprintf(arquivo, "ALERT 80%% (USER %d) TRIGGERED",smemory->phone[i].id_user);
                	escrever_arquivo(arquivo);
            	}
    		} else if(((smemory->phone[i].plafond_initial * 0.90) >= smemory->phone[i].plafond_atual) && ((smemory->phone[i].plafond_initial) < smemory->phone[i].plafond_atual)){
            	message.id_user = smemory->phone[i].id_user;
            	sprintf(msg_alert, "REACHED 90%% OF THE INITIAL PLAFOND");
	
            	strcat(msg, msg_alert);
            	strncpy(message.message, msg, max_msg_size);
            	
            	char arquivo[MAX];
	
            	if (msgsnd(msgq_id, &message, max_msg_size, 0) != -1) {
                	sprintf(arquivo, "ALERT 90%% (USER %d) TRIGGERED",smemory->phone[i].id_user);
                	escrever_arquivo(arquivo);
            	}
        	} else if(((smemory->phone[i].plafond_initial) = smemory->phone[i].plafond_atual)){
            	message.id_user = smemory->phone[i].id_user;
            	sprintf(msg_alert, "REACHED 100%% OF THE INITIAL PLAFOND");
	
            	strcat(msg, msg_alert);
            	strncpy(message.message, msg, max_msg_size);
            	
            	char arquivo[MAX];
	
            	if (msgsnd(msgq_id, &message, max_msg_size, 0) != -1) {
                	sprintf(arquivo, "ALERT 100%% (USER %d) TRIGGERED",smemory->phone[i].id_user);
                	escrever_arquivo(arquivo);	
            	}
        	} 
		}
    	
    	if (tempo % 30 == 0){ 
            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.id_user = 1;

            sprintf(msg_alert, "PERIODIC MESSAGE FROM MONITOR ENGINE\n\nSERVICE  TOTAL DATA  AUTH REQUESTS\n""VIDEO    %d          %d\n""MUSIC    %d          %d\n""SOCIAL   %d          %d\n",smemory->est->video_total, smemory->est->nr_requests_video,smemory->est->musica_total, smemory->est->nr_requests_musica,smemory->est->social_total, smemory->est->nr_requests_social);
	
            strcat(msg, msg_alert);
            strncpy(message.message, msg, max_msg_size);
	
            if (msgsnd(msgq_id, &message, max_msg_size, 0) != -1) {
				escrever_arquivo("MONITOR ENGINE: PERIODIC STATS SEND TO BACK_OFFICE");
            } 

        }
        
        i++;
        tempo++;

        if (i == conf_file.MAX_MOBILE_USERS){
            i = 0;
        }
    	sleep(1); // Aguarda 1 segundo
    }
    
}

// processo authorization_request_manager
void* authorization_request_manager() {
    // abrir o pipe USER_PIPE
    if ((fd_user_pipe = open(USER_PIPE, O_RDWR)) < 0) {
        escrever_arquivo("ERROR OPENING USER PIPE");
        exit(1);
    }

    // abrir o pipe BACK_PIPE
    if ((fd_back_pipe = open(BACK_PIPE, O_RDWR)) == -1) {
        escrever_arquivo("ERROR OPENING BACK PIPE");
        exit(1);
    }
    
    smemory->auth_list = (int*)malloc(conf_file.AUTH_SERVERS_MAX * sizeof(int));
    
	if (smemory->auth_list == NULL) {
        escrever_arquivo("ERROR ALLOCATING MEMORY FOR AUTH LIST");
        exit(1);
	}
    
    // alocar memoria para os unnamed pipes
    unnamed_pipes = malloc(conf_file.AUTH_SERVERS_MAX * sizeof(int*));
	
    // inicializar authorization_request
    for (int i = 0; i < conf_file.AUTH_SERVERS_MAX; i++) {
        // inicializar a lista de auth
        // 0 -> livre; 1 -> ocupado 
        smemory->auth_list[i] = 0;
        
        if (pipe(unnamed_pipes[i]) == -1) {
            escrever_arquivo("ERROR CREATING PIPE");
            exit(1);
        }
        
        //print do workers list
        int authoriztion_forks;
        if ((authoriztion_forks = fork()) == 0) {
            authorization_engine(i);
            exit(0);
        }
        
        else if (authoriztion_forks == -1) {
            escrever_arquivo("ERROR CREATING AUTHORIZATION_ENGINE");
            exit(1);
        }
    }
    
    // Criar as Threads e esperar que completem
    pthread_create(&receiver, NULL, receiver_func, NULL);
    pthread_create (&sender,NULL, sender_func, NULL);
    
    pthread_join(receiver, NULL);
    pthread_join(sender,NULL);

    for (int i = 0; i < conf_file.AUTH_SERVERS_MAX; i++) {
        wait(NULL);
    }
    
    free(unnamed_pipes);

    return NULL;
    // Falta criar unnamed pipes extras e depois apagar quando necessário
}

// funcao que cria as threads, os workers e o alerts watcher
void system_manager(char* config_file,int id) { 
    // Incializa o programa
    inicia_programa();
	
    // Informar no server de logs que já a correr o SIMULADOR
    escrever_arquivo("5G_AUTH_PLATFORM SIMULATOR STARTING");
	escrever_arquivo("PROCESS SYSTEM_MANAGER CREATED");

    // ler config file
    read_config(config_file);
    
    // Inicialização de smemory->phone como uma lista
    smemory->phone = (Phone_input*)malloc(conf_file.MAX_MOBILE_USERS * sizeof(Phone_input));
    if (smemory->phone == NULL) {
        escrever_arquivo("ERROR ALLOCATING MEMORY FOR PHONE_INPUT LIST");
        exit(1);
    }
    
    // Inicializar todos os id_user com -1
    for (int i = 0; i < conf_file.MAX_MOBILE_USERS; i++) {
        smemory->phone[i].id_user = -1;
        smemory->phone[i].plafond_initial = 0;
        smemory->phone[i].plafond_atual = 0;
    }
    
    smemory->est = (stats*)malloc(sizeof(stats));
    
    if (smemory->est == NULL) {
        escrever_arquivo("ERROR ALLOCATING MEMORY FOR STATISTICS LIST");
        exit(1);
    }
    
    smemory->est->nr_requests_video = 0;
    smemory->est->video_total = 0;
    smemory->est->nr_requests_musica = 0;
    smemory->est->musica_total = 0;
    smemory->est->nr_requests_social = 0;
    smemory->est->social_total = 0;
    
    // inicializar authorization_request
    int a;
    if((a = fork()) == 0){
        escrever_arquivo("PROCESS AUTHORIZATION_REQUEST_MANAGER CREATED");
        authorization_request_manager();
        exit(0);
    } else if(a == -1){
        escrever_arquivo("ERROR CREATING AUTHORIZATION_REQUEST_MANAGER");
        exit(1);
    }
    
    // inicializar monitor_engine
    int m;
    if ((m = fork()) == 0) {
        escrever_arquivo("PROCESS MONITOR_ENGINE CREATED");
        monitor_engine();
        exit(0);
    }
    else if (m == -1) {
        escrever_arquivo("ERROR CREATING  MONITOR_ENGINE");
        exit(1);
    }

    signal(SIGINT,sigint);

    wait(NULL);
    wait(NULL);
}
