/*
    This file links libraries, implements macros, functions and structures used by the server and the client.

    Autor: Ignat Gabriel-Andrei  <gabriel.ignat.v09@gmail.com>
*/
#include <sys/types.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <unistd.h>

#include <stdio.h>

#include <errno.h>

#include <stdlib.h>

#include <netdb.h>

#include <string.h>

#include <ctype.h> 

#include <stdbool.h>

#include <signal.h>

#include <pthread.h>

#define LARGE_LEN_BUFFER      4096

#define MEDIUM_LEN_BUFFER     512

#define SMALL_LEN_BUFFER      128

#define MAX_Threads           1000000

#define MAX_NR_SERVICES       64

#define MAX_LEN_SERVICE_NAME  256

#define MAX_LEN_IP            32

#define MAX_LEN_PORT          32

#define MAX_LEN_QUEUE         8

#define MAX_LIST_NUMBERS      100

#define PRINT_MSG(...) \
    {printf(__VA_ARGS__), fflush(stdout);}

#define ERROR_EXIT(condition , ...) \
    if((condition)) \
    {\
      PRINT_MSG(__VA_ARGS__);\
      perror("");\
      exit(EXIT_FAILURE);\
    }

#define ERROR_RET(condition , ret, ...) \
    if((condition)) \
    {\
      PRINT_MSG(__VA_ARGS__);\
      perror("");\
      return ret;\
    }

#define ERROR_RET_VOID(condition, ...) \
    if((condition)) \
    {\
      PRINT_MSG(__VA_ARGS__);\
      perror("");\
      return;\
    }

#define ERROR_RET_VOID_NULL(condition, ...) \
    if((condition)) \
    {\
      PRINT_MSG(__VA_ARGS__);\
      perror("");\
      return(NULL);\
    }

#define ERROR_CONTINUE(condition, ...)\
    if((condition)) \
    {\
      PRINT_MSG(__VA_ARGS__);\
      perror("");\
      continue;\
    }

// scrieri partiale
int write_again_and_again(int dest_fd, const void *buffer, int expected_len_write) 
{
  int len_write, total_len_write = 0;
  const char *data_buffer = buffer;
  while (expected_len_write > 0) {
    do {
            len_write = write(dest_fd, data_buffer, expected_len_write);
            if (len_write != -1) 
              total_len_write += len_write;
      } while ((len_write < 0) && ( errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK));
    if (len_write < 0)
      return len_write;
    expected_len_write -= len_write;
    data_buffer += len_write;
  }
  return total_len_write;
}

bool write_with_length_prefixed(int dest_fd, const char *buffer, int expected_len_write) 
{
  ERROR_RET( write_again_and_again(dest_fd, &expected_len_write, sizeof(int)) != sizeof(int), false,
            "Nu se poate scrie lungimea sirului de caractere ce se doreste a fi scris = %d.\n", expected_len_write);	//mai intai scrie lungimea -> daca reuseste
  ERROR_RET( write_again_and_again(dest_fd, buffer, expected_len_write) != expected_len_write, false,
            "Nu se poate scrie mesajul.\n");                                                 //apoi scrie sirul propriu-zis								

  return true;
}

// citiri partiale
int read_again_and_again(int src_fd, void *buffer, int expected_len_read) 
{
  char *data_buffer = buffer;
  int len_read, total_len_read = 0;
  while (expected_len_read > 0 && (len_read = read(src_fd, data_buffer, expected_len_read)) != 0) {
      if (len_read == -1)
        return -1;
	    total_len_read += len_read;
	    data_buffer += len_read;
	    expected_len_read -= len_read;
  }
  return total_len_read;
}

bool read_with_length_prefixed(int src_fd, char *buffer) 
{
  int expected_len_read = 0;
  ERROR_RET( read_again_and_again(src_fd, &expected_len_read, sizeof(int)) != sizeof(int), false,
          "Nu se poate citi lungimea sirului ce se doreste a fi citit. Expected_len_read: %d.\n",expected_len_read);  //mai intai citeste lungimea -> daca reuseste

  ERROR_RET( read_again_and_again(src_fd, buffer, expected_len_read) != expected_len_read, false,
          "Nu se poate citi mesajul.\n");                              //apoi citeste sirul propriu-zis											
  buffer[expected_len_read] = '\0';
  
  return true;
}



bool check_is_address_valid(const char *ip_address) 
{
  if (!(strlen(ip_address) >= 7 && strlen(ip_address) <= 15))
	  return false;
  
  if (ip_address == NULL) 
    return false;

  char sectiune[4];
  
  int index_sectiune = 0,
      points = 0,
      nr;
  bool all_digits = true;

  for(int index = 0 ; index < strlen(ip_address) ; index += 1) 
  {
      if (all_digits == true) 
      {
        if (!(ip_address[index] >= '0' && ip_address[index] <= '9')) 
            return false;
        
        sectiune[index_sectiune] = ip_address[index];
        index_sectiune += 1;
        
        if (index_sectiune == 3 || (index + 1 < strlen(ip_address) && ip_address[index + 1] == '.')) 
        {
          sectiune[index_sectiune] = '\0';
          nr = (int)atoi(sectiune);
          
          if (!(nr >= 0 && nr <= 255)) 
            return false;
         
          index_sectiune = 0;
          all_digits = false;
        }
      } 
      else  if (ip_address[index] != '.') 
              return false; 
            else 
            {
              all_digits = true;
              points += 1;
            }
  }

  return points == 3;
}

bool check_is_port_valid(const char *port)
{
  if (strlen(port) > 6)
	  return false;

  if (port == NULL)
	  return false;
  
  int index = 0,
      nr;
  while (index < strlen(port)) 
  {
	  if (!(port[index] >= '0' && port[index] <= '9')) 
      return false;
	  index += 1;
	} 
  nr = atoi(port);

  return nr < 65536;
}