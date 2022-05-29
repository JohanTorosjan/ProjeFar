#include <stdio.h>
#include <stdlib.h>


// colorier le texte dans un terminal linux
// (couleur de fond printf("\033[40m") à printf("\033[47m"))
 
#define NOIR      1
#define ROUGE     2
#define VERT      3
#define ORANGE    4
#define BLEU      5
#define MAGENTA   6
#define CYAN      7
#define BLANC     8
#define JAUNE     9




void couleur_char(char c);
char *nom_couleur(char c);
void printrouge(char* msg);
void printvert(char * msg);

main ()
{
   char c;

   for (c = NOIR; c <= BLEU; c++) {
      couleur_char (c);
      printf("%c%s%s%s", c + 0x30, "  Mon texte en '",  nom_couleur (c), "'");
      couleur_char (c+1);
      printf("%c%s%s%s", c + 0x30, "  Mon texte en '",  nom_couleur (c+1), "'");
   }
   printrouge("bjr");
   printvert("YO");

   exit (0);
}



void printrouge(char * msg){
    printf("\033[31m" );
    printf("%s\n",msg);
    printf("\033[37m" );
}

void printvert(char * msg){
    printf("\033[32m" );
    printf("%s\n",msg);
    printf("\033[37m" );

}


void couleur_char(char c)
{
   switch (c) {
      case NOIR    : printf("\033[30m" ); break;
      case ROUGE   : printf("\033[31m" ); break;
      case VERT    : printf("\033[32m" ); break;
      case ORANGE  : printf("\033[33m" ); break;
      case BLEU    : printf("\033[34m" ); break;
      case MAGENTA : printf("\033[35m" ); break;
      case CYAN    : printf("\033[36m" ); break;
      case BLANC   : printf("\033[37m" ); break;
      case JAUNE   : printf("\033[00m" ); break;
      default      : printf("\ncouleur non répertoriée\n");
   }
}






char *nom_couleur(char c)
{
   switch (c) {
      case NOIR    : return    ("noir");
      case ROUGE   : return   ("rouge");
      case VERT    : return    ("vert");
      case ORANGE  : return  ("orange");
      case BLEU    : return    ("bleu");
      case MAGENTA : return ("magenta");
      case CYAN    : return    ("cyan");
      case BLANC   : return   ("blanc");
      case JAUNE   : return   ("jaune");
      default      : return      (NULL);
   }
}