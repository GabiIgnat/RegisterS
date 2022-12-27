/* client.c - This is an example of a client that asks for information from RegisterS and communicates with another server
   Send a name of a service(other server) to RegisterS; receive from server "<ip_address> <port_number>".
         
   Autor: Ignat Gabriel-Andrei  <gabriel.ignat.v09@gmail.com>
*/
#include "../Commons/common.c"

extern int errno;

char msg_received[MAX_LEN_SERVICE_NAME];
char msg_answer[MAX_LEN_SERVICE_NAME];
char ip_address[MAX_LEN_IP];
char port_number[MAX_LEN_PORT];
char interactive;

int port;

int main (int argc, char *argv[])
{
  //  -------------  conecct to RegisterS
  int socket_register, socket_service;
  struct sockaddr_in server, server_service;

  ERROR_EXIT((argc != 3) ,
            "[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
  ERROR_EXIT(check_is_address_valid(argv[1]) == false,
            "[client] Adresa IPv4 invalida.\n");
  ERROR_EXIT(check_is_port_valid(argv[2]) == false,
            "[client] PORT invalid")
  
  ERROR_EXIT((socket_register = socket (AF_INET, SOCK_STREAM, 0)) == -1, 
            "Eroare la socket().\n");

    port = atoi (argv[2]);
    server.sin_family = AF_INET;  
    server.sin_addr.s_addr = inet_addr(argv[1]);    
    server.sin_port = htons (port);
    
  ERROR_EXIT(connect (socket_register, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1 ,
            "[client]Eroare la connect().\n");
  //  ------------- end connect to RegisterS

  while(1)  //cerere info despre un serviciu si utilizarea serviciului dorit
  {
    bzero(msg_answer,MAX_LEN_SERVICE_NAME);
    bzero(msg_received,MAX_LEN_SERVICE_NAME);
    
    PRINT_MSG("[client] Doriti sa aflati informatii despre un anumit serviciu? (Y/N)(X for exit): "); 
    read(0,&interactive,sizeof(char) + 1);

    switch(toupper(interactive))
    {
      case 'X' :  
                  exit(EXIT_SUCCESS);
                  break;
      case 'Y' : 
                  PRINT_MSG("[client] Introduceti un nume de serviciu: ");
                  int len_msg = read (0, msg_answer, MAX_LEN_SERVICE_NAME);
                  msg_answer[len_msg] = '\0';
                  
                  ERROR_EXIT(write_with_length_prefixed(socket_register,msg_answer,len_msg-1) == false,
                            "[client]Eroare la write() spre server.\n")

                  ERROR_EXIT(read_with_length_prefixed(socket_register, msg_received) == false,
                            "[client]Eroare la read() de la server.\n");

                  PRINT_MSG ("[client] Informatii despre serviciul %s : %s\n", msg_answer, msg_received);
                  break;
      default  :  
                  break;
    }
    
    PRINT_MSG("[client] Ati aflat informatiile dorite? (Y/N)(X for exit): "); 
    read(0,&interactive,sizeof(char) + 1);

    switch(toupper(interactive))
    {
      case 'X' :  
                  exit(EXIT_SUCCESS);
                  break;
      case 'Y' :
                  //  ------------- conectare la un anume serviciu
                  PRINT_MSG("[client] Introduceti adresa IP a serviciului dorit: "); 
                  int len_msg = read(0,ip_address,MAX_LEN_IP);      
                  ip_address[len_msg - 1] ='\0';
                  ERROR_CONTINUE(check_is_address_valid(ip_address) == false, 
                                "[client] Adresa IPv4 introdusa nu este valida.\n");

                  PRINT_MSG("[client] Introduceti PORT-ul serviciului dorit: ");
                  len_msg = read(0,port_number,MAX_LEN_PORT);   
                  port_number[len_msg - 1] = '\0';
                  ERROR_CONTINUE(check_is_port_valid(port_number) == false,
                                "[client] PORT-ul introdus nu este valid.\n");

                  port = atoi(port_number);
                  ERROR_CONTINUE((socket_service = socket (AF_INET, SOCK_STREAM, 0)) == -1,
                                "[client] Eroare la socket().\n");
                  server_service.sin_family = AF_INET;  
                  server_service.sin_addr.s_addr = inet_addr(ip_address);    
                  server_service.sin_port = htons (port);
                  ERROR_CONTINUE(connect (socket_service, (struct sockaddr *) &server_service,sizeof (struct sockaddr)) == -1,
                                "[client]Eroare la connect().\n");
                  
                  PRINT_MSG("[client] Introduceti informatia pentru procesare(conform serviciului ales): ");
                  len_msg = read (0, msg_answer, MAX_LEN_SERVICE_NAME);
                  msg_answer[len_msg] = '\0';
                  
                  ERROR_CONTINUE(write_with_length_prefixed(socket_service,msg_answer,len_msg-1) == false,
                                "[client]Eroare la write() spre server.\n");
                  ERROR_CONTINUE(read_with_length_prefixed(socket_service, msg_received) == false,
                                "[client]Eroare la read() de la server.\n");
                  
                  PRINT_MSG ("[client] Rezultat: %s\n\n\n",msg_received);

                  ERROR_CONTINUE(close(socket_service) == -1,
                                "[client]Eroare la close()");
                  // ------------- end executie serviciu
                  break;
      default  : 
                  PRINT_MSG("Incercati din nou...\n"); 
                  break;
    }

}
  /* inchidem conexiunea, am terminat */
  ERROR_EXIT(close (socket_register) == -1,
            "[client]Eroare la close()");
}