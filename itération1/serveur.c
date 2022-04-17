// INCLUDE
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>

//DEFINE
#define PORT 2633
#define NBCL 30
#define TAILLE 30

//VAR
int connected; // Pour stocker le nombre de personne connecté 
int dSClient[NBCL]; // Pour stocker les dSC de chaques client
char names[NBCL][TAILLE]; // Pour stocler les noms des clients
int dispo[NBCL];// Pour stocker les disponibilités

pthread_t connecte[NBCL]; // threads

/// Fonction associé au threads de communications
void* communication(void *numcliact){
    int* numCliAct=numcliact;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    int taille;
    while(1){
        recv(dSClient[n],&taille,sizeof(int),0);  // Recoit la taille
        char *msg =(char*)malloc(taille*sizeof(char));
        int rec=recv(dSClient[n],msg,taille*sizeof(char),0); // Recoit le message 
        if(rec==0){ // Si le client a envoyé fin ou a arreté le programme, la réception échoue et l'on ferme la connection
            connected--;
            printf("%sdisconnect\n",names[n]);
            dispo[n]=0; 
            break;
        }
        char tosend[taille];
		printf("%s-> %s\n", names[n],msg); // Envoi du msg sous la forme: pseudo -> msg
        sprintf(tosend,"%s-> %s\n", names[n],msg);
        taille=strlen(tosend);   
        for(int i=0;i<connected;i++){
            if(i!=n){
                send(dSClient[i], &taille, sizeof(int), 0) ;
                send(dSClient[i], tosend, strlen(tosend)+1, 0) ;
            }
        }   
    }
    pthread_exit(NULL);
}




                //MAIN//


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
    for(int i=0;i<NBCL;i++)
    {
        dispo[i]=0;
    }
    connected=0;
    printf("En attente de connections clients...\n");

    while(1){
        struct sockaddr_in aC;
        socklen_t lg = sizeof(struct sockaddr_in);
        char name[TAILLE];
        while(connected<NBCL){
            int dSC=accept(dS,(struct sockaddr *)&aC,&lg);
            int i=0;
		    int test=0;
            while(test!=1){
			    if(dispo[i]==0){
				    test=1;
			    }
			    i++;
		    }
            int numCliAct=i-1;
            dSClient[numCliAct]=dSC;
            recv(dSClient[numCliAct],name,TAILLE*sizeof(char),0);
            strcpy(names[numCliAct],name);
            dispo[numCliAct]=1; 
            printf("Client number %d : %s  \n",numCliAct+1,names[numCliAct]);
            pthread_create(&connecte[numCliAct],NULL,communication,&numCliAct);
            connected++;    
        }
        printf("server is full\n");
        for(int i=0;i<NBCL;i++){
        pthread_join(connecte[i],NULL);
        }
    }
    printf("FINI");
    return 0;
}

