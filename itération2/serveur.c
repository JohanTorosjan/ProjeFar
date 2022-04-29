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
#include <semaphore.h>

//DEFIN3
#define PORT 2637
#define NBCL 2
#define TAILLE 200

//VAR
int connected; // Pour stocker le nombre de personne connecté 
int dSClient[NBCL]; // Pour stocker les dSC de chaques client
char names[NBCL][TAILLE]; // Pour stocler les noms des clients
int dispo[NBCL];// Pour stocker les disponibilités
char availablescommands[10][10];
pthread_t connecte[NBCL]; // threads

sem_t semaphore; // semaphore
pthread_mutex_t mutex; // mutex
int nbcommand;

/// @mp:pseudo:msg 
/// @command::

void innitialisecommands(){
    FILE *f;
    char c;
    f=fopen("man.txt","rt");
    fscanf(f,"%s\n%s\n%s",availablescommands[0],availablescommands[1],availablescommands[2]);
    nbcommand=3;
    fclose(f);
    // char mp[]="@mp";
    // char fin[]="@end";
    // char man[]="@man";
    // strcpy(availablescommands[0],mp);
    // strcpy(availablescommands[1],fin);
    // strcpy(availablescommands[2],man);
}


/// command : char* msg ----> Int 
/// -1 si le message n'est pas une commande 
/// 0 si la commmande n'est pas valable 
// i+1 correspondant à l'indice +1 dans de le tableau des commandes de la commande si elle existe 
int command(char* msg){
    if(msg[0]=='@'){
        char* msgcopy=malloc(strlen(msg)+1);
        strcpy(msgcopy,msg);
        char* token=strtok(msgcopy,":");
        if(strlen(token)>4 || token==NULL){
            return 0;
        }
        for(int i=0;i<strlen(*availablescommands);i++){
            if(strcmp(token,availablescommands[i])==0){
                return i+1;
            }     
        }
        return 0;
    }
    else{
        return -1;
    }
}

//// SendPrivateMessage : char* msg, int n ---> Int
/// -1 Si le pseudo n'existe pas 
/// i qui correspond à l'indice dans le tableau des noms  du pseudo correspondant 
int SendPrivateMessage(char* msg,int n){
    char* pseudo;
    char copy[strlen(msg)+1];
    strcpy(copy,msg);
    char * msgcopy;
    msgcopy=strtok(copy,":");
    msgcopy=strtok(NULL,":");
    msgcopy=strtok(NULL,"\0");
    pseudo=strtok(msg,":");
    pseudo=strtok(NULL,":");
    for(int i=0;i<connected;i++){
        if(strcmp(names[i],pseudo)==0){
            printf("Private message from %s for %s: %s\n",names[n],names[i],msgcopy);
            int taille=strlen(msgcopy)+strlen(names[n])+20; 
            char tosend[taille];
            sprintf(tosend,"PV from %s-> %s\n", names[n],msgcopy);
            send(dSClient[i],&taille, sizeof(int), 0) ; 
            send(dSClient[i], tosend, strlen(tosend)+1, 0) ;   
            return i;
            } 
    }
    return -1;
     }

   


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
            
            //// DECONNECTION : 
            pthread_mutex_lock(&mutex);
            connected--;
            printf("%s disconnect\n",names[n]);
            strcpy(names[n],"");
            dispo[n]=0; 
            sem_post(&semaphore);
            pthread_mutex_unlock(&mutex);
            break;
            ////////////////////////
        }
        char tosend[taille];
        int commande=command(msg);
        //Si le message contient une commande
        if(commande>0){
            if(commande==1){ //Si c'est une commande mp 
                int i=SendPrivateMessage(msg,n);
                if(i>=0){
                    printf("Message pv envoyé\n");
                }
                else{
                    printf("No user with this pseudo\n");
                    sprintf(tosend,"No user with this pseudo");
                    taille=strlen(tosend)+1;
                    send(dSClient[n], &taille, sizeof(int), 0) ;
                    send(dSClient[n], tosend, strlen(tosend)+1, 0) ;    
                }
            }
           
            if(commande==2){ // Si c'est une commande end
               
                //// DECONNECTION : 
                pthread_mutex_lock(&mutex);
                connected--;
                printf("%s disconnect\n",names[n]);
                strcpy(names[n],"");
                dispo[n]=0; 
                sem_post(&semaphore);
                pthread_mutex_unlock(&mutex);
                break;
                ////////////////////////
            
            }

            if(commande==3){
                taille=nbcommand*18;
                char man[taille];
                char com[18];
                for(int i=0;i<nbcommand;i++){
                   // sprintf(man,"%s::\n",availablescommands[i]);
                   strcpy(com,availablescommands[i]);
                   strcat(com,":[optional]:\n");
                    strcat(man,com);
                    // strcat(src,availablescommands[i]);
                    printf("%s\n",availablescommands[i]);
                    
                }
                send(dSClient[n], &taille, sizeof(int), 0) ;
                send(dSClient[n],man, strlen(man)+1, 0) ;
                printf("MAN : %s\n",man);
            }




            // printf("%s-> %s\n", names[n],msg); // Envoi du msg sous la forme: pseudo -> msg
            // sprintf(tosend,"%s-> %s\n", names[n],msg);
            // taille=strlen(tosend);   
            // for(int i=0;i<connected;i++){
            //     if(i!=n && strcmp(names[i],"")!=0){
            //         send(dSClient[i], &taille, sizeof(int), 0) ;
            //         send(dSClient[i], tosend, strlen(tosend)+1, 0) ;
            //     }







       
        }
        //Si la commande n'est pas valide 
        else if(commande==0){
            sprintf(tosend,"This is not a vailable command");
            taille=strlen(tosend)+1;
            send(dSClient[n], &taille, sizeof(int), 0) ;
            send(dSClient[n], tosend, strlen(tosend)+1, 0) ;    
        }

        // Si ce n'est pas une commande 
        else{
            printf("%s-> %s\n", names[n],msg); // Envoi du msg sous la forme: pseudo -> msg
            sprintf(tosend,"%s-> %s\n", names[n],msg);
            taille=strlen(tosend);   
            for(int i=0;i<connected;i++){
                if(i!=n && strcmp(names[i],"")!=0){
                    send(dSClient[i], &taille, sizeof(int), 0) ;
                    send(dSClient[i], tosend, strlen(tosend)+1, 0) ;
                }
            }
        }   
    }
    pthread_exit(NULL);
}



                //MAIN//


int main(int argc, char *argv[]) {
    innitialisecommands();
///////////////////CONNECTION//////////////////////////
    printf("Starting server\n");
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY ;
    ad.sin_port = htons(PORT) ;
    if (bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) {
        perror("erreur nommer socket");
        };

//////////////////////////////////////////////////////
    listen(dS, 7) ;
    for(int i=0;i<NBCL;i++)
    {
        dispo[i]=0;
    }
    connected=0;
    printf("Waiting for clients...\n");
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    char name[TAILLE];
    sem_init(&semaphore, PTHREAD_PROCESS_SHARED, NBCL); // initialisation semaphore
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // initialisation mutex
    int dSC;
    int accepted=1;
    while(1){
        
        dSC=accept(dS,(struct sockaddr *)&aC,&lg);
        if(connected>=NBCL){accepted=0;}
        send(dSC,&accepted, sizeof(int), 0) ; 
        while(connected==NBCL){
            accepted=0;
            dSC=accept(dS,(struct sockaddr *)&aC,&lg);
            if(connected<NBCL){accepted=1;}
            send(dSC,&accepted, sizeof(int), 0) ; 
        } 
        sem_wait(&semaphore); //  semaphore attente
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
        int rec=recv(dSClient[numCliAct],name,TAILLE*sizeof(char),0);
        if(rec !=0){
            char *pos;
            pos=strchr(name,'\n');
            *pos='\0';
            test=0; // On test si le pseudo recu existe deja 
            for(int i=0;i<NBCL;i++){
                if(strcmp(names[i],name)==0){
                    test=1;
                }
            }
            if(test==0){ // Si il n'existe pas 
                send(dSClient[numCliAct], &test, sizeof(int), 0) ;
                pthread_mutex_lock(&mutex); //mutex lock 
                strcpy(names[numCliAct],name);
                dispo[numCliAct]=1; 
                connected++;   
                pthread_mutex_unlock(&mutex); //mutex unlock
                printf("Client number %d : %s  \n",numCliAct+1,names[numCliAct]);
                pthread_create(&connecte[numCliAct],NULL,communication,&numCliAct);
                
            }
            else{ // Si il existe 
                    send(dSClient[numCliAct], &test, sizeof(int), 0) ;
                    dSC=0;
                    sem_post(&semaphore);
            } 
        }
        else{   
            sem_post(&semaphore);
        }
    }
    sem_destroy(&semaphore);
    printf("EXIT\n");
    return 0;
}

