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

        char cwd[128];
        printf("%s$DG-minishell$ ", getcwd(cwd, 128));
        
        struct cmdline *commande = readcmd();
        pid_t pidFils;

        if (*commande->seq[0][0] == EOF) {
            boucle = false;
        } else if (strcmp(commande->seq[0][0], "exit") == 0) {
            exit(1);
        } else if (strcmp(commande->seq[0][0], "cd") == 0) {
            printf("%s\n", *commande->seq[0]);
            chdir(commande->seq[0][1]);
        } else if (*commande->seq[0][0] == '\n') {
            printf("\n");
        } else {
            pidFils = fork();

            if (pidFils == -1) {
                printf("Erreur fork\n");
                exit(1);
            } else if (pidFils == 0) {		            /* fils */
                execvp(commande->seq[0][0], commande->seq[0]);
                exit(pidFils);
            } else {
                if (commande->backgrounded == NULL) {
                    waitpid(pidFils, NULL, WUNTRACED);
                }
            }
        }
    }
}
