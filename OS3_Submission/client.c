#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include "types.h"

uint32_t client_id = 0;

void get_line_from_stdin(char *line, uint32_t max_length)
{
  int i = 0;
  char c;
  while (i < max_length - 1 && (c = getchar()) != '\n')
  {
    line[i] = c;
    i++;
  }
  line[i] = '\0';
}

void print_user_menu ( void )
{
  printf("Welcome to the Sorter Client\n");
  printf("Please select one of the following options:\n");
  printf("1. Sort\n");
  printf("2. End\n");
  printf("3. Help\n");
}

void print_sort_menu ( char* filename, uint32_t length )
{
  printf("Please enter the filename \n");
  get_line_from_stdin(filename, length);

  if(access(filename, F_OK) != 0)
  {
    printf("File does not exist\n");
    return;
  }
}

void print_sorted_response(sort_request_t *client_sort_response)
{
  printf("\n");
  printf("======================= CID=%d, file=%s, N=%d \n", client_sort_response->client_id, client_sort_response->filename, client_sort_response->number_of_integers);

  for (int i = 0; i < client_sort_response->number_of_integers; i++)
  {
    printf("%d ", client_sort_response->integers[i]);
    if (i % 10 == 0 && i != 0)
    {
      printf("\n");
    }
  }
  printf("\n\n");
}

void process_sort_request(int socket_id, sort_request_t *request)
{
  FILE *file = fopen(request->filename, "r");
  if(file == NULL) {
    printf("Error opening file\n");
    return;
  }

  fscanf(file, "%d", &request->number_of_integers);
  request->integers = malloc(sizeof(uint32_t) * request->number_of_integers);

  for(int i = 0; i < request->number_of_integers; i++) {
    fscanf(file, "%d", &request->integers[i]);
  }

  uint32_t buffer_size = sizeof(sort_request_t) + request->number_of_integers * sizeof(uint32_t);
  char *buffer = malloc(buffer_size);
  memcpy(buffer, request, sizeof(sort_request_t));
  memcpy(buffer + sizeof(sort_request_t), request->integers, request->number_of_integers * sizeof(uint32_t));

  if(send(socket_id, buffer, buffer_size, 0) < 0) {
    printf("Error sending data\n");
    return;
  }

  free(buffer);
  free(request->integers);
  fclose(file);

}




int main(int argc, char *argv[])
{
  char server_name[SERVER_NAME_LEN_MAX + 1] = { 0 };
  int server_port, socket_fd;
  struct hostent *server_host;
  struct sockaddr_in server_address;

  bool is_end_requested = false;

  if (argc < 4)
  {
    printf("Usage: %s <server name> <server port> <client id>\n", argv[0]);
    exit(1);
  }

  /* Get server name from command line arguments or stdin. */
  strncpy(server_name, argv[1], SERVER_NAME_LEN_MAX);
  server_port =  atoi(argv[2]);
  client_id = atoi(argv[3]);

  /* Get server host from server name. */
  server_host = gethostbyname(server_name);

  /* Initialise IPv4 server address with server host. */
  memset(&server_address, 0, sizeof server_address);
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(server_port);
  memcpy(&server_address.sin_addr.s_addr, server_host->h_addr, server_host->h_length);

  /* Create TCP socket. */
  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  /* Connect to socket with server address. */
  if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1) {
    perror("connect");
    exit(1);
  }

  printf("Connected to server.\n");

  while (!is_end_requested)
  {
    print_user_menu();
    char user_input[MAX_USER_INPUT_LEN];
    get_line_from_stdin(user_input, MAX_USER_INPUT_LEN);
    int user_input_int = atoi(user_input);
    switch (user_input_int) {
      case 1:
      {
        sort_request_t client_sort_request = { .client_id = client_id };
        print_sort_menu(client_sort_request.filename, FILE_NAME_LEN_MAX);
        process_sort_request(socket_fd, &client_sort_request);
        break;
      }
      case 2:
        is_end_requested = true;
        break;
      case 3:
        printf("1. Sort\n");
        printf("2. End\n");
        printf("3. Help\n");
        break;
      default:
        printf("Invalid input.\n");
        break;
    }
  }

  close(socket_fd);
  return 0;
}