#ifndef TYPES_H_
#define TYPES_H_

#include <stdint.h>

#define SERVER_NAME_LEN_MAX 255
#define FILE_NAME_LEN_MAX 255
#define MAX_USER_INPUT_LEN 255
#define MAX_SORT_INTEGER 65535

typedef enum command
{
  CMD_SORT,
  CMD_END,
}command_t;

typedef struct sort_request
{
  uint32_t client_id;
  char filename[FILE_NAME_LEN_MAX];
  uint32_t number_of_integers;
  uint32_t *integers;
}sort_request_t;

typedef struct pipe_data {
  command_t command;
  uint32_t data_size;
}pipe_data_t;

#endif //TYPES_H_
