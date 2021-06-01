#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "readcmd.h"

int main(int argc, char *argv[], char *arge[]) {
    bool boucle = true;
    while(boucle) {
        
        printf("DG-minishell$ ");
        
        char  buf [30];                                 /*  contient  la  commande  saisie  au  clavier  */
        int  ret;                                       /*  valeur  de  retour  de  scanf */
        ret = scanf("%s", buf);
        pid_t pidFils;

        if (ret == EOF) {
            boucle = false;
        } else if (strcmp(buf, "exit") == 0) {
            exit(1);
        } else if (strcmp(buf, "cd") == 0) {
            execlp(buf, buf, NULL);
        } else {
            pidFils = fork();

            if (pidFils == -1) {
            printf("Erreur fork\n");
            exit(1);
            } else if (pidFils == 0) {		            /* fils */
                execlp(buf, buf, NULL);
                exit(EXIT_SUCCESS);
            } else {
                waitpid(pidFils, NULL, WUNTRACED);
            }
        }
    }
}
