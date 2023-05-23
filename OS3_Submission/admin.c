#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "admin.h"
#include "types.h"
#include "cal-new.h"

#define BACKLOG 10
#define PORT_NUM 190313
#define MAX_CALC_BUFFERS 1

int p[2];
int p1[2];
sem_t *semaphore_parent_child;

void print_sorted_response(sort_request_t *client_sort_response) {
  printf("\n");
  printf("======================= CID=%d, file=%s, N=%d \n",
         client_sort_response->client_id,
         client_sort_response->filename,
         client_sort_response->number_of_integers);

  for (int i = 0; i < client_sort_response->number_of_integers; i++) {
    printf("%d ", client_sort_response->integers[i]);
    if (i % 10 == 0 && i != 0) {
      printf("\n");
    }
  }
  printf("\n\n");
}

pthread_mutex_t mutex;

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void SetupSignalHandler();

int CreateServerSocket(int port);

int server_socket_fd;

int write_to_pipe(int fd, char *buf, int len) {
  pthread_mutex_lock(&mutex);
  int ret = write(fd, buf, len);
  pthread_mutex_unlock(&mutex);
  return ret;
}

int read_from_pipe(int fd, char *buf, int len) {
  pthread_mutex_lock(&mutex);
  int ret = read(fd, buf, len);
  pthread_mutex_unlock(&mutex);
  return ret;
}

void create_shared_semaphore(void) {
  /* place semaphore in shared memory */
  semaphore_parent_child = mmap(NULL, sizeof(*semaphore_parent_child),
                                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,
                                -1, 0);
  if (semaphore_parent_child == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  /* create/initialize semaphore */
  if (sem_init(semaphore_parent_child, 1, MAX_CALC_BUFFERS) < 0) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
}

void destroy_shared_semaphore(void) {
  /* destroy semaphore */
  if (sem_destroy(semaphore_parent_child) < 0) {
    perror("sem_destroy");
    exit(EXIT_FAILURE);
  }

  /* remove semaphore from shared memory */
  if (munmap(semaphore_parent_child, sizeof(*semaphore_parent_child)) < 0) {
    perror("munmap");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  pid_t pid;

  pthread_mutex_init(&mutex, NULL);

  create_shared_semaphore();

  pipe(p); //for child process
  pipe(p1); //for parent process

  pid = fork();

  if (pid == 0) {
    close(p[0]);
    close(p1[1]);

    pipe_data_t pipe_data;
    int signal = 1; //Variable that will signal parent process to send next instruction

    while (true) {

      /* Read incoming SORT Request */
      read(p1[0], &pipe_data, sizeof(pipe_data));

      if (pipe_data.command == CMD_END)
      {
        write(p[1], &signal, sizeof(int)); //signal the parent
        break; //breaks the loop and exits
      }
      else if (pipe_data.command == CMD_SORT)
      {
        sort_request_t *received_sort_request;
        int buffer_size = sizeof(sort_request_t) + (sizeof(unsigned int) * MAX_SORT_INTEGER);
        char *buffer = malloc(buffer_size);

        /* Read the buffer with the actual data */
        read(p1[0], buffer, buffer_size); //reading the sort request

        received_sort_request = (sort_request_t *) buffer;
        received_sort_request->integers = (unsigned int *) (buffer + sizeof(sort_request_t));

        /* Sort the incoming numbers*/
        process_parallel_merge(received_sort_request);

        print_sorted_response(received_sort_request);
        sem_post(semaphore_parent_child);
      }
    }

    close(p[1]); //closing the pipes
    close(p1[0]);
    return 0; //child process ends here
  } else {
    int port, new_socket_fd;
    pthread_attr_t pthread_client_attr;
    pthread_t pthread;
    socklen_t client_address_len;
    struct sockaddr_in client_address;

    /* Get port from command line arguments or stdin.
     * For this server, this is fixed to 1113*/
    port = PORT_NUM;

    /*Create the server socket */
    server_socket_fd = CreateServerSocket(port);

    /*Setup the signal handler*/
    SetupSignalHandler();

    /* Initialise pthread attribute to create detached threads. */
    if (pthread_attr_init(&pthread_client_attr) != 0) {
      perror("pthread_attr_init");
      exit(1);
    }

    if (pthread_attr_setdetachstate(&pthread_client_attr, PTHREAD_CREATE_DETACHED) != 0) {
      perror("pthread_attr_setdetachstate");
      exit(1);
    }

    while (1) {
      /* Accept connection to client. */
      client_address_len = sizeof(client_address);
      new_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client_address, &client_address_len);
      if (new_socket_fd == -1) {
        perror("accept");
        continue;
      }

      printf("Client connected\n");
      unsigned int *thread_arg = (unsigned int *) malloc(sizeof(unsigned int));
      *thread_arg = new_socket_fd;
      /* Create thread to serve connection to client. */
      if (pthread_create(&pthread, &pthread_client_attr, pthread_routine, (void *) thread_arg) != 0) {
        perror("pthread_create");
        continue;
      }
    }
  }
  return 0;
}

int CreateServerSocket(int port) {
  struct sockaddr_in address;
  int socket_fd;

  /* Initialise IPv4 address. */
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = INADDR_ANY;

  /* Create TCP socket. */
  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  /* Bind address to socket. */
  if (bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
    perror("bind");
    exit(1);
  }

  /* Listen on socket. */
  if (listen(socket_fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  // Configure server socket
  int enable = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  return socket_fd;
}

void SetupSignalHandler() {/* Assign signal handlers to signals. */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("signal");
    destroy_shared_semaphore();
    exit(1);
  }

  if (signal(SIGTERM, signal_handler) == SIG_ERR) {
    perror("signal");
    destroy_shared_semaphore();
    exit(1);
  }
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    perror("signal");
    destroy_shared_semaphore();
    exit(1);
  }
}

void *pthread_routine(void *arg) {

  int client_socket = *(int *) arg;
  free(arg);

  int read_size;
  int buffer_size = sizeof(sort_request_t) + (sizeof(unsigned int) * MAX_SORT_INTEGER);
  char *buffer = malloc(buffer_size);

  read_size = read(client_socket, buffer, buffer_size);
  if (read_size == -1) {
    perror("read");
    free(buffer);
    exit(1);
  } else if (read_size == 0) {
    printf("Client disconnected\n");
    free(buffer);
    close(client_socket);
    return NULL;
  }

  sort_request_t *sort_request = (sort_request_t *) buffer;
  if (sort_request->number_of_integers > MAX_SORT_INTEGER) {
    printf("Too many integers to sort.\n");
    close(client_socket);
    free(buffer);
    return NULL;
  }

  sort_request->integers = (unsigned int *) (buffer + sizeof(sort_request_t));

  if (sem_wait(semaphore_parent_child) < 0) {
    perror("sem_wait");
  }

  pipe_data_t pipe_data = {
      .command = CMD_SORT,
      .data_size = sort_request->number_of_integers,
  };

  write_to_pipe(p1[1], (char *) &pipe_data, sizeof(pipe_data));
  write_to_pipe(p1[1], buffer, read_size);
  free(buffer);
  return NULL;
}

void signal_handler(int signal_number) {
  close(server_socket_fd);
  exit(0);
}