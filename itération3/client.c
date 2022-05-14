#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include<arpa/inet.h>
#include<unistd.h>

//gcc -o client client.c -lpthread
#define PORT 2637
#define PORTforFILE 2638
#define IP "127.0. 0.1."
#define TAILLE 200
#define SIZE 1024
#define KEY "KHFZIEjkfejfkfkjdsfkjKJNFVEOKJFKLE?f;ld,sgklezj,gkpJfgkLekoẑakeopfkfopezkgopekzfezjgLKKGJLEFLK?KLEdgdslfkdslgkseldmgk;ezldjgkdjgOZJFKIOEPJPOA98787893U0948jgkjdfks"
char pseudo[TAILLE];
char fin[]="@end:\n";
int dS;
pthread_t thread_ReceivingFile;
void quit(){
  printf("Serveur shutdown \n");
  shutdown(dS,SHUT_RDWR);
  exit(0);
}




////////////////////////////////////////////////
int listfiles(){
  printf("\n");
  DIR *d = opendir("./FilesToUpload");
  struct dirent *dir;
  int cpt=1;
  printf("----------------------\n");
  if (d)
    {
      while ((dir = readdir(d)) != NULL){

        if(dir->d_type==8){
          printf("%d : %s\n",cpt, dir->d_name);
          cpt++;
        }
      }
    closedir(d);
    }
    printf("----------------------\n");
    printf("Choose a file or enter quit to leave\n");
    return cpt;
}

char* getfilename(int n){
  char* filename;
  DIR *d = opendir("./FilesToUpload");
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

void *SendingFile(void *num){
  int* Num=num;
  int n= *Num; // Recup le numéro associé à chaque clients
  char* name = getfilename(n);
  FILE *fp;
  int dSF = socket(PF_INET, SOCK_STREAM, 0);
  if (dSF == -1) {
      perror("erreur dans l'initialisation du socket");
  }
  struct sockaddr_in aSF;
  aSF.sin_family = AF_INET;
  inet_pton(AF_INET,IP,&(aSF.sin_addr)) ;
  aSF.sin_port = htons(PORTforFILE);
  socklen_t lgAF = sizeof(struct sockaddr_in) ;
  if (connect(dSF, (struct sockaddr *) &aSF, lgAF) == -1 ) {
      perror("erreur dans la connection du socket");
      exit(0);
  }
  if(n<0){
    send(dSF,&n,sizeof(int),0);
    shutdown(dSF,SHUT_RDWR);
    pthread_exit(NULL);    
    }
  int tailleF=(strlen(name)+1)*sizeof(char);
  if(send(dSF,&tailleF,sizeof(int),0)==0){quit();}
  send(dSF,name,(strlen(name)+1)*sizeof(char),0);
  char* folder="FilesToUpload/";
  char* path=(char*)malloc((strlen(name)+strlen(folder))*sizeof(char));
  sprintf(path,"FilesToUpload/%s",name);
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
  send(dSF,&filesize,sizeof(long),0);
  for(int i=0;i<filesize;i+=SIZE){
    if(i+SIZE<filesize){
      cpt=SIZE;
    }
    else{
      cpt=filesize-i;
    }
    fread(buffer,cpt,1,fp);
    send(dSF,buffer,sizeof(buffer),0);

    bzero(buffer,SIZE);
  }
  printf("File sended \n");
  fclose(fp);
  free(path);
  shutdown(dSF,SHUT_RDWR);
  pthread_exit(NULL);
}

void *ReceivingFile(void* num){
  int* Num=num;
  int n= *Num; // Recup le numéro associé à chaque clients
  send(dS,&n,sizeof(int),0);
  int dSF = socket(PF_INET, SOCK_STREAM, 0);
  if (dSF == -1) {
      perror("erreur dans l'initialisation du socket");
  }
  struct sockaddr_in aSF;
  aSF.sin_family = AF_INET;
  inet_pton(AF_INET,IP,&(aSF.sin_addr)) ;
  aSF.sin_port = htons(PORTforFILE);
  socklen_t lgAF = sizeof(struct sockaddr_in) ;
  if (connect(dSF, (struct sockaddr *) &aSF, lgAF) == -1 ) {
      perror("erreur dans la connection du socket");
      exit(0);
  }
  int taille;
  recv(dSF, &taille,sizeof(int), 0);
  char *filename = (char*)malloc(taille*sizeof(char));
  recv(dSF, filename, taille, 0); 
  FILE *fp;   
  char* folder="DownloadedFiles/";
  char* path=(char*)malloc((strlen(filename)+strlen(folder))*sizeof(char));
  sprintf(path,"DownloadedFiles/%s",filename);
  fp=fopen(path,"w+");
  char buffer[SIZE];
  long filesize;
  recv(dSF,&filesize,sizeof(long),0);
  int cpt;
      for(int i=0;i<filesize;i+=SIZE){
        if(i+SIZE<filesize){
             cpt=SIZE;
            }
        else{
             cpt=filesize-i;
        }
        recv(dSF,buffer,sizeof(buffer),0);
        fwrite(buffer, sizeof(buffer),1, fp);
        bzero(buffer,SIZE);
    }
    fclose(fp);
    printf("File downloaded\n");
    free(path);
    shutdown(dSF,SHUT_RDWR);
    pthread_exit(NULL);

}


void *envoi() {
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
    }

}

//Fonction de reception :
void *reception(void *ds) {
  while(1){
    
    int taille;
    if(recv(dS, &taille, sizeof(int), 0)==0){quit();}
    char *msg = (char*)malloc(taille*sizeof(char));
    recv(dS, msg, taille, 0);
    if(strcmp(msg,KEY)==0){
      int n;
      recv(dS,&n, sizeof(int), 0);
      pthread_create(&thread_ReceivingFile,NULL,ReceivingFile,&n);
      pthread_join(thread_ReceivingFile,NULL);
    }
    else{
      puts(msg);
    }

  }
}

int main(int argc, char *argv[]) {
  pthread_t thread_reception;
	pthread_t thread_envoi;


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
  printf("################################\n");
  int connected=1;
  int accepted;

  while(connected==1){
  if(recv(dS, &accepted, sizeof(int), 0)==0){quit();}
  if(accepted!=1){
    printf("Server is full for the moment \n");
    exit(0);
  }
    printf("Choose a pseudo: ");
    fgets(pseudo,TAILLE,stdin);
    send(dS,pseudo,(strlen(pseudo)+1)*sizeof(char),0);
    if(recv(dS, &connected, sizeof(int), 0)==0){quit();}
    if(connected==1){
      printf("This pseudo already exists \n");
      dS = socket(PF_INET, SOCK_STREAM, 0);
      connect(dS, (struct sockaddr *) &aS, lgA);
    }
  }
  printf("Welcome,you're connected as %s\n",pseudo);
  printf("Press @man: to see the availables commands");
  // Innitialisation des thread //
  pthread_create(&thread_envoi,NULL,envoi,NULL);
  pthread_create(&thread_reception,NULL,reception,NULL);

  pthread_join(thread_envoi,NULL);
  pthread_join(thread_reception,NULL);
  return 0;
}











