#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "readcmd.h"

int main(int argc, char *argv[], char *arge[]) {
    bool boucle = true;
    while(boucle) {
        
        printf("DG-minishell$ ");
        
        char  buf [30];                                 /*  contient  la  commande  saisie  au  clavier  */
        int  ret;                                       /*  valeur  de  retour  de  scanf */
        ret = scanf("%s", buf);
        pid_t pidFils;

        if (ret == EOF || strcmp(buf, "exit") == 0) {
            boucle = false;

        } else {
            pidFils = fork();

            if (pidFils == -1) {
            printf("Erreur fork\n");
            exit(1);
            } else if (pidFils == 0) {		            /* fils */
                execlp(buf, buf, NULL);
                exit(EXIT_SUCCESS);
            }
        }
    }
}
