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
#include <dirent.h>

//DEFIN3
#define PORT 2637
#define NBCL 2
#define TAILLE 200
#define PORTforFILE 2638
#define SIZE 1024
#define KEY "KHFZIEjkfejfkfkjdsfkjKJNFVEOKJFKLE?f;ld,sgklezj,gkpJfgkLekoẑakeopfkfopezkgopekzfezjgLKKGJLEFLK?KLEdgdslfkdslgkseldmgk;ezldjgkdjgOZJFKIOEPJPOA98787893U0948jgkjdfks"

//VAR
int connected; // Pour stocker le nombre de personne connecté 
int dSClient[NBCL]; // Pour stocker les dSC de chaques client
char names[NBCL][TAILLE]; // Pour stocler les noms des clients
int dispo[NBCL];// Pour stocker les disponibilités
char availablescommands[10][10];
char *FilesAvailables;
int dSclientFiles[NBCL]; // Pour stocker les dSC associé à l'envoi de fichier

int dSF;

pthread_t connecte[NBCL]; // threads
pthread_t files_receive[NBCL];
pthread_t files_send[NBCL];

sem_t semaphore; // semaphore
pthread_mutex_t mutex; // mutex
int nbcommand;


void innitialisecommands(){
    FILE *f;
    char c;
    f=fopen("man.txt","rt");
    fscanf(f,"%s\n%s\n%s\n%s\n%s\n%s",availablescommands[0],availablescommands[1],availablescommands[2],availablescommands[3],availablescommands[4],availablescommands[5]);
    nbcommand=6;
    fclose(f);
}

int ListServerFiles(){
    DIR *d = opendir("./ServerStorage");
    struct dirent *dir;
    int cpt=1;
    if (d)
       {
        while ((dir = readdir(d)) != NULL){
        if(dir->d_type==8){
            cpt=strlen(dir->d_name)+cpt;
        }
      }
        d = opendir("./ServerStorage");
        FilesAvailables=malloc(cpt*2);
        cpt=1;
        while ((dir = readdir(d)) != NULL){
            if(dir->d_type==8){
                char* namefile=malloc(strlen(dir->d_name)+6);
                sprintf(namefile,"%d : %s\n",cpt,dir->d_name);
                strcat(FilesAvailables,namefile);
                cpt++;
            }  
      }
    closedir(d);
    strcat(FilesAvailables,"\nUse @get:[FILE WANTED]: to get a file\n");
    return cpt-1;
    }
}
char* getfilename(int n){
  char* filename;
  DIR *d = opendir("./ServerStorage");
  struct dirent *dir;
  int cpt=1;
  if (d)
    {
      while ((dir = readdir(d)) != NULL){
          if(cpt==n){
            filename=dir->d_name;
          }
          cpt++;
      }
    closedir(d);
    }
    return filename;
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
        for(int i=0;i<nbcommand;i++){
            printf("%s\n",availablescommands[i]);
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



void * receive_file(void *numcliact){
    int* numCliAct=numcliact;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    struct sockaddr_in aCF;
    socklen_t lgF = sizeof(struct sockaddr_in);
    dSclientFiles[n]=accept(dSF,(struct sockaddr *)&aCF,&lgF);
    ///
    int taille;
    recv(dSclientFiles[n],&taille,sizeof(int),0); 
    if(taille<0){
        pthread_exit(NULL);
    }
    char *filename =(char*)malloc(taille*sizeof(char));
    recv(dSclientFiles[n],filename,taille*sizeof(char),0); 
    printf("Downloading %s\n",filename);
    ///
    FILE *fp;   
    char* folder="ServerStorage/";
    char* path=(char*)malloc((strlen(filename)+strlen(folder))*sizeof(char));
    sprintf(path,"ServerStorage/%s",filename);
    fp=fopen(path,"w+");
    char buffer[SIZE];
    long filesize;
    recv(dSclientFiles[n],&filesize,sizeof(long),0);
    printf("Size : %ld\n",filesize);
    int cpt;
        for(int i=0;i<filesize;i+=SIZE){
            if(i+SIZE<filesize){
                cpt=SIZE;
                }
            else{
                cpt=filesize-i;
            }
            recv(dSclientFiles[n],buffer,sizeof(buffer),0);
            fwrite(buffer, sizeof(buffer),1, fp);
            bzero(buffer,SIZE);
        }
        fclose(fp);
        printf("File downloaded\n");
        pthread_exit(NULL);
}


void * sending_file(void* numcliact){   
    int* numCliAct=numcliact;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    int FileNumber;
    recv(dSClient[n],&FileNumber,sizeof(int),0);
    struct sockaddr_in aCF;
    socklen_t lgF = sizeof(struct sockaddr_in);
    dSclientFiles[n]=accept(dSF,(struct sockaddr *)&aCF,&lgF);
    char* name = getfilename(FileNumber);
    int tailleF=(strlen(name)+1)*sizeof(char);
    send(dSclientFiles[n],&tailleF,sizeof(int),0);
    send(dSclientFiles[n],name,(strlen(name)+1)*sizeof(char),0);
    FILE *fp;
    char* folder="ServerStorage/";
    char* path=(char*)malloc((strlen(name)+strlen(folder))*sizeof(char));
    sprintf(path,"ServerStorage/%s",name);
    fp = fopen(path,"rb");
    if(fp==NULL){
        printf("fichier vide\n");
        shutdown(dSF,SHUT_RDWR);
        pthread_exit(NULL);
  } 
    fseek(fp,0,SEEK_END);
    long filesize= ftell(fp);
    rewind(fp);
    char buffer[SIZE];
    int cpt;
    send(dSclientFiles[n],&filesize,sizeof(long),0);
    for(int i=0;i<filesize;i+=SIZE){
        if(i+SIZE<filesize){
            cpt=SIZE;
        }
        else{
            cpt=filesize-i;
            }
        fread(buffer,cpt,1,fp); 
        send(dSclientFiles[n],buffer,sizeof(buffer),0);
        bzero(buffer,SIZE);
    }
    printf("File sended \n");
    fclose(fp);
    free(path);
    pthread_exit(NULL); 
}


int GetFileNum(char* msg){
    char* Fnum;
    int testnum=msg[5];
    Fnum=strtok(msg,":");
    Fnum=strtok(NULL,":");
    int filenumbers=ListServerFiles();
    int FileNumber=atoi(Fnum);
    if(FileNumber>0&&FileNumber<=filenumbers){
        return FileNumber;
    }
    else{
        return 0;
    } 
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
        //char tosend[TAILLE];
        char* tosend=malloc(TAILLE);
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

            if(commande==3){ // Si c'est une commande man 
                taille=nbcommand*18;
                char man[taille];
                // char com[18];
                 char* com=malloc(taille);
                for(int i=0;i<nbcommand;i++){
                   // sprintf(man,"%s::\n",availablescommands[i]);
                    strcpy(com,availablescommands[i]);
                    strcat(com,":[optional]:\n");
                    strcat(man,com);
                    // strcat(src,availablescommands[i]);
                }
                send(dSClient[n], &taille, sizeof(int), 0) ;
                send(dSClient[n],man, strlen(man)+1, 0) ;
            }

            if(commande==4){ // Si c'est une commande ls
                printf("Ready to receive a file from : %s\n",names[n]);
                pthread_create(&files_receive[n],NULL,receive_file,&n);
                pthread_join(files_receive[n],NULL);
            }

            if(commande==5){
                ListServerFiles();
                taille=strlen(FilesAvailables);    
                send(dSClient[n], &taille, sizeof(int), 0) ;
                send(dSClient[n],FilesAvailables, strlen(FilesAvailables)+1, 0) ;
            }

            if(commande==6){
                int IsValid=GetFileNum(msg);
                printf("FIN DE GETFILENUM ; %d\n",IsValid);
                if(IsValid==0){
                    sprintf(tosend,"This is not a valid file number");
                    taille=strlen(tosend)+1;
                    send(dSClient[n], &taille, sizeof(int), 0) ;
                    send(dSClient[n], tosend, strlen(tosend)+1, 0) ;
                    
                }
                else{
                    sprintf(tosend,KEY);
                    taille=strlen(tosend)+1;
                    send(dSClient[n], &taille, sizeof(int), 0) ;
                    send(dSClient[n], tosend, strlen(tosend)+1, 0) ;
                    send(dSClient[n],&IsValid,sizeof(int),0);
                    pthread_create(&files_send[n],NULL,sending_file,&n);    
                    pthread_join(files_send[n],NULL);

                }
            }
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
    ////
    dSF = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adF;
    adF.sin_family = AF_INET;
    adF.sin_addr.s_addr = INADDR_ANY ;
    adF.sin_port = htons(PORTforFILE) ;
    if (bind(dSF, (struct sockaddr*)&adF, sizeof(adF)) == -1) {
        perror("erreur nommer socket");
    };
    listen(dSF, 7) ;
   




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

