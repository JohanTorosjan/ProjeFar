#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include<arpa/inet.h>
#include<unistd.h>

#define PORT 2637
#define IP "127.0.0.1"
#define TAILLE 200
#define PortForFile 2638
#define SIZE 1024


// Variables globales
pthread_t thread_reception;
pthread_t thread_envoi;

pthread_t thread_ReceivingFile;
pthread_t thread_SendingFile;


int dS;
int dsF;


void quit(){
    shutdown(dS,SHUT_RDWR);
    exit(0);
}

void rcvFileText() {
    int taillemessage;
    recv(dS,&taillemessage,sizeof(int),0); 
    char * buffer=malloc(sizeof(char)*taillemessage);
    recv(dS,buffer,taillemessage*sizeof(char),0);
    printf("%s\n",buffer);
    free(buffer);
}

char* getfilename(int n) {
    char* filename;
    DIR *d = opendir("./FilesToUpload");
    struct dirent *dir;
    int cpt=1;
    if (d) {
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

void *sendMessage() {
    char message[TAILLE];
    while(1){
        printf("\n");
        fgets(message,TAILLE,stdin);
        int taille=(strlen(message)+1)*sizeof(char);
        if(send(dS,&taille,sizeof(int),0)==0){quit();}
        send(dS,message,(strlen(message)+1)*sizeof(char),0);
    }

    /*
  char message[TAILLE];
  pthread_t thread_SendingFile;
  while(1){
    printf("\n");
    //pthread_join(thread_ReceivingFile,NULL); Mettre ou pas ? 
    fgets(message,TAILLE,stdin);
    int taille=(strlen(message)+1)*sizeof(char);
    if(send(dS,&taille,sizeof(int),0)==0){quit();}
    send(dS,message,(strlen(message)+1)*sizeof(char),0);
    if(strcmp(message,"@ls::\n")==0 || strcmp(message,"@ls:\n")==0){
      while(1){
        int nbfiles=listfiles();
        char choice[TAILLE];
        fgets(choice,TAILLE,stdin);
        if(strcmp(choice,"quit\n")!=0){ // Si il ecrit pas quit
          int num=atoi(choice);
          if(num>0 && num<nbfiles){
            pthread_create(&thread_SendingFile,NULL,SendingFile,&num);
            pthread_join(thread_SendingFile,NULL);
            break;
          }
        }
        else{
          int quit=-1;
          pthread_create(&thread_SendingFile,NULL,SendingFile,&quit);
          pthread_join(thread_SendingFile,NULL);
          break;
        }
      }
    }
    if(strcmp(message,fin)==0 || strcmp(message,"@end::\n")==0){
        printf("#################\n");
        exit(0);
        }
    }*/
}


void *ReceivingFile(void* num){
    int* Num=num;
    int n= *Num; // Recup le numéro associé à chaque clients
    send(dS,&n,sizeof(int),0);
    printf("Thread crée et num envoyé\n");
    dsF = socket(PF_INET, SOCK_STREAM, 0);
    if (dsF == -1) {
         perror("erreur dans l'initialisation du socket");
    }
    struct sockaddr_in aSF;
    aSF.sin_family = AF_INET;
    inet_pton(AF_INET,IP,&(aSF.sin_addr)) ;
    aSF.sin_port = htons(PortForFile);
    socklen_t lgAF = sizeof(struct sockaddr_in) ;
    if (connect(dsF, (struct sockaddr *) &aSF, lgAF) == -1 ) {
        perror("erreur dans la connection du socket");
        exit(0);
    }
    printf("Ok\n");
    int taille;
    recv(dsF, &taille,sizeof(int), 0);
    char filename[taille];
    recv(dsF, filename, taille, 0); 
    printf("nom : %s\n",filename);
    char* folder="FichierTelecharge/";
    char path[(strlen(filename)+strlen(folder))*sizeof(char)];
    sprintf(path,"FichierTelecharge/%s",filename);
    puts(path);
    FILE *fp;   
    fp=fopen(path,"w+");
    char buffer[SIZE];
    long filesize;
    recv(dsF,&filesize,sizeof(long),0);
    printf("FILE SIZE : %ld\n",filesize);
    int cpt;
      for(int i=0;i<filesize;i+=SIZE){
        if(i+SIZE<filesize){
             cpt=SIZE;
            }
        else{
             cpt=filesize-i;
        }
        recv(dsF,buffer,sizeof(buffer),0);
        fwrite(buffer, sizeof(buffer),1, fp);
        bzero(buffer,SIZE);
    }
    fclose(fp);
    printf("File downloaded\n");
    shutdown(dsF,SHUT_RDWR);
    pthread_exit(NULL);
}

int listfiles(int c){
    if (c == 1) {
        printf("\n");
        printf("----------------------\n");
    }
    DIR *d = opendir("./FilesToUpload");
    struct dirent *dir;
    int cpt=1;
    if (d)
    {
        while ((dir = readdir(d)) != NULL){

            if(dir->d_type==8){
                if (c == 1) {
                    printf("%d : %s\n",cpt, dir->d_name);
                }  
                cpt++;
            }
        }
    closedir(d);
    }
    if (c == 1) {
        printf("----------------------\n");
        printf("@snd:NUM FILE:\n");
    }
    return cpt;
}

void *SendingFile(void *num){
    int* Num=num;
    int n= *Num; // Recup le numéro associé à chaque clients
    char* name = getfilename(n);
    FILE *fp;
    dsF = socket(PF_INET, SOCK_STREAM, 0);
    if (dsF == -1) {
        perror("erreur dans l'initialisation du socket");
    }
    struct sockaddr_in aSF;
    aSF.sin_family = AF_INET;
    inet_pton(AF_INET,IP,&(aSF.sin_addr)) ;
    aSF.sin_port = htons(PortForFile);
    socklen_t lgAF = sizeof(struct sockaddr_in) ;
    if (connect(dsF, (struct sockaddr *) &aSF, lgAF) == -1 ) {
        perror("erreur dans la connection du socket");
        exit(0);
    }
    int tailleF=(strlen(name)+1)*sizeof(char);
    if(send(dsF,&tailleF,sizeof(int),0)==0){quit();}
    send(dsF,name,(strlen(name)+1)*sizeof(char),0);
    char* folder="FilesToUpload/";
    char path[strlen(name)+strlen(folder)];
    sprintf(path,"FilesToUpload/%s",name);
    fp = fopen(path,"rb");
    if(fp==NULL){
        printf("fichier vide\n");
        shutdown(dsF,SHUT_RDWR);
        pthread_exit(NULL);
    }

    fseek(fp,0,SEEK_END);
    long filesize= ftell(fp);
    rewind(fp);
    char buffer[SIZE];
    int cpt;
    send(dsF,&filesize,sizeof(long),0);
    for(int i=0;i<filesize;i+=SIZE){
        if(i+SIZE<filesize){
        cpt=SIZE;
        }
        else{
        cpt=filesize-i;
        }
        fread(buffer,cpt,1,fp);
        send(dsF,buffer,sizeof(buffer),0);

        bzero(buffer,SIZE);
    }
    printf("File sended \n");
    fclose(fp);
    shutdown(dsF,SHUT_RDWR);
    pthread_exit(NULL);
}



void *rcvMessage(void *ds) {

    int taillemessage;
    recv(dS,&taillemessage,sizeof(int),0); 
    char buffer[taillemessage];
    recv(dS,buffer,taillemessage*sizeof(char),0);
    printf("%s\n",buffer);
    while(1){
        int taille;
        if(recv(dS, &taille, sizeof(int), 0)==0){quit();}
        
        if(taille<200){
            char *msg = (char*)malloc(taille*sizeof(char));
            recv(dS, msg, taille, 0);
            puts(msg);
        }

        else if (taille == 1000){
            recv(dS,buffer,taillemessage*sizeof(char),0);
            printf("%s\n",buffer);
        }

        else if (taille==1001){
            printf("Déconnexion...\n");
            quit();
            exit(0);
        }
         else if (taille==1002){
            printf("Déconnexion du channel...\n");
            break;
            
        }
        else if (taille == 1005){
            int taillefichier;
            recv(dS,&taillefichier,sizeof(int),0); 
            char bufferfichier[taillefichier];
            recv(dS,bufferfichier,taillefichier*sizeof(char),0);
            puts(bufferfichier);

        }

        else if(taille == 10060){
            printf("Fichier non valide\n");
        }

        else if(taille == 10061){
            int n;
            recv(dS,&n, sizeof(int), 0);
            pthread_create(&thread_ReceivingFile,NULL,ReceivingFile,&n);
            pthread_join(thread_ReceivingFile,NULL);
        }

        else if(taille == 1007){
            listfiles(1);  
        }

        else if (taille == 1008){
            int nbrfile = listfiles(0)-1;
            send(dS,&nbrfile,sizeof(int),0);
            recv(dS,&taille, sizeof(int), 0);
            if(taille == 10080) {
                printf("Fichier non valide\n");
            }

            if(taille == 10081) {
                int numfichier;
                recv(dS,&numfichier, sizeof(int), 0);
                pthread_create(&thread_ReceivingFile,NULL,SendingFile,&numfichier);
                pthread_join(thread_ReceivingFile,NULL);
            }
        }

        

        /*
        if(strcmp(msg,KEY)==0){
        int n;
        recv(dS,&n, sizeof(int), 0);
        pthread_create(&thread_ReceivingFile,NULL,ReceivingFile,&n);
        pthread_join(thread_ReceivingFile,NULL);
        }
        
        else{
        puts(msg);
        }
        */
    }
    pthread_exit(NULL);
}


void sendpseudo(int dS){
    while(1){
        char pseudo[TAILLE];
        printf("Entrez un pseudo : ");
        fgets(pseudo,TAILLE,stdin);
        int taillepseudo=strlen(pseudo);
        send(dS,&taillepseudo,sizeof(int),0);
        send(dS,pseudo,strlen(pseudo)*sizeof(char),0);
        int pseudok;
        recv(dS,&pseudok,sizeof(int),0);
        if(pseudok==0){
            printf("Ce pseudo est invalide ou déja utilisé \n");
        }
        else{
            printf("Bienvenue %s",pseudo);
            return;
        }
    }
}

int main(int argc, char *argv[]) {
      /////////////////// CONNECTION //////////////////////////////
    dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("erreur dans l'initialisation du socket");
    }
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,IP,&(aS.sin_addr)) ;
    aS.sin_port = htons(PORT) ;
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    if (connect(dS, (struct sockaddr *) &aS, lgA) == -1 ) {
        perror("erreur dans la connection du socket");
        exit(0);
    }
  ///////////////////////////////////////////////////////////
    int full;
    recv(dS, &full, sizeof(int), 0);
    if(full==1){
        printf("Le serveur est plein pour l'instant \n");
        quit();
    }

    int taillemessage;
    recv(dS,&taillemessage,sizeof(int),0); 
    char buffer[taillemessage];
    recv(dS,buffer,taillemessage*sizeof(char),0);
    printf("%s\n",buffer);

    int nbchannels;
    recv(dS,&nbchannels,sizeof(int),0);
    
    
    int taillemessage2;
    taillemessage2 -= 2;
    recv(dS,&taillemessage2,sizeof(int),0); 
    char buffer3[taillemessage2];
    recv(dS,buffer3,(taillemessage2)*sizeof(char),0);
    printf("%s\n",buffer3);
    

    printf("Rentrez le buméro d'un channel ou quit pour quitter\n#");
    char choiceChannel[TAILLE];
    fgets(choiceChannel,TAILLE,stdin);
    int taillechoice=(strlen(choiceChannel)+1)*sizeof(char);
    while (strcmp(choiceChannel,"quit\n")!= 0) {
        int numchoice=atoi(choiceChannel);
        if ((0<numchoice) && (numchoice<=nbchannels)) {
            printf("num channel choisi : %d\n",numchoice);
            send(dS,&numchoice,sizeof(int),0);
            int etatchannel;
            recv(dS,&etatchannel,sizeof(int),0);
            if (etatchannel == 1) {
                sendpseudo(dS);
                printf("Création des thread.. \n");
                
                
                
                pthread_create(&thread_envoi,NULL,sendMessage,NULL);
                pthread_create(&thread_reception,NULL,rcvMessage,NULL);

                pthread_join(thread_reception,NULL);
                pthread_cancel(thread_envoi); 
                printf("\n%s\n",buffer3);
                printf("Rentrez un numéro d'un channel ou quit pour quitter\n#");
                fgets(choiceChannel,TAILLE,stdin);

            }
            else {
                printf("Channel plein, veuillez rentrer un channel différent\n#");
                fgets(choiceChannel,TAILLE,stdin);
            }
        }
        else {
            printf("\n%s\n",buffer3);
            printf("Rentrez le buméro VALIDE d'un channel ou quit pour quitter\n#");
            fgets(choiceChannel,TAILLE,stdin);
        }

    }
    quit();
    
}