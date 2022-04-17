#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>

#define PORT 2633
#define NBCL 10
#define TAILLE 30


int connected;
int dSClient[NBCL];
char names[NBCL][TAILLE];
int dispo[NBCL][1];

pthread_t connecte[NBCL];

void* communication(void *numcliact){
    printf("OKKKKKKKK");
    int* numCliAct=numcliact;
    int taille;
    int n=*numCliAct-1;
    while(1){
        recv(dSClient[n],&taille,sizeof(int),0); 
        char *msg =(char*)malloc(taille*sizeof(char));
        int rec=recv(dSClient[n],msg,taille*sizeof(char),0);
        printf("cassé\n");
        if(rec==0){
            connected--;
           // dispo[n][0]=0; 
            break;
        }
        printf("cassé\n");

        char tosend[taille];
		printf("%s-> %s\n", names[n],msg);
        sprintf(tosend,"%s-> %s\n", names[n],msg);
        taille=strlen(tosend);   
        for(int i=0;i<connected;i++){
            if(i!=n){
                send(dSClient[i], &taille, sizeof(int), 0) ;
              //  printf("Taille Envoyé\n");
                send(dSClient[i], tosend, strlen(tosend)+1, 0) ;
              //  printf("Message Envoyé\n");
            }
        }   
    }
    pthread_exit(NULL);
}





//////////////////////////////
void *connection(void *ds) {   
    int* dS=ds;
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    char name[TAILLE];
    while(connected<NBCL){
        int i=0;
		int test=0;
        while(test!=1){
			if(dispo[i][0]==0){
				test=1;
			}
			i++;
		}
        int numCliAct=i-1;
        int dSC=accept(*dS,(struct sockaddr *)&aC,&lg);
        dSClient[numCliAct]=dSC;
        recv(dSClient[numCliAct],name,TAILLE*sizeof(char),0);
        strcpy(names[numCliAct],name);
        dispo[numCliAct][0]=1; 
        printf("Client number %d : %s  \n",numCliAct+1,names[numCliAct]);
        pthread_create(&connecte[numCliAct],NULL,communication,&numCliAct);
        connected++;    
    }
    pthread_exit(NULL);
}
/////////////////////////////////

int main(int argc, char *argv[]) {
///////////////////CONNECTION//////////////////////////
    printf("Début programme\n");
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Créé\n");
    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY ;
    ad.sin_port = htons(PORT) ;
    if (bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) {
        perror("erreur nommer socket");
        };
    printf("Socket Nommé\n");
//////////////////////////////////////////////////////
    listen(dS, 7) ;
    printf("Mode écoute\n");
    while(1){
        for(int i=0;i<NBCL;i++)
        {
            dispo[i][0]=0;
        }
        connected=0;
        printf("En attente de connections clients...\n");
        pthread_t thread;
        pthread_create(&thread,0,connection,(void*)&dS);
        pthread_join(thread,NULL);
    }
    printf("FINI");
    return 0;

}



    // struct sockaddr_in aC ;
    // socklen_t lg = sizeof(struct sockaddr_in) ;
    // int dSC = accept(dS, (struct sockaddr*) &aC,&lg) ;
    // if (dSC == -1) {
    //     perror("erreur connection client");
    // };
    // printf("Client Connecté\n%D");  






//     listen(dS, 7) ;
//     printf("Mode écoute\n");

    
//     struct sockaddr_in aC ;
//     socklen_t lg = sizeof(struct sockaddr_in) ;
//     int dSC = accept(dS, (struct sockaddr*) &aC,&lg) ;
//     if (dSC == -1) {
//         perror("erreur connection client");
//     };
//     printf("Client Connecté\n");

//     int taille;
//     if (recv(dSC, &taille, sizeof(int), 0) == -1) {
//         perror("erreur connection client");
//     };
//     printf("Taille reçu : %d\n", taille) ;

//     char *msg =(char*)malloc(taille*sizeof(char));

//     if (recv(dSC, msg, taille , 0) == -1) {
//         perror("erreur connection client");
//     };
//     printf("Message reçu : %s", msg) ;
    
//     int r = 10 ;

//     /* ---------- Connexion 2èm client ---------- */

//     listen(dS, 7) ;
//     printf("Mode écoute 2ème client\n");

//     struct sockaddr_in aC2 ;
//     socklen_t lg2 = sizeof(struct sockaddr_in) ;
//     int dSC2 = accept(dS, (struct sockaddr*) &aC2,&lg2) ;
//     if (dSC2 == -1) {
//         perror("erreur connection client");
//     };
//     printf("Client Connecté\n");

//     /* ---------- Envoie message 2èm client ---------- */
//     /* ---------- Envoie taille ---------- */

//     send(dSC2, &taille, sizeof(int), 0) ;
//     printf("Taille Envoyé\n");

//     /* ---------- Envoie message ---------- */
    
//     send(dSC2, msg, strlen(msg)+1, 0) ;
//     printf("Message Envoyé\n");
//     shutdown(dSC, 2) ; 
//     shutdown(dSC2, 2) ;

//   shutdown(dS, 2) ;
//   printf("Fin du programme\n");

