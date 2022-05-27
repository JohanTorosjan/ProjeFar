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

//

// DEFINE 
#define NBCLIENTCHANNEL 50
#define NBCHANNEL 20
#define PORT 2637
#define PortForFile 2638
#define SIZE 1024

// Variables globales

int nbclientsco;
int dsF;

pthread_t ThreadClientsCo[NBCHANNEL*NBCLIENTCHANNEL]; // Tableau des thread pour chaques clients
pthread_t ThreadCommunication[NBCHANNEL*NBCLIENTCHANNEL];
pthread_t files_receive[NBCHANNEL*NBCLIENTCHANNEL];
pthread_t files_send[NBCHANNEL*NBCLIENTCHANNEL];
pthread_mutex_t mutex;
////////////////////////////////////////////////////

typedef struct Channel Channel; // Structure pour les channels
struct Channel
{
    int num;
    int clientsco;
    char* nom;
    char* description;
};

typedef struct Client Client; // Structure pour les channels
struct Client
{
    int num;
    int dS;
    char* pseudo;
    int channel;
    int isAdmin;
    int isConnected;
    int dsF;
};

Client allclients[NBCHANNEL*NBCLIENTCHANNEL]; // Tableau de tout les clients 2

Channel allchannels[NBCHANNEL]; // Tableau des channels qui restent 


/////////////////////////////////////////////////////

void innitialiseclients() {
    Client client = {0,0,"",-1,0,0,0};
    for (int i = 0; i < NBCLIENTCHANNEL*NBCHANNEL; i++) {
        client.num = i;
        allclients[i] = client;
    }   
}

void innitialisechannels(){
    Channel channel1 ={0,0,"Général","Discussion générals"};
    Channel channel2 ={1,0,"Devoirs","Discussion sur les devoirs"};
    Channel channel3 ={2,0,"Loisirs","Discussion sur le loisirs"};
    Channel channel4 ={3,0,"Fun","Pour faire des trucs funs"};
    Channel channelvide ={-1,0," "," "};
    allchannels[0]=channel1;
    allchannels[1]=channel2;
    allchannels[2]=channel3;
    allchannels[3]=channel4;
    for(int i=4;i<NBCHANNEL;i++){
        allchannels[i]=channelvide;
    }
}

int getnbchannels(){
    int cpt=0;
    for(int i=0;i<NBCHANNEL;i++){
        if(allchannels[i].num!=-1){
            cpt++;
        }
    }
    return cpt;
}

void getnbclient(){
    int nbc = 0;
    for (int i = 0; i < NBCLIENTCHANNEL*NBCHANNEL; i++) {
        if (allclients[i].isConnected == 1) {
            nbc += 1;
        }
    }
    nbclientsco = nbc;
}

void decoclient(int numclient) {
    Client client = {numclient,0,"",-1,0,0,0};
    allclients[numclient] = client;
    pthread_mutex_lock(&mutex);
    nbclientsco --;
    pthread_mutex_unlock(&mutex);
}

int getclientdispo(){
    int nbc = 0;
    for (int i = 0; i < NBCLIENTCHANNEL*NBCHANNEL; i++) {
        if (allclients[i].isConnected == 0) {
            return(i);
        }
    }
    return(-1);
}

int getfilesize(char* path){
    FILE *f;
    char c;
    f=fopen(path,"rt");
    int cpt=0;
    while((c=fgetc(f))!=EOF){
       cpt++;
    }
    fclose(f);
    return cpt;
}

char* getbuffer(char* path){
    FILE *fp;
    char c;
    fp=fopen(path,"rt");
    int taillefichier=getfilesize(path);
    char* buffer=malloc(taillefichier*sizeof(char));
    int i=0;
    while((c=fgetc(fp))!=EOF){
       buffer[i]=c;
       i++;
    }
    fclose(fp);
    return buffer;
}

void writechannels(char* path){
    FILE *fpc;
    fpc=fopen(path,"w+");
    for(int i=0;i<NBCHANNEL;i++){
        if(allchannels[i].num!=-1){
            char channels[strlen(allchannels[i].nom)+strlen(allchannels[i].description)];
            sprintf(channels,"#%d %s : %s (%d/%d)\n",i+1,allchannels[i].nom,allchannels[i].description,allchannels[i].clientsco,NBCLIENTCHANNEL);
            fwrite(channels,strlen(channels)*sizeof(char),1,fpc);
        }  
    }
    
    fclose(fpc);
}

int listefiles(char * path,int writefile){
    DIR *d = opendir("./ServerStorage");
    struct dirent *dir;
    int cpt=0;
    if(d){
        while ((dir = readdir(d)) != NULL){
        if(dir->d_type==8){
            cpt++;
        }
    }

    if(writefile==1){
        FILE *fpc;
        fpc=fopen(path,"w+");
        printf("cpt : %d\n",cpt);
        d = opendir("./ServerStorage");
        int nbrfichier=cpt;
        cpt=1;
        while ((dir = readdir(d)) != NULL){
            if(dir->d_type==8){
                printf("cpt : %d\n",cpt);
                printf("nbr : %d\n",nbrfichier);
                
                if(nbrfichier!=cpt){
                    char namefile[strlen(dir->d_name+4)];
                    sprintf(namefile,"%d : %s\n",cpt,dir->d_name);
                    fwrite(namefile,strlen(namefile)*sizeof(char),1,fpc);
                }
                else{
                    char namefile[strlen(dir->d_name+3)];
                    sprintf(namefile,"%d : %s",cpt,dir->d_name);
                    fwrite(namefile,strlen(namefile)*sizeof(char),1,fpc);
                }
                cpt++;
                }
            }
            fwrite("\nUtilisez @get:numéro_du_fichier: pour télécharger un fichier\n",66*sizeof(char),1,fpc);
            fclose(fpc);
        }
    }
    return cpt;
}

/// command : char* msg ----> Int 
/// -1 si le message n'est pas une commande 
/// 0 si la commmande n'est pas valable 
// i+1 correspondant à l'indice +1 dans de le tableau des commandes de la commande si elle existe 
int command(char* msg){
    char allCommand[10][10] = {""};
    strcpy(allCommand[0], "@man");
    strcpy(allCommand[1], "@end");
    strcpy(allCommand[2], "@lc");
    strcpy(allCommand[3], "@mp");
    strcpy(allCommand[4], "@lss");
    strcpy(allCommand[5], "@get");
    strcpy(allCommand[6], "@ls");
    strcpy(allCommand[7], "@snd");
    if(msg[0]=='@'){
        char* msgcopy=malloc(strlen(msg)+1);
        strcpy(msgcopy,msg);
        char* token=strtok(msgcopy,":");
        printf("avant if\n");
        int c = 0;
        printf("token : %s\n",token);
        for(int i=0;i<8;i++){
            printf("%s\n",allCommand[i]);
            if(strcmp(token,allCommand[i])==0){
                printf("i : %d\n",i);
                c = i+1;
            }     
        }

        free(msgcopy);
        return c;
    }
    else{
        return -1;
    }
}
//// SendPrivateMessage : char* msg, int n ---> Int
/// -1 Si le pseudo n'existe pas 
/// i qui correspond à l'indice dans le tableau des noms  du pseudo correspondant 
int SendPrivateMessage(char* msg,int n){
    getnbclient();
    char* pseudo;
    char copy[strlen(msg)+1];
    strcpy(copy,msg);
    char * msgcopy;
    msgcopy=strtok(copy,":");
    msgcopy=strtok(NULL,":");
    msgcopy=strtok(NULL,"\0");
    pseudo=strtok(msg,":");
    pseudo=strtok(NULL,":");
    for(int i=0;i<nbclientsco;i++){
        if(strcmp(allclients[i].pseudo,pseudo)==0){
            printf("Private message from %s for %s: %s\n",allclients[n].pseudo,allclients[i].pseudo,msgcopy);
            int taille=strlen(msgcopy)+strlen(allclients[n].pseudo)+20; 
            char tosend[taille];
            sprintf(tosend,"---\n[MP]%s-> %s---\n", allclients[n].pseudo,msgcopy);
            send(allclients[i].dS,&taille, sizeof(int), 0) ; 
            send(allclients[i].dS, tosend, strlen(tosend)+1, 0) ;   
            return i;
        } 
    }
    return -1;
}
void setpseudo(int n){
    pthread_mutex_lock(&mutex);
    int taillepseudo;
    int pseudok=0;
    while(pseudok==0){
        int rec = recv(allclients[n].dS,&taillepseudo,sizeof(int),0);
            /////////////////////// Deconnexion ///////////////////////
        if(rec == 1){
                decoclient(n);
                printf("client deco\n");
                pthread_exit(NULL);
        }

        char pseudo[taillepseudo];
        recv(allclients[n].dS,pseudo,sizeof(char)*taillepseudo,0);
        char *pos;
        pos=strchr(pseudo,'\n');
        *pos='\0';
        pseudok=1;
        for (int i = 0; i < NBCLIENTCHANNEL*NBCHANNEL; i++) {
            if( (strcmp(allclients[i].pseudo,pseudo)==0) || (strlen(pseudo)<2) || (pseudo[0]=='@') )
            {
                pseudok=0;
            }
        }
        send(allclients[n].dS,&pseudok,sizeof(int),0);
        if(pseudok==1){
            allclients[n].pseudo=pseudo;
        }
    }
     pthread_mutex_unlock(&mutex);
}


int GetFileNum(char* msg, int filenumbers){
    char* Fnum;
    int testnum=msg[5];
    Fnum=strtok(msg,":");
    Fnum=strtok(NULL,":");
    int FileNumber=atoi(Fnum);
    if(FileNumber>0&&FileNumber<=filenumbers){
        return FileNumber;
    }
    else{
        return 0;
    } 
}
char* getfilename(int n){
    char* filename;
    DIR *d = opendir("./ServerStorage");
    struct dirent *dir;
    int cpt=1;
    if (d){
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

void * sending_file(void* numcliact){   

    int* numCliAct=numcliact;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    int FileNumber;
    recv(allclients[n].dS,&FileNumber,sizeof(int),0);
    printf("Création thread avec fichier voulu : %d\n",FileNumber);
    struct sockaddr_in aCF;
    socklen_t lgF = sizeof(struct sockaddr_in);
    allclients[n].dsF=accept(dsF,(struct sockaddr *)&aCF,&lgF);
    char* name = getfilename(FileNumber); // a free 
    printf("nom : %s\n",name);
    int tailleF=(strlen(name)+1)*sizeof(char);
    send(allclients[n].dsF,&tailleF,sizeof(int),0);
    send(allclients[n].dsF,name,(strlen(name)+1)*sizeof(char),0);
    FILE *fp;
    char* folder="ServerStorage/"; 
    char path[(strlen(name)+strlen(folder))*sizeof(char)];
    sprintf(path,"ServerStorage/%s",name);
    puts(path);
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
    printf("FILE SIZE : %ld\n",filesize);
    send(allclients[n].dsF,&filesize,sizeof(long),0);
    for(int i=0;i<filesize;i+=SIZE){
        if(i+SIZE<filesize){
            cpt=SIZE;
        }
        else{
            cpt=filesize-i;
            }
        fread(buffer,cpt,1,fp); 
        send(allclients[n].dsF,buffer,sizeof(buffer),0);
        bzero(buffer,SIZE);
    }
    printf("File sended \n");
    fclose(fp);
    pthread_exit(NULL);
}

void * receive_file(void *numcliact){
    int* numCliAct=numcliact;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    struct sockaddr_in aCF;
    socklen_t lgF = sizeof(struct sockaddr_in);
    allclients[n].dsF=accept(dsF,(struct sockaddr *)&aCF,&lgF);
    ///
    int taille;
    recv( allclients[n].dsF,&taille,sizeof(int),0); 
    if(taille<0){
        pthread_exit(NULL);
    }
    char *filename =(char*)malloc(taille*sizeof(char));
    recv( allclients[n].dsF,filename,taille*sizeof(char),0); 
    printf("Downloading %s\n",filename);
    ///
    FILE *fp;   
    char* folder="ServerStorage/";
    char path[strlen(filename)+strlen(folder)];
    sprintf(path,"ServerStorage/%s",filename);
    fp=fopen(path,"w+");
    char buffer[SIZE];
    long filesize;
    recv(allclients[n].dsF,&filesize,sizeof(long),0);
    printf("Size : %ld\n",filesize);
    int cpt;
        for(int i=0;i<filesize;i+=SIZE){
            if(i+SIZE<filesize){
                cpt=SIZE;
                }
            else{
                cpt=filesize-i;
            }
            recv(allclients[n].dsF,buffer,sizeof(buffer),0);
            fwrite(buffer, sizeof(buffer),1, fp);
            bzero(buffer,SIZE);
        }
    fclose(fp);
    printf("File downloaded\n");
    pthread_exit(NULL);
}
void *communication(void * NumCliAct){

    /////////////////////// Accueil Client ///////////////////////

    int* numCliAct=NumCliAct;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    printf("OK client numéro : %d\n",n);
    int taille;

    /////////////////////// Envoie Commandes ///////////////////////

    int taillefichier=getfilesize("Man.txt");
    send(allclients[n].dS,&taillefichier,sizeof(int),0);
    char* bufferMan=malloc(taillefichier*sizeof(char));
    bufferMan=getbuffer("Man.txt");
    send(allclients[n].dS,bufferMan,taillefichier*sizeof(char),0);
    
    while(1){
        getnbclient();
        int rec=recv(allclients[n].dS,&taille,sizeof(int),0);  // Recoit la taille
        if(rec==0){
            allchannels[allclients[n].channel].clientsco--;
            decoclient(n);
            break;
        }
        char *msg =(char*)malloc(taille*sizeof(char));
        recv(allclients[n].dS,msg,taille*sizeof(char),0); 
        
        int commande=command(msg);

        /////////////////////// Envoie Message ///////////////////////

        if (commande == -1) {
            taille=strlen(msg)+strlen(allchannels[allclients[n].channel].nom)+strlen(allclients[n].pseudo)+16; 
            char tosend[taille];
            sprintf(tosend,"[ %s ] %s : %s\n",allchannels[allclients[n].channel].nom,allclients[n].pseudo,msg);
            for(int i=0;i<nbclientsco;i++){
                if(allclients[i].num!=allclients[n].num && allclients[n].channel==allclients[i].channel){
                    send(allclients[i].dS,&taille, sizeof(int), 0) ;
                    send(allclients[i].dS, tosend, strlen(tosend)+1, 0) ;
                }
            }
        }
        
        /////////////////////// Commandes ///////////////////////

        if(commande>0){
            printf("command numéro ; %d\n",commande);

            /////////////////////// Commande Man ///////////////////////
            if(commande==1){
                taille=1000;
                send(allclients[n].dS,&taille, sizeof(int), 0) ;
                send(allclients[n].dS,bufferMan,taillefichier*sizeof(char),0);
            }
            if (commande==2){
                taille=1001;
                send(allclients[n].dS,&taille, sizeof(int), 0) ;
                taille=23 +strlen(allclients[n].pseudo);
                char tosend[taille];
                sprintf(tosend,"%s s'est déconnecté\n", allclients[n].pseudo); 
                for(int i=0;i<nbclientsco;i++){
                    if(allclients[i].num!=allclients[n].num && allclients[n].channel==allclients[i].channel){
                        send(allclients[i].dS,&taille, sizeof(int), 0) ;
                        send(allclients[i].dS, tosend, strlen(tosend)+1, 0) ;
                    }
                }
                allchannels[allclients[n].channel].clientsco--;
                decoclient(n);
                pthread_exit(NULL);
            }
            if (commande==3){
                taille=1002;
                send(allclients[n].dS,&taille, sizeof(int), 0) ;
                taille=23 + strlen(allclients[n].pseudo);
                char tosend[taille];
                sprintf(tosend,"%s s'est déconnecté\n", allclients[n].pseudo);
                for(int i=0;i<nbclientsco;i++){
                    if(allclients[i].num!=allclients[n].num && allclients[n].channel==allclients[i].channel){
                        send(allclients[i].dS,&taille, sizeof(int), 0) ;
                        send(allclients[i].dS, tosend, strlen(tosend)+1, 0) ;
                    }
                }
                allchannels[allclients[n].channel].clientsco--;
                allclients[n].pseudo="";
                allclients[n].channel=0;
                printf("client change channel\n");
                pthread_exit(NULL);
            }
            if(commande == 4) {
                int pv=SendPrivateMessage(msg,n);
                if(pv>=0){
                    printf("Message pv envoyé\n");
                }
                else{
                    taille=118;
                    char tosend[taille];
                    printf("No user with this pseudo\n");
                    sprintf(tosend,"Pseudo invalide, utilsez @acc: pour voir tout les utilisateurs connecté et @cc pour voir les utilisateurs du channel");
                    taille=strlen(tosend)+1;
                    send(allclients[n].dS, &taille, sizeof(int), 0) ;
                    send(allclients[n].dS, tosend, strlen(tosend)+1, 0) ;    
                } 
            }

            if(commande == 5 ) {
                listefiles("fichiersdisponibles.txt",1);
                taille=1005;
                send(allclients[n].dS,&taille, sizeof(int), 0) ;
                int taillefichier=getfilesize("fichiersdisponibles.txt");
                send(allclients[n].dS,&taillefichier,sizeof(int),0);
                char* bufferfiles=malloc(taillefichier*sizeof(char));
                bufferfiles=getbuffer("fichiersdisponibles.txt");
                send(allclients[n].dS,bufferfiles,taillefichier*sizeof(char),0);
                free(bufferfiles);
            }

            if(commande == 6 ) {
                int numvoulu=GetFileNum(msg,listefiles("fichiersdisponibles.txt",0));
                printf("NUm voulu : %d\n",numvoulu);
                if(numvoulu == 0){
                    taille=10060;
                    send(allclients[n].dS,&taille, sizeof(int), 0) ;
                   

                }
                else{
                    taille=10061;
                    send(allclients[n].dS,&taille, sizeof(int), 0) ;
                    send(allclients[n].dS,&numvoulu, sizeof(int), 0) ;
                    pthread_create(&files_send[n],NULL,sending_file,&n);  
                    pthread_join(files_send[n],NULL);  
                }
            }

            if(commande == 7){
                taille=1007;
                send(allclients[n].dS,&taille, sizeof(int), 0) ;
            }

            if(commande == 8 ) {
                taille=1008;
                int nbfichier;
                send(allclients[n].dS,&taille, sizeof(int), 0);
                recv(allclients[n].dS,&nbfichier,sizeof(int),0);
                int numvoulu=GetFileNum(msg,nbfichier);
                printf("%d\n",numvoulu);
                taille = 10080;
                if (numvoulu == 0) {
                    taille = 10080;
                    send(allclients[n].dS,&taille, sizeof(int), 0);
                    
                }
                else {
                    taille = 10081;
                    send(allclients[n].dS,&taille, sizeof(int), 0);
                    send(allclients[n].dS,&numvoulu, sizeof(int), 0);
                    printf("Ready to receive a file");
                    pthread_create(&files_receive[n],NULL,receive_file,&n);
                    pthread_join(files_receive[n],NULL);
                }
                
            }



        }
        //Si la commande n'est pas valide 
        else if(commande==0){
            taille=31;
            char tosend[taille];
            sprintf(tosend,"This is not a vailable command");
            send(allclients[n].dS, &taille, sizeof(int), 0) ;
            send(allclients[n].dS, tosend, strlen(tosend)+1, 0) ;
        }
        // Si ce n'est pas une commande 

        free(msg);
        
    }   
}

void *serveur(void * NumCliAct){

    /////////////////////// Accueil Client ///////////////////////
    
    int* numCliAct=NumCliAct;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    

    printf("Connexion du client numéro %d\n",n);
    int taillefichier=getfilesize("Welcome.txt");
    send(allclients[n].dS,&taillefichier,sizeof(int),0);
    char* buffer=malloc(taillefichier*sizeof(char));
    buffer=getbuffer("Welcome.txt");
    send(allclients[n].dS,buffer,taillefichier*sizeof(char),0);
    free(buffer);

    /////////////////////// Liste Channel ///////////////////////
    
    int nbchannels=getnbchannels();
    send(allclients[n].dS,&nbchannels,sizeof(int),0);
    writechannels("Channels.txt");
    int taillefichier2=getfilesize("Channels.txt");
    char* bufferChannel=malloc(taillefichier2*sizeof(char));
    send(allclients[n].dS,&taillefichier2,sizeof(int),0);
    bufferChannel=getbuffer("Channels.txt");
    send(allclients[n].dS,bufferChannel,taillefichier2*sizeof(char),0);
    free(bufferChannel);
    while (1) {
        int choixclient;
        int rec = recv(allclients[n].dS,&choixclient,sizeof(int),0)==0;

        /////////////////////// Deconnexion ///////////////////////

        if(rec == 1){
                decoclient(n);
                printf("client deco\n");
                pthread_exit(NULL);
        }

        /////////////////////// Choix Channel ///////////////////////

        else {
            printf("choix client : %d\n",choixclient);
            int etatChannel = 1;
            if (allchannels[choixclient-1].clientsco == NBCLIENTCHANNEL) {
                etatChannel = 0;
                send(allclients[n].dS,&etatChannel,sizeof(int),0);
                printf("channel pein ! : %d\n",choixclient);
            }
            else {
                send(allclients[n].dS,&etatChannel,sizeof(int),0);
                allclients[n].channel = choixclient-1;
                setpseudo(n);
                allchannels[choixclient-1].clientsco++;
                printf("Client numéro %d : %s rejoins le channel %d\n",allclients[n].num,allclients[n].pseudo,allclients[n].channel);
                printf("Création des thread.. \n");
                pthread_mutex_lock(&mutex);
                int numcliact=allclients[n].num;
                 pthread_mutex_unlock(&mutex);
                pthread_create(&ThreadClientsCo[allclients[n].num],NULL,communication,&numcliact);
                pthread_join(ThreadClientsCo[allclients[n].num],NULL);
                printf("ici ok \n");
                if(allclients[n].isConnected==0){
                    printf("Client déconnecté \n");
                    pthread_exit(NULL);
                }
                printf("ici ok2 \n");
        /////////////////////// Communication ///////////////////////
            }
        }
    }
    

    
}




int main(int argc, char *argv[]) { 

// ---------------------------------------------------------------------------------------------//

    // Innitialisation des Channels & Clients //
  
    innitialisechannels();
    innitialiseclients();
  
// ---------------------------------------------------------------------------------------------//

    printf("Démarage\n");
   
// ---------------------------------------------------------------------------------------------//


    // ------------------ Innitialisation des sockets clients ------------------ //

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY ;
    ad.sin_port = htons(PORT) ;
    if (bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) {
        perror("erreur nommer socket");
        };
    listen(dS, 7);
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    

    int dSC; // Socket client de base


    // ------------------ Innitialisation des sockets pour les files------------------ //
    dsF = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adF;
    adF.sin_family = AF_INET;
    adF.sin_addr.s_addr = INADDR_ANY ;
    adF.sin_port = htons(PortForFile) ;
    if (bind(dsF, (struct sockaddr*)&adF, sizeof(adF)) == -1) {
        perror("erreur nommer socket");
    };
    listen(dsF, 7) ;
   

// ---------------------------------------------------------------------------------------------//
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // initialisation mutex

    int full=0;
    while(1){
        printf("blo\n");
        dSC=accept(dS,(struct sockaddr *)&aC,&lg);
        printf("quant\n");
        int numcliact = getclientdispo();
        if(numcliact == -1){
            full=1;
            send(dSC,&full, sizeof(int), 0) ; 
        }
        else{
            full=0;
            send(dSC,&full, sizeof(int), 0) ; 
            pthread_mutex_lock(&mutex); //mutex lock 
            nbclientsco++;
            pthread_mutex_unlock(&mutex); //mutex unlock
            allclients[numcliact].isConnected = 1;
            allclients[numcliact].dS = dSC;
            printf("numcliact : %d\n",numcliact);
            pthread_create(&ThreadClientsCo[numcliact],NULL,serveur,&numcliact);
        }            
    }   
}