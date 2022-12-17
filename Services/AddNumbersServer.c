/* AddNumbersServer.c - This is an example of a server that receives numbers and sends their sum
    At the beginning, the server sends a message to RegisterS with the ip addres, port number and service name.

   Autor: Ignat Gabriel-Andrei  <gabriel.ignat.v09@gmail.com>
*/

#include "../Commons/common.c"

#define PORT 2507
#define IPv4 "127.0.0.1"

extern int errno;
#define OPERATOR(nr1,nr2)\
    nr1 + nr2

int port_register;
struct sockaddr_in server_register;



typedef struct thData{
	int idThread;
	int cl;
}thData;

static void *treat(void *);
bool answer(void *);
bool verify_if_all_numbers(const char*, int*, int*);
bool is_number(const char*);
int get_result_operation(int*,int);


int main (int argc, char *argv[])
{
  ERROR_EXIT((argc != 3) ,
            "[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
  ERROR_EXIT(check_is_address_valid(argv[1]) == false,
            "[client] Adresa IPv4 invalida.\n");
  ERROR_EXIT(check_is_port_valid(argv[2]) == false,
            "[client] PORT invalid")

  // start anunta_register_server
  struct sockaddr_in server_register;
  int socket_register;

  port_register = atoi (argv[2]);
  bzero(&server_register, sizeof(server_register));

    server_register.sin_family = AF_INET;   
    server_register.sin_addr.s_addr = inet_addr(argv[1]);    
    server_register.sin_port = htons(port_register);

  ERROR_EXIT((socket_register = socket (AF_INET, SOCK_STREAM, 0)) == -1,
            "[server]Eroare la socket().\n");

  ERROR_EXIT(connect (socket_register, (struct sockaddr *) &server_register,sizeof (struct sockaddr)) == -1,
            "[server]Eroare la connect().\n");

  char buf[MAX_LEN_SERVICE_NAME + MAX_LEN_IP + MAX_LEN_PORT];
  ERROR_EXIT(sprintf(buf, "%s %d %s", IPv4, PORT, argv[0] + 2) < 0, 
            "[server]Eroare la sprintf().\n");

  int len = strlen(buf);
  
  ERROR_EXIT(write_with_length_prefixed(socket_register, buf, len) == false,
            "[server]Eroare la write() spre server.\n");
  
  // ERROR_EXIT(close(socket_register) == -1,
  //           "[server]Eroare la close().\n")
  // end anunta_register_server

  // ----------------- start treat_clients
  struct sockaddr_in server, from;	
  int socket_server, i = 0;
  pthread_t th[MAX_Threads];
  

  ERROR_EXIT((socket_server = socket (AF_INET, SOCK_STREAM, 0)) == -1,
            "[server]Eroare la socket() pentru client.\n");
  int on=1;     
  ERROR_EXIT(setsockopt(socket_server,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) != 0,
            "[server]Eroare la setsockopt().\n");
  
  bzero (&server, sizeof (server));   
  bzero (&from, sizeof (from));
  
    server.sin_family = AF_INET;	
    server.sin_addr.s_addr = inet_addr(IPv4);  
    server.sin_port = htons (PORT);
  
  ERROR_EXIT(bind (socket_server, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1,
            "[server]Eroare la bind().\n");
  ERROR_EXIT(listen (socket_server, MAX_LEN_QUEUE) == -1,
            "[server]Eroare la listen().\n");

  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
  {
      int client;
      thData * td; //parametru functia executata de thread     
      socklen_t length = sizeof (from);

      PRINT_MSG ("[server]Asteptam la portul %d...\n",PORT);
      
      ERROR_CONTINUE( (client = accept (socket_server, (struct sockaddr *) &from, &length)) < 0 , 
                  "[server]Eroare la accept().\n");

      ERROR_CONTINUE( (td = (struct thData*)malloc(sizeof(struct thData))) == NULL,
                    "[register]Eroare la malloc.\n");	
      td = (struct thData*)malloc(sizeof(struct thData));	
      td -> idThread = i++;
      td -> cl = client;
      
      //thread CREATION
      ERROR_CONTINUE(pthread_create(&th[i], NULL, &treat, td) != 0,
                    "[register]Eroare la phtread_create().\n");	    	
	}//while
  ERROR_EXIT(close(socket_server) == -1,
            "[server]Eroare la close().\n");      
};				
static void *treat(void * arg)
{		
  struct thData tdL; 
  tdL= *((struct thData*)arg);	
  PRINT_MSG ("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
        
  ERROR_RET_VOID_NULL( (pthread_detach(pthread_self())) != 0,
                      "[thread] - %d - Eroare la pthread_detach().\n", tdL.idThread);		
  ERROR_RET_VOID_NULL( answer((struct thData*)arg) == false,
                      "[thread] - %d - Eroare la servirea clientului .\n", tdL.idThread);
  /* am terminat cu acest client, inchidem conexiunea */
  ERROR_RET_VOID_NULL( close (tdL.cl) == -1, 
                    "[thread] - %d - Eroare la servirea clientului .\n", tdL.idThread);
  return(NULL);	
};


bool answer(void *arg)
{
  char msg_received[MAX_LEN_IP + MAX_LEN_PORT + MAX_LEN_SERVICE_NAME],
         msg_answer[MAX_LEN_IP + MAX_LEN_PORT + MAX_LEN_SERVICE_NAME];
	struct thData tdL; 
	tdL= *((struct thData*)arg);

    int numbers[MAX_LIST_NUMBERS];
    int length = 0;
  
  // while(1)
  // {
    ERROR_RET( read_with_length_prefixed(tdL.cl, msg_received) == false, false,
              "[Thread %d] Eroare la read() de la client.\n",tdL.idThread);

    PRINT_MSG ("[Thread %d]Mesajul a fost receptionat...%s\n",tdL.idThread, msg_received);
    
    if (verify_if_all_numbers(msg_received,numbers,&length) == false)
    {
        ERROR_RET(strcpy(msg_answer,"The numbers where not typed correctly(use only digits)...") == NULL, false,
              "[Thread %d] Eroare la strcpy().\n",tdL.idThread);
    }
    else{
        ERROR_RET(sprintf(msg_answer,"%d",get_result_operation(numbers,length)) < 0, false,
            "[server]Eroare la sprintf().\n");
    }

    PRINT_MSG("[Thread %d]Trimitem mesajul inapoi...%s\n",tdL.idThread, msg_answer);
            
    /* returnam mesajul clientului */
    int len_msg = strlen(msg_answer);
    ERROR_RET( write_with_length_prefixed (tdL.cl, msg_answer, len_msg) == false, false,
                  "[Thread %d] Eroare la write().\n",tdL.idThread);
    PRINT_MSG ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
  // }	
  return true;
}

bool verify_if_all_numbers(const char* msg, int numbers[], int* count)
{
  char tmp[MAX_LEN_IP + MAX_LEN_PORT + MAX_LEN_SERVICE_NAME];
  strcpy(tmp,msg);

  char* str = strtok(tmp," ");
  *count = 0;
  while(str != NULL)
  {
      if(is_number(str) == false)
          return false;
      numbers[*count] = atoi(str);    *count += 1;

      str = strtok(NULL," ");
  }

  return true;
}

bool is_number(const char* msg)
{
  if (msg == NULL) {
	return false;
  }
  int index = 0;
  while (index < strlen(msg)) {
	  if (!(msg[index] >= '0' && msg[index] <= '9')) 
      { return false; }
	  index += 1;
	} 
  return true;
}

int get_result_operation(int numbers[],int length)
{
  int result = numbers[0];
  for(int i = 1 ; i < length ; i++)
    result = OPERATOR(result,numbers[i]);
  return result;
}