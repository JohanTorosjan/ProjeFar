#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#define PORT 2633
#define IP "192.168.1.49"
#define TAILLE 30
char pseudo[TAILLE];
char fin[]="fin\n";
int dS;


//Fonction d'envoi : 
void *envoi() {
  char message[TAILLE];
  while(1){
    printf("\n");
    fgets(message,TAILLE,stdin);
    int taille=(strlen(message)+1)*sizeof(char);
    send(dS,&taille,sizeof(int),0);
    send(dS,message,(strlen(message)+1)*sizeof(char),0);
    if(strcmp(message,fin)==0){
      printf("#################\n");
      exit(0);
    }
  }
}

//Fonction de reception :
void *reception(void *ds) {
  while(1){
    int taille;
    recv(dS, &taille, sizeof(int), 0) ;
    char *msg = (char*)malloc(taille*sizeof(char));
    recv(dS, msg, taille, 0) ;
    printf("\n");
    puts(msg);
  }
}

int main(int argc, char *argv[]) {
  pthread_t thread_reception;
	pthread_t thread_envoi;
  /////////////////// CONNECTION //////////////////////////////
  printf("Début programme\n");
  dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1) {
      perror("erreur dans l'initialisation du socket");
  }
  printf("Socket Créé\n");
  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,IP,&(aS.sin_addr)) ;
  aS.sin_port = htons(PORT) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;
  if (connect(dS, (struct sockaddr *) &aS, lgA) == -1 ) {
      perror("erreur dans la connection du socket");
      exit(0);
  }
  printf("Socket Connecté\n");
  ///////////////////////////////////////////////////////////
  printf("################################\n");
  printf("Choose a pseudo: ");
	fgets(pseudo,TAILLE,stdin);
  send(dS,pseudo,(strlen(pseudo)+1)*sizeof(char),0);
  printf("Welcome,you're connected as %s\n",pseudo);

  // Innitialisation des thread //
  pthread_create(&thread_envoi,NULL,envoi,NULL);
  pthread_create(&thread_reception,NULL,reception,NULL);
  pthread_join(thread_envoi,NULL);
  pthread_join(thread_reception,NULL);
  return 0;
}






// int main(int argc, char *argv[]) {
//     printf("Début programme\n");
//     int ID = atoi(argv[3]);
//     int dS = socket(PF_INET, SOCK_STREAM, 0);
//     if (dS == -1) {
//         perror("erreur dans l'initialisation du socket");
//     }
//     printf("Socket Créé\n");
//     struct sockaddr_in aS;
//     aS.sin_family = AF_INET;
//     inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
//     aS.sin_port = htons(atoi(argv[2])) ;
//     socklen_t lgA = sizeof(struct sockaddr_in) ;
//     if (connect(dS, (struct sockaddr *) &aS, lgA) == -1 ) {
//         perror("erreur dans la connection du socket");
//         exit(0);
//     }
//     printf("Socket Connecté\n");
//     if (ID == 1) {

   
//     char *buffer =(char*)malloc(20*sizeof(char));
//     printf("Entrez un message de taille 20 max: \n");
//     fgets(buffer,20*sizeof(char),stdin); // stdin = entrée terminal
//     int taille = strlen(buffer);
//     if (send(dS, &taille, sizeof(int) , 0) == -1 ) { // +1 pour ajouter délimiteur
//         perror("erreur dans l'envoie du socket");
//     }
//     printf("/////////////\n");
//     if (send(dS, buffer, strlen(buffer)+1 , 0) == -1 ) { // +1 pour ajouter délimiteur
//         perror("erreur dans l'envoie du socket");
//     }
//     printf("Message Envoyé \n");
//   }
//   if (ID == 2) {
//     int taille;
//     recv(dS, &taille, sizeof(int), 0) ;
//     printf("/////////////\n");
//     char *retour = (char*)malloc(taille*sizeof(char));
//     recv(dS, retour, taille, 0) ;
//     printf("Réponse reçue : %s\n", retour) ;

//   }    
//   shutdown(dS,2) ;
//   printf("Fin du programme\n");
// }






// void *envoi(void *ds) {
//     int* dS=ds;
//     char *buffer =(char*)malloc(20*sizeof(char));
//     printf("Entrez un message de taille 20 max: \n");
//     fgets(buffer,20*sizeof(char),stdin); // stdin = entrée terminal
//     int taille = strlen(buffer);
//     if (send(*dS, &taille, sizeof(int) , 0) == -1 ) { // +1 pour ajouter délimiteur
//         perror("erreur dans l'envoie du socket");
//     }
//     printf("/////////////\n");
//     if (send(*dS, buffer, strlen(buffer)+1 , 0) == -1 ) { // +1 pour ajouter délimiteur
//         perror("erreur dans l'envoie du socket");
//     }
//     printf("Message Envoyé \n");
//   }


// void *reception(void *ds) {
//   int* dS=ds;
//   int taille;
//   recv(*dS, &taille, sizeof(int), 0) ;
//   printf("/////////////\n");
//   char *retour = (char*)malloc(taille*sizeof(char));
//   recv(*dS, retour, taille, 0) ;
//   printf("Réponse reçue : %s\n", retour) ;
  
// }

// int main(int argc, char *argv[]) {

//   pthread_t thread_envoi;
//   pthread_t thread_reception;

// /////////////////////////CONNECTION/////////////////////////////// 
//   int dS = socket(PF_INET, SOCK_STREAM, 0);
//     if (dS == -1) {
//         perror("erreur dans l'initialisation du socket");
//     }
//     printf("Socket Créé\n");
//     struct sockaddr_in aS;
//     aS.sin_family = AF_INET;
//     inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
//     aS.sin_port = htons(atoi(argv[2])) ;
//     socklen_t lgA = sizeof(struct sockaddr_in) ;
//     if (connect(dS, (struct sockaddr *) &aS, lgA) == -1 ) {
//         perror("erreur dans la connection du socket");
//         exit(0);
//     }
//     printf("Socket Connecté\n");
// ////////////////////////////////////////////////////////////////////


//     pthread_create(&thread_envoi,0,envoi,(void*)&dS);
//     pthread_create(&thread_reception,0,reception,(void*)&dS);
//   pthread_join(thread_envoi,0);
//   pthread_join(thread_reception,0);
//   shutdown(dS,2);
// exit(0);


// }







