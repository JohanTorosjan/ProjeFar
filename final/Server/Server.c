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
#define MDP "mdp"

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

void printrouge(char * msg){
    printf("\033[31m" );
    printf("%s",msg);
    printf("\033[37m" );
}

void printvert(char * msg){
    printf("\033[32m" );
    printf("%s",msg);
    printf("\033[37m" );
}

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

    allchannels[allclients[numclient].channel].clientsco--;
    Client client = {numclient,0,"",-1,0,0,0};
    allclients[numclient] = client; 
    nbclientsco --;

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
    int taillefichier=1+getfilesize(path);
    char* buffer=malloc(taillefichier*sizeof(char));
    int i=0;
    while((c=fgetc(fp))!=EOF){
       buffer[i]=c;
       i++;
    }
    fclose(fp);
    return buffer;
}
int createchannel(char* nom, char* descr){
    int i=0;
    while(allchannels[i].num!=-1){
        i++;
    }

    allchannels[i].num=i;
    //allchannels[i].nom=nom;
    allchannels[i].nom=malloc(strlen(nom)*sizeof(char));
    strcpy(allchannels[i].nom,nom);
    allchannels[i].description=malloc(strlen(descr)*sizeof(char));
    strcpy(allchannels[i].description,descr);
    printvert("Channel crée");
    printf("\n");
    return i;
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
    fwrite("\0",1,1,fpc);
    fclose(fpc);
}


void writeclients(char* path){
    
    FILE *fpc;
    fpc=fopen(path,"w+");
    for(int i=0;i<NBCLIENTCHANNEL*NBCHANNEL;i++){
        if(allclients[i].channel!=-1){
            char infoclient[strlen(allchannels[allclients[i].channel].nom)+strlen(allclients[i].pseudo)+27];
            sprintf(infoclient,"[%s] client N° %d : %s \n",allchannels[allclients[i].channel].nom,i+1,allclients[i].pseudo);
            fwrite(infoclient,strlen(infoclient)*sizeof(char),1,fpc);
        }  
    }
    fwrite("\0",1,1,fpc);
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
        d = opendir("./ServerStorage");
        int nbrfichier=cpt;
        cpt=1;
        while ((dir = readdir(d)) != NULL){
            if(dir->d_type==8){
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
    char allCommand[13][10] = {""};
    strcpy(allCommand[0], "@man");
    strcpy(allCommand[1], "@end");
    strcpy(allCommand[2], "@lc");
    strcpy(allCommand[3], "@mp");
    strcpy(allCommand[4], "@lss");
    strcpy(allCommand[5], "@get");
    strcpy(allCommand[6], "@ls");
    strcpy(allCommand[7], "@snd");
    strcpy(allCommand[8], "@adm");
    strcpy(allCommand[9], "@kick");
    strcpy(allCommand[10], "@crt");
    strcpy(allCommand[11], "@acc");
    strcpy(allCommand[12], "@acm");
    if(msg[0]=='@'){
        char* msgcopy=malloc(strlen(msg)+1);
        strcpy(msgcopy,msg);
        char* token=strtok(msgcopy,":");
        int c = 0;
        for(int i=0;i<13;i++){
            if(strcmp(token,allCommand[i])==0){
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
int setpseudo(int n){
    
    int taillepseudo;
    int pseudok=0;
    while(pseudok==0){
        int rec = recv(allclients[n].dS,&taillepseudo,sizeof(int),0);
            /////////////////////// Deconnexion ///////////////////////
        if(rec == 0){
               // decoclient(n);
                return -1;
                // pthread_exit(NULL);
        }

        char pseudo[taillepseudo];
        recv(allclients[n].dS,pseudo,sizeof(char)*taillepseudo,0);
        char *pos;
        pos=strchr(pseudo,'\n');
        *pos='\0';
        pseudok=1;
        for (int i = 0; i < NBCLIENTCHANNEL*NBCHANNEL; i++) {
            if( (strcmp(allclients[i].pseudo,pseudo)==0) || (strlen(pseudo)<2) || (pseudo[0]=='@') || (pseudo[0]==' ') )
            {
                pseudok=0;
            }
        }
        send(allclients[n].dS,&pseudok,sizeof(int),0);
        if(pseudok==1){
            pthread_mutex_lock(&mutex);
            allclients[n].pseudo=pseudo;
            pthread_mutex_unlock(&mutex);
        }
    }
     
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

char * getacm(char * msg){
    char msgcpy[strlen(msg)];
    strcpy(msgcpy,msg);
    char * cpy;
    cpy = strtok(msgcpy,":");    
    cpy=strtok(NULL,"\0");
    return cpy;
}


int verifadmin(char * msg){
    char* mdp;
    char copy[strlen(msg)+1];
    strcpy(copy,msg);
    // char * msgcopy;
    // msgcopy=strtok(copy,":");
    // msgcopy=strtok(NULL,":");
    // msgcopy=strtok(NULL,"\0");
    mdp=strtok(msg,":");
    mdp=strtok(NULL,":");
    if(strcmp(mdp,MDP)==0){
        return 1;
    }
    return 0;
}

int estadmin(int n){
    int estadmin=1;
    if (allclients[n].isAdmin==0)
    {
        estadmin=20000;
        send(allclients[n].dS,&estadmin, sizeof(int),0);
    }
    return estadmin;
}


int verifchannel(char* nom, char* descr){
    if( (strcmp(nom,"")==0) || ( strcmp(descr,"") ==0 ) ) {
        return -1;
    }

    for(int i=0;i<getnbchannels();i++){
        if(strcmp(nom,allchannels[i].nom)==0){
            return 0;
        }
    }
    return 1;
}
/**
 * @brief 
 * retourne -2 si le client voulant etre kicker est admin 
 * retourne -1 si il n'y a pas de pseudo correspondant
 * retourne i le numéro du client kicker
 * @param msg 
 * @return int 
 */

int kickvalide(char* msg){
    getnbclient();
    char* pseudo;
    char copy[strlen(msg)+1];
    strcpy(copy,msg);
    pseudo=strtok(msg,":");
    pseudo=strtok(NULL,":");
    for(int i=0;i<=nbclientsco;i++){
        if (strcmp(allclients[i].pseudo,pseudo)==0){
            if(allclients[i].isAdmin==1){
                return -2;
            }
            else {
                return i;
            }
        }
    }
    return -1;

}

void * sending_file(void* numcliact){   

    int* numCliAct=numcliact;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    int FileNumber;
    recv(allclients[n].dS,&FileNumber,sizeof(int),0);
    struct sockaddr_in aCF;
    socklen_t lgF = sizeof(struct sockaddr_in);
    allclients[n].dsF=accept(dsF,(struct sockaddr *)&aCF,&lgF);
    char* name = getfilename(FileNumber); // a free 
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
        shutdown(dsF,SHUT_RDWR);
        pthread_exit(NULL);
    } 
    fseek(fp,0,SEEK_END);
    long filesize= ftell(fp);
    rewind(fp);
    char buffer[SIZE];
    int cpt;
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
    printvert("File sended \n");
    printf("\n");
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
    ///
    FILE *fp;   
    char* folder="ServerStorage/";
    char path[strlen(filename)+strlen(folder)];
    sprintf(path,"ServerStorage/%s",filename);
    fp=fopen(path,"w+");
    char buffer[SIZE];
    long filesize;
    recv(allclients[n].dsF,&filesize,sizeof(long),0);
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
    printvert("File downloaded\n");
    printf("\n");
    pthread_exit(NULL);
}
void *communication(void * NumCliAct){

    /////////////////////// Accueil Client ///////////////////////

    int* numCliAct=NumCliAct;
    int n=*numCliAct; // Recup le numéro associé à chaque clients
    int taille;

    /////////////////////// Envoie Commandes ///////////////////////

    int taillefichier=1+getfilesize("Man.txt");
    send(allclients[n].dS,&taillefichier,sizeof(int),0);
    char* bufferMan=malloc(taillefichier*sizeof(char));
    bufferMan=getbuffer("Man.txt");
    send(allclients[n].dS,bufferMan,taillefichier*sizeof(char),0);
    
    while(1){
        getnbclient();
        int rec=recv(allclients[n].dS,&taille,sizeof(int),0);  // Recoit la taille
        if(rec==0){
            decoclient(n);
            break;
        }
        char *msg =(char*)malloc(taille*sizeof(char));
        recv(allclients[n].dS,msg,taille*sizeof(char),0); 
        
        int commande=command(msg);

        /////////////////////// Envoie Message ///////////////////////

        if (commande == -1) {
            taille=strlen(msg)+strlen(allchannels[allclients[n].channel].nom)+strlen(allclients[n].pseudo)+14; 
            char tosend[taille];
            sprintf(tosend,"[%s] %s : %s\n",allchannels[allclients[n].channel].nom,allclients[n].pseudo,msg);
            
            int tailleadmin=strlen(msg)+strlen(allchannels[allclients[n].channel].nom)+strlen(allclients[n].pseudo)+17; 
            char tosendadmin[tailleadmin];
            sprintf(tosendadmin,"ADMIN [%s] : %s\n",allclients[n].pseudo,msg);
            
            for(int i=0;i<nbclientsco;i++){
                if(allclients[i].num!=allclients[n].num && allclients[n].channel==allclients[i].channel){
                    if(allclients[n].isAdmin==1){
                        send(allclients[i].dS,&tailleadmin, sizeof(int), 0) ;
                        send(allclients[i].dS, tosendadmin, strlen(tosendadmin)+1, 0) ;
                    }
                    else{
                        send(allclients[i].dS,&taille, sizeof(int), 0) ;
                        send(allclients[i].dS, tosend, strlen(tosend)+1, 0) ;
                    }
        
                }
            }
        }
        
        /////////////////////// Commandes ///////////////////////

        if(commande>0){

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
                //allchannels[allclients[n].channel].clientsco--;
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
                allclients[n].channel=-1;
                int nbchannels=getnbchannels();
                send(allclients[n].dS,&nbchannels,sizeof(int),0);
                writechannels("Channels.txt");
                int taillefichier2=1+getfilesize("Channels.txt");
                char* bufferChannel=malloc(taillefichier2*sizeof(char));
                send(allclients[n].dS,&taillefichier2,sizeof(int),0);
                bufferChannel=getbuffer("Channels.txt");
                send(allclients[n].dS,bufferChannel,taillefichier2*sizeof(char),0);
                free(bufferChannel);

                pthread_exit(NULL);
            }
            if(commande == 4) {
                int pv=SendPrivateMessage(msg,n);
                if(pv>=0){
                    printvert("Message pv envoyé\n");
                    printf("\n");
                }
                else{
                    taille=73;
                    char tosend[taille];
                    printrouge("No user with this pseudo\n");
                    printf("\n");
                    sprintf(tosend,"Pseudo invalide, utilsez @acc: pour voir tout les utilisateurs connecté");
                    taille=strlen(tosend)+1;
                    send(allclients[n].dS, &taille, sizeof(int), 0) ;
                    send(allclients[n].dS, tosend, strlen(tosend)+1, 0) ;    
                } 
            }

            if(commande == 5 ) {
                listefiles("fichiersdisponibles.txt",1);
                taille=1005;
                send(allclients[n].dS,&taille, sizeof(int), 0) ;
                int taillefichier=1+getfilesize("fichiersdisponibles.txt");
                send(allclients[n].dS,&taillefichier,sizeof(int),0);
                char* bufferfiles=malloc(taillefichier*sizeof(char));
                bufferfiles=getbuffer("fichiersdisponibles.txt");
                send(allclients[n].dS,bufferfiles,taillefichier*sizeof(char),0);
                free(bufferfiles);
            }

            if(commande == 6 ) {
                int numvoulu=GetFileNum(msg,listefiles("fichiersdisponibles.txt",0));
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
                taille = 10080;
                if (numvoulu == 0) {
                    taille = 10080;
                    send(allclients[n].dS,&taille, sizeof(int), 0);
                    
                }
                else {
                    taille = 10081;
                    send(allclients[n].dS,&taille, sizeof(int), 0);
                    send(allclients[n].dS,&numvoulu, sizeof(int), 0);
                    printvert("Ready to receive a file");
                    printf("\n");
                    pthread_create(&files_receive[n],NULL,receive_file,&n);
                    pthread_join(files_receive[n],NULL);
                }
            }

            if(commande == 9){
                taille = 1009;
                send(allclients[n].dS,&taille, sizeof(int), 0);      
                int estadmin=verifadmin(msg);
                if(estadmin==1){
                    allclients[n].isAdmin=1;
                    send(allclients[n].dS,&estadmin, sizeof(int),0);
                }
                else{ 
                    send(allclients[n].dS,&estadmin, sizeof(int),0);
                }
            }

            if( commande == 10) {
                int verificationadmin=estadmin(n);
                if(verificationadmin==1){
                    taille = 1010;
                    send(allclients[n].dS,&taille, sizeof(int), 0);    
                    int valide=kickvalide(msg);
                    send(allclients[n].dS,&valide, sizeof(int),0);
                    if(valide>=0){
                        
                        
                        // Envoie kick au client //
                        int kick=20021;
                        send(allclients[valide].dS,&kick, sizeof(int),0);
                        taille=strlen(allclients[n].pseudo);
                        send(allclients[valide].dS,&taille, sizeof(int), 0) ;
                        send(allclients[valide].dS, allclients[n].pseudo, taille+1, 0) ;
                        // --------------------//

                        taille=strlen(allclients[valide].pseudo)+strlen(allclients[n].pseudo)+27;
                        char tosend[taille];
                        sprintf(tosend,"%s s'est fait kick par %s\n",allclients[valide].pseudo,allclients[n].pseudo);
                        decoclient(valide);
                        for(int i=0;i<nbclientsco;i++){
                            send(allclients[i].dS,&taille, sizeof(int), 0) ;
                            send(allclients[i].dS, tosend, strlen(tosend)+1, 0) ;
                        }        
                    }
                }
            }

            if(commande == 11){
                int verificationadmin=estadmin(n);
                if(verificationadmin==1){

                    int channeldispos=NBCHANNEL-getnbchannels();
                    if(channeldispos>0){
                        taille =10111;
                        send(allclients[n].dS,&taille, sizeof(int), 0);
                        
                        
                        int channelok=0;
                        while(channelok!=1){
                            printvert("Mode reception\n");
                            printf("\n");
                            
            //--------------------------------reception nom---------------------------------//
                            int taillenom;
                            int rec = recv(allclients[n].dS,&taillenom,sizeof(int),0);
                    
                    /////////////////////// Deconnexion ///////////////////////
                            if(rec == 0){
                                decoclient(n);
                                pthread_exit(NULL);
                            }

                            char nomchannel[taillenom];
                            recv(allclients[n].dS,nomchannel,sizeof(char)*taillenom,0);
                            char *pos;
                            pos=strchr(nomchannel,'\n');
                            *pos='\0';
                            
                            
            //--------------------------------reception description---------------------------------//
                            int tailledescr;
                            rec = recv(allclients[n].dS,&tailledescr,sizeof(int),0);
                    
                    /////////////////////// Deconnexion ///////////////////////
                            if(rec == 0){
                                decoclient(n);
                                pthread_exit(NULL);
                            }
                            char descriptionchannel[tailledescr];
                            recv(allclients[n].dS,descriptionchannel,sizeof(char)*tailledescr,0);
                            pos=strchr(descriptionchannel,'\n');
                            *pos='\0';
            //------------------------------------------------------------------------------------//
                            channelok=verifchannel(nomchannel,descriptionchannel);
                            send(allclients[n].dS,&channelok, sizeof(int), 0);
                            if(channelok==1){
                                createchannel(nomchannel,descriptionchannel); 
                            }
                        }
                    }

                    else{
                        taille =10110;
                        send(allclients[n].dS,&taille, sizeof(int), 0);
                    }
                }
            }

            if(commande == 12){
                taille=1012;
                send(allclients[n].dS,&taille, sizeof(int), 0);
                send(allclients[n].dS,&nbclientsco,sizeof(int),0);
                writeclients("Clients.txt");
                int tailleclient=1+getfilesize("Clients.txt");
                char* bufferClient=malloc(tailleclient*sizeof(char));
                send(allclients[n].dS,&tailleclient,sizeof(int),0);
                bufferClient=getbuffer("Clients.txt");
                send(allclients[n].dS,bufferClient,tailleclient*sizeof(char),0);
                free(bufferClient);     
            }

            if(commande ==13){
                int verificationadmin=estadmin(n);
                if(verificationadmin==1){
                    char * acm=getacm(msg);
                    int tailleadmin=strlen(msg)+strlen(allchannels[allclients[n].channel].nom)+strlen(allclients[n].pseudo)+33; 
                    char tosendadmin[tailleadmin];
                    sprintf(tosendadmin,"ADMIN [%s] TO ALL CHANNELS : %s\n",allclients[n].pseudo,acm);
                    for(int i=0;i<nbclientsco;i++){
                        send(allclients[i].dS,&tailleadmin, sizeof(int), 0) ;
                        send(allclients[i].dS, tosendadmin, strlen(tosendadmin)+1, 0) ;
                    }
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

    int taillefichier=1+getfilesize("Welcome.txt");
    send(allclients[n].dS,&taillefichier,sizeof(int),0);
    char* buffer=malloc(taillefichier*sizeof(char));
    buffer=getbuffer("Welcome.txt");
    send(allclients[n].dS,buffer,taillefichier*sizeof(char),0);
    free(buffer);

    /////////////////////// Liste Channel ///////////////////////
    
    int nbchannels=getnbchannels();
    send(allclients[n].dS,&nbchannels,sizeof(int),0);
    writechannels("Channels.txt");
    int taillefichier2=1+getfilesize("Channels.txt");
    char* bufferChannel=malloc(taillefichier2*sizeof(char));
    send(allclients[n].dS,&taillefichier2,sizeof(int),0);
    bufferChannel=getbuffer("Channels.txt");
    send(allclients[n].dS,bufferChannel,taillefichier2*sizeof(char),0);
    free(bufferChannel);

    
    while (1) {

        /////////////////////// Choix Channel Client ///////////////////////

        int choixclient;
        int rec = recv(allclients[n].dS,&choixclient,sizeof(int),0)==0;

        /////////////////////// Deconnexion ///////////////////////

        if(rec == 1){
                decoclient(n);
                printrouge("client deco\n");
                printf("\n");
                pthread_exit(NULL);
        }

        /////////////////////// Choix Channel ///////////////////////

        else {
            int etatChannel = 1;
            if (allchannels[choixclient-1].clientsco == NBCLIENTCHANNEL) {
                etatChannel = 0;
                send(allclients[n].dS,&etatChannel,sizeof(int),0);
            }
            else {
                send(allclients[n].dS,&etatChannel,sizeof(int),0);
                allclients[n].channel = choixclient-1;
                int okpseudo=setpseudo(n);
                allchannels[choixclient-1].clientsco++;
                if(okpseudo==-1){
                    decoclient(n);
                    printrouge("Client déconnecté \n");
                    printf("\n");
                    pthread_exit(NULL);
                }
                printf("Client numéro %d : %s rejoins le channel %d\n",allclients[n].num,allclients[n].pseudo,allclients[n].channel);
                pthread_mutex_lock(&mutex);
                int numcliact=allclients[n].num;
                 pthread_mutex_unlock(&mutex);
                pthread_create(&ThreadClientsCo[allclients[n].num],NULL,communication,&numcliact);
                pthread_join(ThreadClientsCo[allclients[n].num],NULL);
                if(allclients[n].isConnected==0){
                    printf("Client déconnecté \n");
                    pthread_exit(NULL);
                }
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
    printvert("########################################################\n");
    printvert("Démarage\n");
    printvert("########################################################");
   
   printf("\n");
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
        dSC=accept(dS,(struct sockaddr *)&aC,&lg);
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
            pthread_create(&ThreadClientsCo[numcliact],NULL,serveur,&numcliact);
        }            
    }   
}