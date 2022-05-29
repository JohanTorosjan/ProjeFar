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

void printrouge(char * msg){
    printf("\033[31m" );
    printf("%s",msg);
    printf("\033[37m" );
}

void printbleu(char * msg){
    printf("\033[34m" );
    printf("%s",msg);
    printf("\033[37m" );
}

void printvert(char * msg){
    printf("\033[32m" );
    printf("%s",msg);
    printf("\033[37m" );
}

void printorange(char * msg){
    printf("\033[33m" );
    printf("%s",msg);
    printf("\033[37m" );
}
void printcyanfile(char* msg, int num) {
    printf("\033[36m" );
    printf("%d : %s\n",num,msg);
    printf("\033[37m" );
}

void printcyan(char* msg) {
    printf("\033[36m" );
    printf("%s",msg);
    printf("\033[37m" );
}

void printcyanint(int n){
    printf("\033[36m" );
    printf("%d",n);
    printf("\033[37m" );
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
       // printf("\n");
        fgets(message,TAILLE,stdin);
        int taille=(strlen(message)+1)*sizeof(char);
        if(send(dS,&taille,sizeof(int),0)==0){quit();}
        send(dS,message,(strlen(message)+1)*sizeof(char),0);
        printf("\n");
        printf("\n");
    }
}


void *ReceivingFile(void* num){
    int* Num=num;
    int n= *Num; // Recup le numéro associé à chaque clients
    send(dS,&n,sizeof(int),0);
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
    int taille;
    recv(dsF, &taille,sizeof(int), 0);
    char filename[taille];
    recv(dsF, filename, taille, 0); 
    char* folder="FichierTelecharge/";
    char path[(strlen(filename)+strlen(folder))*sizeof(char)];
    sprintf(path,"FichierTelecharge/%s",filename);
    FILE *fp;   
    fp=fopen(path,"w+");
    char buffer[SIZE];
    long filesize;
    recv(dsF,&filesize,sizeof(long),0);
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
    printvert("File downloaded\n");
    printf("\n");
    shutdown(dsF,SHUT_RDWR);
    pthread_exit(NULL);
}

int listfiles(int c){
    if (c == 1) {
        printf("\n");
        printcyan("----------------------\n");
    }
    DIR *d = opendir("./FilesToUpload");
    struct dirent *dir;
    int cpt=1;
    if (d)
    {
        while ((dir = readdir(d)) != NULL){

            if(dir->d_type==8){
                if (c == 1) {
                    printcyanfile(dir->d_name,cpt);
                }  
                cpt++;
            }
        }
    closedir(d);
    }
    if (c == 1) {
        printcyan("----------------------\n");
        printcyan("Utilisez la commande @snd:filenumber: pour envoyer un fichier au serveur\n");
    }
    printf("\n");
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
    printvert("File sended \n");
    printf("\n");
    fclose(fp);

    shutdown(dsF,SHUT_RDWR);
    pthread_exit(NULL);
}



void *rcvMessage(void *ds) {

    int taillemessage;
    recv(dS,&taillemessage,sizeof(int),0); 
    char buffer[taillemessage];
    recv(dS,buffer,taillemessage*sizeof(char),0);
    printf("\n");
    printorange(buffer);
    printf("\n");
    while(1){
        int taille;
        if(recv(dS, &taille, sizeof(int), 0)==0){quit();}
        
        if(taille<200){
            char *msg = (char*)malloc(taille*sizeof(char));
            recv(dS, msg, taille, 0);
            if (msg[0] == 'A') {
                printrouge(msg);
                printf("\n");
            }
            else if(msg[0]=='[') {
                char msgcpy[taille];
                strcpy(msgcpy,msg);
                char * cpy;
                cpy = strtok(msgcpy," ");
                printbleu(msgcpy); // print channel en bleu
                cpy=strtok(NULL," ");
                printorange(cpy); // print pseudo en orange
                cpy=strtok(NULL,"\0");
                puts(cpy);
                printf("\n");
            }

            else{
                printrouge(msg);
                printf("\n");
                printf("\n");
            }
        }

        else if (taille == 1000){
            recv(dS,buffer,taillemessage*sizeof(char),0);
            printf("\n");
            printorange(buffer);
            printf("\n");
        }

        else if (taille==1001){
            printrouge("Déconnexion...\n");
            printf("\n");
            printf("\n");
            quit();
            exit(0);
        }
         else if (taille==1002){
            printrouge("Déconnexion du channel...\n");
            printf("\n");
            printf("\n");
            break;
            
        }
        else if (taille == 1005){
            int taillefichier;
            recv(dS,&taillefichier,sizeof(int),0); 
            char bufferfichier[taillefichier];
            recv(dS,bufferfichier,taillefichier*sizeof(char),0);
            printcyan(bufferfichier);
            printf("\n");
        }

        else if(taille == 10060){
            printrouge("Fichier non valide\n");
            printf("\n");
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
                printrouge("Fichier non valide\n");
            }

            if(taille == 10081) {
                int numfichier;
                recv(dS,&numfichier, sizeof(int), 0);
                pthread_create(&thread_ReceivingFile,NULL,SendingFile,&numfichier);
                pthread_join(thread_ReceivingFile,NULL);
            }
        }

        else if (taille == 1009){
            int estadmin;
            recv(dS,&estadmin, sizeof(int), 0);
            if(estadmin==1){
                printvert("Vous êtes bien connecté en tant qu'Admin, faites @man: pour voir les commandes\n");
                printf("\n");
            }
            else{
                printrouge("Mauvais mot de passe admin\n");
                printf("\n");
            }
        }

        else if (taille == 1010){
            int kickvalide;
            recv(dS,&kickvalide, sizeof(int), 0);
            if(kickvalide==-2){
                printrouge("Impossible de kicker un autre administrateur\n");
                printf("\n");
            }
            else if (kickvalide ==-1){
                printrouge("Ce pseudo n'existe pas\n");
                printf("\n");
            }

        }

        else if (taille == 10111){
            int channelok=0;
            pthread_cancel(thread_envoi);
            while(channelok!=1){


                char nomchannel[TAILLE];
                printf("\nEntrez un nom pour le channel : ");
                fgets(nomchannel,TAILLE,stdin);
                int taillenomchannel=(strlen(nomchannel)+1)*sizeof(char);
                if(send(dS,&taillenomchannel,sizeof(int),0)==0){quit();}
                send(dS,nomchannel,(strlen(nomchannel)+1)*sizeof(char),0);


                
                char descriptionchannel[TAILLE];
                printf("\nEntrez une description pour le channel : ");
                fgets(descriptionchannel,TAILLE,stdin);
                int tailledescriptionchannel=(strlen(descriptionchannel)+1)*sizeof(char);
                if(send(dS,&tailledescriptionchannel,sizeof(int),0)==0){quit();}
                send(dS,descriptionchannel,(strlen(descriptionchannel)+1)*sizeof(char),0);


                recv(dS,&channelok, sizeof(int), 0);

                if(channelok==0){
                    printrouge("Veuillez entrez un nom de channel non existant\n");
                }
                else if(channelok==-1){
                    printrouge("Veuillez ne pas laisser de champs vide\n");
                }
            }
            pthread_create(&thread_envoi,NULL,sendMessage,NULL);
            printf("\n");
            printvert("Channel Crée\n");
            printf("\n");
        }

        else if(taille==10110){
            printrouge("Nombre maximum de channel atteint\n");
        }

        else if( taille==1012){
            int nbcl;
            recv(dS,&nbcl, sizeof(int), 0);
            printcyan("Nombre de client connectés: ");
            printcyanint(nbcl);
            printf("\n");
            int tailleclients;
            recv(dS,&tailleclients,sizeof(int),0); 
            char bufferclients[tailleclients];
            recv(dS,bufferclients,(tailleclients)*sizeof(char),0);
            printcyan(bufferclients);
            printf("\n");
            
        }
        //-------------------- SI LE CLIENT TENTE UNE COMMANDE ADMIN  ---------------------//


        else if( taille==20000){

            printrouge("Vous n'avez pas les droits d'administrateur, tentez @adm:PASSWORD: pour vous connecter\n");
            printf("\n");
        }
        //-------------------- SI LE CLIENT SE FAIT KICK ---------------------//

        else if (taille == 20021){
            recv(dS, &taille, sizeof(int), 0);
            char *msg = (char*)malloc(taille*sizeof(char));
            recv(dS, msg, taille, 0);
            printrouge("Vous vous êtes fait kicker par ");
            printrouge(msg);
            printf("\n");
            quit();
            exit(0);
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
    printf("\n");
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
            printrouge("Ce pseudo est invalide ou déja utilisé \n");
            printf("\n");
        }
        else{
            printf("\n");
            printvert("#####################################\n");
            printvert("Bienvenue ");
            printvert(pseudo);
            printvert("#####################################\n");
            printf("\n");
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
        printrouge("Le serveur est plein pour l'instant \n");
        quit();
    }

    ////////////////////////////// Message Bienvenu /////////////////////////////

    int taillemessage;
    recv(dS,&taillemessage,sizeof(int),0); 
    char buffer[taillemessage];
    recv(dS,buffer,taillemessage*sizeof(char),0);
    printf("\n \n \n");
    //printf("%s\n",buffer);
    printvert(buffer);
    printf("\n");

    ////////////////////////////// Liste Channel /////////////////////////////

    int nbchannels;
    recv(dS,&nbchannels,sizeof(int),0);
    int taillemessage2;
    taillemessage2 -= 2;
    recv(dS,&taillemessage2,sizeof(int),0); 
    char buffer3[taillemessage2];
    recv(dS,buffer3,(taillemessage2)*sizeof(char),0);
    printvert(buffer3);
    printf("\n");
    printf("Rentrez le buméro d'un channel ou quit pour quitter\n#");
    ////////////////////////////// Choix Channel /////////////////////////////
    char choiceChannel[TAILLE];
    fgets(choiceChannel,TAILLE,stdin);
    printf("\n");
    int taillechoice=(strlen(choiceChannel)+1)*sizeof(char);
    
    while (strcmp(choiceChannel,"quit\n")!= 0) {
        int numchoice=atoi(choiceChannel);
        if ((0<numchoice) && (numchoice<=nbchannels)) {
            send(dS,&numchoice,sizeof(int),0);
            int etatchannel;
            recv(dS,&etatchannel,sizeof(int),0);
            if (etatchannel == 1) {
                sendpseudo(dS);
                printvert("Création des thread.. \n");
                        
                pthread_create(&thread_envoi,NULL,sendMessage,NULL);
                pthread_create(&thread_reception,NULL,rcvMessage,NULL);

                pthread_join(thread_reception,NULL);
                pthread_cancel(thread_envoi); 
                ////////////////////////////// Liste Channel /////////////////////////////

                recv(dS,&nbchannels,sizeof(int),0);
                taillemessage2 -= 2;
                recv(dS,&taillemessage2,sizeof(int),0); 
                char buffer3[taillemessage2];
                recv(dS,buffer3,(taillemessage2)*sizeof(char),0);
                printvert(buffer3);
                printf("\n");
                printf("Rentrez le buméro d'un channel ou quit pour quitter\n#");
                fgets(choiceChannel,TAILLE,stdin);
                printf("\n");

            }
            else {
                printrouge("Channel plein, veuillez rentrer un channel différent\n");
                printf("#");
                fgets(choiceChannel,TAILLE,stdin);
            }
        }
        else {
            printf("\n");
            printvert(buffer3);
            printf("\n");
            printrouge("Rentrez le buméro VALIDE d'un channel ou quit pour quitter\n");
            printf("#");
            fgets(choiceChannel,TAILLE,stdin);
        }

    }
    quit();
    
}