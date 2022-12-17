/* registerS.c - This is an example of a server whith the role of a register for other servers that will offer some services
    There will be 2 types of messages received by registerS:
    - "<ip_address> <port> <service_name>" (a server with a certain service sends this message at the opening);
    - "<service_name>"                     (a client that wants to call a service, asks where he can find that service).

Cerinta "RegisterS":
Sa se implementeze un server care va oferi clientilor informatii privitoare la serverele (adresa ip si port) care ofera serviciile dorite 
de acestia. Modalitatea de functionare este urmatoarea: atunci cand un server care ofera un anumit serviciu este pornit, acesta va trimite
serverului de inregistrare adresa ip, portul si numele serviciului oferit; atunci cand un client doreste un anumit serviciu, va trimite 
mai intai o cerere la serverul de inregistrare cu numele serviciului dorit, iar acesta ii va returna un mesaj ce va contine adresa ip si
portul serverului care ofera serviciul dorit.  
   
   Autor: Ignat Gabriel-Andrei  <gabriel.ignat.v09@gmail.com>
*/

#include "../Commons/common.c"

#define PORT 2777
#define IPv4 INADDR_ANY

extern int errno;

typedef struct thData{
	int idThread; /* id-ul thread-ului tinut in evidenta de acest program */
	int cl; /* descriptorul intors de accept */
}thData;

pthread_mutex_t mutex;

struct serviceStruct{
  char ip_address[MAX_LEN_IP];
  char port_number[MAX_LEN_PORT];
  char service_name[MAX_LEN_SERVICE_NAME];
}available_services[MAX_NR_SERVICES];

int  nr_services = 0; /* numarul de servicii disponibile la un moment de timp */

static void *treat(void *); /* functie executata de fiecare thread */
bool answer(void *);
bool is_msg_add_service(const char*, char*, char*, char*, bool*);
bool verify_if_service_is_available(const char*, const char*, bool*);
bool add_service(const char*, const char*, const char*, int);
void delete_service(int position);
bool display_available_services(char* msg, int nr_thread);

int main () /* fara argumente */
{
  ERROR_EXIT(pthread_mutex_init(&mutex, NULL) != 0, 
            "[register]Eroare la initializare mutex.\n"); /* pentru a evita race-condition */

  struct sockaddr_in  server,  
                      from;	
  int socket_server,
      i=0;
  pthread_t th[MAX_Threads];    /* Identificatorii thread-urilor care se vor crea */

  
  /* ------------- start get_ready_server */
  ERROR_EXIT((socket_server = socket (AF_INET, SOCK_STREAM, 0)) == -1, 
            "[register]Eroare la socket().\n");

  int on=1;     
  ERROR_EXIT(setsockopt(socket_server,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) != 0, 
            "[register]Eroare la setsockopt().\n");
  
  bzero (&server, sizeof (server));

    server.sin_family = AF_INET;	
    server.sin_addr.s_addr = htonl (IPv4);  
    server.sin_port = htons (PORT);

  ERROR_EXIT(bind (socket_server, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1 , 
            "[register]Eroare la bind().\n");
  ERROR_EXIT(listen (socket_server, MAX_LEN_QUEUE) == -1 , 
            "[register]Eroare la listen().\n");
  /* ------------- end get_ready_server */

  /* tratare clienti */
  while (1)
  {
      int client;
      thData * td;     
      socklen_t length = sizeof (from);
      bzero (&from, sizeof (from));

      PRINT_MSG ("[register]Asteptam la portul %d...\n",PORT);

      ERROR_CONTINUE( (client = accept (socket_server, (struct sockaddr *) &from, &length)) < 0,
                    "[register]Eroare la accept().\n");	

      ERROR_CONTINUE( (td = (struct thData*)malloc(sizeof(struct thData))) == NULL,
                    "[register]Eroare la malloc.\n");	
      td->idThread = i++;
      td->cl = client;
      
      /* thread CREATION */
      ERROR_CONTINUE(pthread_create(&th[i], NULL, &treat, td) != 0,
                    "[register]Eroare la phtread_create().\n");	
	}/* while */    
  ERROR_EXIT(close(socket_server) == -1,
            "[register]Eroare la close().\n");
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
  ERROR_RET_VOID_NULL( close ((intptr_t)arg) == -1, 
                      "[thread] - %d - Eroare la inchiderea clientului .\n", tdL.idThread);
  return(NULL);	
};


bool answer(void *arg)
{
  char  msg_received[MAX_LEN_IP + MAX_LEN_PORT + MAX_LEN_SERVICE_NAME],
        msg_answer[MAX_LEN_IP + MAX_LEN_PORT + MAX_LEN_SERVICE_NAME];
  char  service[MAX_LEN_SERVICE_NAME], 
        ip_addr[MAX_LEN_IP], 
        port_nr[MAX_LEN_PORT];

	struct thData tdL; 
	tdL= *((struct thData*)arg);
  bool res;

  while(1)
  {
    ERROR_RET( read_with_length_prefixed(tdL.cl, msg_received) == false, false,
              "[Thread %d] Eroare la read() de la client.\n",tdL.idThread);
    
    PRINT_MSG ("[Thread %d]Mesajul a fost receptionat...%s\n",tdL.idThread, msg_received);

    ERROR_RET(is_msg_add_service(msg_received, ip_addr, port_nr, service,&res) == false, false,
              "[Thread %d] Eroare la is_msg_add_service().\n",tdL.idThread);

    if(res == true) /* mesajul primit este de la un server */
    {
      /* sectiune critica -> folosim mutex  */
      ERROR_RET(pthread_mutex_lock(&mutex) == -1, false,
                "[Thread %d] Eroare la phtread_mutex_lock().\n",tdL.idThread);
      ERROR_RET(add_service(ip_addr, port_nr, service, tdL.idThread) == false, false, /*adaugare serviciu*/
                "[Thread %d] Eroare la add_services().\n",tdL.idThread);
      ERROR_RET(pthread_mutex_unlock(&mutex) == -1, false,
                "[Thread %d] Eroare la phtread_mutex_unlock().\n",tdL.idThread);
    }  
    else  /* altfel mesajul primit este un request de la client care cere informatii despre un anumit serviciu */
    {
        /* sectiune critica -> folosim mutex  */
        ERROR_RET(pthread_mutex_lock(&mutex) == -1, false,
                  "[Thread %d] Eroare la phtread_mutex_lock().\n",tdL.idThread);

        int  i;
        for(i = 0 ; i < nr_services ; i++)  //cautare serviciu + eliminare servicii indisponibile
        {  
          ERROR_RET(verify_if_service_is_available(available_services[i].ip_address, available_services[i].port_number, &res) == false, false,  //verificare daca serverul este pornit
                          "[Thread %d] Eroare la verify_if_service_is_available().\n",tdL.idThread);
          if(res == false)
          {
            delete_service(i);
            i--;
          }
          else
            if(strcmp(available_services[i].service_name,msg_received) == 0)  //msg_answer va contine mesajul "<ip_address> <port_number>"
              {
                ERROR_RET(strcpy(msg_answer,available_services[i].ip_address) == NULL, false,
                          "[Thread %d] Eroare la strcpy().\n",tdL.idThread);
                ERROR_RET(strcat(msg_answer," ") == NULL, false,
                          "[Thread %d] Eroare la strcat().\n",tdL.idThread);
                ERROR_RET(strcat(msg_answer,available_services[i].port_number) == NULL, false,
                          "[Thread %d] Eroare la strcat().\n",tdL.idThread);
                break;	
              }
        }

        if(i == nr_services)  // daca serviciul cautat nu este in lista
          {
            ERROR_RET(strcpy(msg_answer,"Service name doesn't exist.\n Try these: \n") == NULL, false,
                      "[Thread %d] Eroare la strcat().\n",tdL.idThread);
            ERROR_RET(display_available_services(msg_answer, tdL.idThread) == false, false,
                      "[Thread %d] Eroare la display_available_services().\n",tdL.idThread);
          }
          else  //altfel, daca este in lista
          { ERROR_RET(verify_if_service_is_available(available_services[i].ip_address, available_services[i].port_number, &res) == false, false,  //verificare daca serverul este pornit
                        "[Thread %d] Eroare la verify_if_service_is_available().\n",tdL.idThread);
            if(res == false)  /* serviciul s-a inchis intre timp */
            {
              ERROR_RET(strcpy(msg_answer,"Service not available.\n Try these: \n") == NULL, false,
                      "[Thread %d] Eroare la strcat().\n",tdL.idThread);
              ERROR_RET(display_available_services(msg_answer, tdL.idThread) == false, false,
                      "[Thread %d] Eroare la display_available_services().\n",tdL.idThread);
            }
          }

        ERROR_RET(pthread_mutex_unlock(&mutex) == -1, false,
                  "[Thread %d] Eroare la phtread_mutex_unlock().\n",tdL.idThread);

        PRINT_MSG("[Thread %d]Trimitem mesajul inapoi...%s\n",tdL.idThread, msg_answer)
  
        int len_msg = strlen(msg_answer);
        ERROR_RET( write_with_length_prefixed (tdL.cl, msg_answer, len_msg) == false, false,
                  "[Thread %d] Eroare la write().\n",tdL.idThread);

        PRINT_MSG ("[Thread %d]Mesajul a fost trasmis cu succes.\n\n",tdL.idThread)
    } /*else*/
  } /*while*/
  return true;	
}

bool is_msg_add_service(const char *msg, char* ip_addr, char* port_nr, char* service, bool* res) //se verifica daca primul cuvant este o adresa IP si al doilea cuvant este PORT
{ 
  *res = true;
  char tmp[MAX_LEN_SERVICE_NAME + MAX_LEN_IP + MAX_LEN_PORT];
  ERROR_RET(strcpy(tmp,msg) == NULL,  false,
            "Eroare la strcpy() in thread.\n");

  char* str1 = strchr(msg,' ');
  if(str1 == NULL)   { *res = false; return true; }
  
  int pos1 = str1 - msg;
  ERROR_RET(strcpy(tmp,msg) == NULL, false,
            "Eroare la strcpy() in thread.\n");
  tmp[pos1] = '\0';
  if(check_is_address_valid(tmp) == false)
    {
      *res = false;
      return true;
    }
  ERROR_RET(strcpy(ip_addr,tmp) == NULL, false,
            "Eroare la strcpy() in thread.\n");

  char* str2 = strchr(str1 + 1,' ');
  if(str2 == NULL)   { *res = false;  return true;  }

  int pos2 = str2 - str1;
  ERROR_RET(strcpy(tmp, str1 + 1) == NULL, false,
            "Eroare la strcpy() in thread.\n");
  tmp[pos2 - 1] = '\0';
  if(check_is_port_valid(tmp) == false) 
    { *res = false; return true;  }
  ERROR_RET(strcpy(port_nr,tmp) == NULL, false,
            "Eroare la strcpy() in thread.\n");
  ERROR_RET(strcpy(service,str1 + pos2 + 1) == NULL, false,
            "Eroare la strcpy() in thread.\n");
  *res = true;
  return true;
}

bool verify_if_service_is_available(const char* ip_addr, const char* port_nr, bool* res)  //verificare activitate la adresa ip si portul serviciului gasit
{ 
  *res = true;
  char command[LARGE_LEN_BUFFER];
  ERROR_RET(sprintf(command,"nc -z -v %s %s 2>&1", ip_addr, port_nr) < 0 , false,
            "Eroare la sprintf() in thread.\n");

  FILE *fp = popen(command,"r");
  if(fp == NULL)  { *res = false; return true;  }

  char buffer[LARGE_LEN_BUFFER];
  fgets(buffer,LARGE_LEN_BUFFER,fp);
  pclose(fp);
  
  if(strstr(buffer,"succeeded") == NULL)
    { *res = false; return true;}
  *res = true;
  return true;
}

bool add_service(const char* ip_addr, const char* port_nr, const char* service, int nr_thread)
{
  for(int i = 0 ; i < nr_services ; i++)  /* evitam scrierea de duplicate: aceeasi adresa IPv4 si acelasi PORT: serverul a fost oprit, dar nu s-a sters din vector, si apoi a fost repornit*/
  {
    if( strcmp(available_services[i].port_number,port_nr) == 0 &&
        strcmp(available_services[i].service_name,service) == 0)
    return true;
  }

  ERROR_RET(strcpy(available_services[nr_services].ip_address   ,ip_addr) == NULL, false,
            "[Thread %d] Eroare la strcpy().\n",nr_thread);
  ERROR_RET(strcpy(available_services[nr_services].port_number ,port_nr) == NULL, false,
            "[Thread %d] Eroare la strcpy().\n",nr_thread);
  ERROR_RET(strcpy(available_services[nr_services].service_name,service) == NULL, false,
            "[Thread %d] Eroare la strcpy().\n",nr_thread);
  nr_services++;
  return true;
}

void delete_service(int position)
{
  for(int j = position ; j < nr_services - 1; j++)
  {
    available_services[j] = available_services[j + 1];
  }
  nr_services--;
}

bool display_available_services(char* msg_answer, int nr_thread)
{
  bool res = true;
  for(int j = 0 ; j < nr_services ; j++)  /* se afiseaza doar acele servicii disponibile */
  {
    ERROR_RET(verify_if_service_is_available(available_services[j].ip_address, available_services[j].port_number, &res) == false, false,
                "[Thread %d] Eroare la verify_if_service_is_available().\n",nr_thread);
    if(res == false)
      {
        delete_service(j);
        j--;
      }
      else
      {
        ERROR_RET(strcat(msg_answer,available_services[j].service_name)== NULL, false,
                  "[Thread %d] Eroare la strcat().\n",nr_thread);
        ERROR_RET(strcat(msg_answer,"\n") == NULL, false,
                  "[Thread %d] Eroare la strcat().\n",nr_thread);
      }
  }
  return true;
}