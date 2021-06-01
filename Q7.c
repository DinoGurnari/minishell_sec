#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "readcmd.h"
#define MAX_PROCESSUS 16

enum etat{ACTIF = 1, SUSPENDU = 0};     //définition d'un type énumération
struct Processus {                      //définition d'un struct Processus contenant les infos
    int ident;
    pid_t pid;
    enum etat etat;
    char* commande_lancee;
};
struct Processus* listeP[MAX_PROCESSUS];  //tableaux des processus
pid_t pid_fg;
int id_fg;

/* Permet de rajouter un processus à la liste listeP
 * Renvoie -1 si la liste est pleine
 * Renvoie ident si réussi
 */ 
int ajoutProcessus(pid_t pid, struct cmdline* commande_lancee) {
    int i = 0;
    while (listeP[i] != NULL && i < MAX_PROCESSUS) {
        i++;
    }
    if (i == MAX_PROCESSUS) {
        return -1;

    } else {
        listeP[i] = malloc(sizeof(struct Processus));
        listeP[i]->ident = i;
        listeP[i]->pid = pid;
        listeP[i]->etat = ACTIF;
        listeP[i]->commande_lancee = malloc(sizeof(char));
        strcpy(listeP[i]->commande_lancee, commande_lancee->seq[0][0]);
        return i;
    }
}

/* Permet de trouver un processus dans la liste listeP
 * Renvoie -1 si le processus n'est pas trouvé
 * Renvoie sa position (son ident minishell) si réussite
 */ 
int trouverProcessus(pid_t pid) {
    int i;
    int ident = -1;
    for (i = 0; i < MAX_PROCESSUS; i++) {
        if ((listeP[i] != NULL) && (listeP[i]->pid == pid)) {
            ident = listeP[i]->ident;
        }
    }
    return ident;
}

/* Permet de supprimer un processus de la liste listeP
 * Renvoie -1 si le processus n'est pas trouvé
 * Renvoie 1 si réussite
 */ 
int retirerProcessus(pid_t pid) {
    int ident = trouverProcessus(pid);
    if (ident == -1) {
        return -1;
    } else {                                        //Libération de la mémoire
        free(listeP[ident]->commande_lancee);
        listeP[ident]->commande_lancee = NULL;

        free(listeP[ident]);
        listeP[ident] = NULL;
        return 1;
    }
}

void suivi_fils (int sig) {

    int etat_fils, pid_fils;
    do {

        pid_fils = (int) waitpid(-1, &etat_fils, WNOHANG | WUNTRACED | WCONTINUED);

        if ((pid_fils == -1) && (errno != ECHILD)) {
            perror("waitpid");
            exit(EXIT_FAILURE);

        } else if (pid_fils > 0) {
            if (WIFSTOPPED(etat_fils)) {
                /* traiter la suspension */
                int ident = trouverProcessus(pid_fils);
                if (ident == -1) {
                    perror("stop pid_fils");
                    exit(EXIT_FAILURE);
                }
                listeP[ident]->etat = SUSPENDU;

            } else if (WIFCONTINUED(etat_fils)) {
                /* traiter la reprise */
                int ident = trouverProcessus(pid_fils);
                if (ident == -1) {
                    perror("continue pid_fils");
                    exit(EXIT_FAILURE);
                }
                listeP[ident]->etat = ACTIF;

            } else if (WIFEXITED(etat_fils)) {
                /* traiter exit */
                int ident = retirerProcessus(pid_fils);
                if ( ident == -1) {
                    perror("exit pid_fils");
                    exit(EXIT_FAILURE);
                }

            } else if (WIFSIGNALED(etat_fils)) {
                /* traiter signal */
            }
        }
    } while (pid_fils > 0);
    /* autres actions après le suivi des changements d'état */

}

void handler_SIGTSTP (int sig) {
    printf("\n");
    if (pid_fg > -1) {
        listeP[id_fg]->etat = SUSPENDU;

        if (kill(pid_fg, SIGSTOP) == -1) {
            perror("SIGTSTP");
            exit(EXIT_FAILURE);
        }
    }
}

void handler_SIGINT (int sig) {
    printf("\n");
    if (pid_fg > -1) {

        if (kill(pid_fg, SIGKILL) == -1) {
            perror("SIGINT");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[], char *arge[]) {

    bool boucle = true;

    struct sigaction sigtstp, old_stp;
    sigtstp.sa_handler = handler_SIGTSTP;
    sigemptyset (&sigtstp.sa_mask);
    sigtstp.sa_flags = 0;

    if (sigaction(SIGTSTP, &sigtstp, &old_stp) == - 1) {  // On traite le signal SIGTSTP
        perror("sigaction TSTP");
        exit(EXIT_FAILURE);
    }

    struct sigaction sigint, old_int;
    sigint.sa_handler = handler_SIGINT;
    sigemptyset (&sigint.sa_mask);
    sigint.sa_flags = 0;

    if (sigaction(SIGINT, &sigint, &old_int) == - 1) {  // On traite le signal SIGINT
        perror("sigaction INT");
        exit(EXIT_FAILURE);
    }

    struct sigaction new_action, old_action;
    new_action.sa_handler = suivi_fils;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    char cwd[128];                              /* Arborescence */

    while(boucle) {
        
        printf("%s$DG-minishell$ ", getcwd(cwd, 128));

        struct cmdline *commande = readcmd();
        pid_t pidFils;

        if (commande != NULL) {
            if (commande->seq == NULL) {                /* Ligne vide */
                printf("\n");

            } else if (*commande->seq[0][0] == EOF) {
                boucle = false;

            } else if (strcmp(commande->seq[0][0], "exit") == 0) {
                exit(EXIT_SUCCESS);

            } else if (strcmp(commande->seq[0][0], "cd") == 0) {
                chdir(commande->seq[0][1]);

            } else if (strcmp(commande->seq[0][0], "jobs") == 0) {
                int i;
                for (i = 0; i < MAX_PROCESSUS; i++) {
                    if (listeP[i] != NULL) {
                        printf("Identifiant : %d | ", listeP[i]->ident);
                        printf("PID : %d | ", listeP[i]->pid);
                        printf("Etat : %d | ", listeP[i]->etat);
                        printf("Commande : ");
                        printf("%s\n", listeP[i]->commande_lancee);
                    }
                }

            } else if (strcmp(commande->seq[0][0], "stop") == 0) {
                int id = *commande->seq[0][1] - '0';
                
                if (kill(listeP[id]->pid, SIGSTOP) == -1) {
                    perror("SIGSTOP");
                    exit(EXIT_FAILURE);
                }
                listeP[id]->etat = SUSPENDU;

            } else if (strcmp(commande->seq[0][0], "bg") == 0) {
                int id = *commande->seq[0][1] - '0';
                kill(listeP[id]->pid, SIGCONT);

            } else if (strcmp(commande->seq[0][0], "fg") == 0) {
                int id = *commande->seq[0][1] - '0';
                pid_fg = listeP[id]->pid;
                id_fg = id;

                if (listeP[id]->etat == SUSPENDU) {
                    kill(listeP[id]->pid, SIGCONT);
                }

                waitpid(pid_fg, NULL, WUNTRACED);
                pid_fg = -1;

                if ((listeP[id]->etat == 1) && retirerProcessus(listeP[id]->pid) == -1) {
                    perror("pidFils non trouvé - fg");
                    exit(EXIT_FAILURE);
                }

            } else {
                pidFils = fork();

                if (pidFils == -1) {
                    printf("Erreur fork\n");
                    exit(1);

                } else if (pidFils == 0) {		            /* fils */
                    execvp(commande->seq[0][0], commande->seq[0]);
                    exit(pidFils);  //Si erreur execvp

                } else {                                    /* père */
                    if (sigaction(SIGCHLD, &new_action, &old_action) == - 1) {  // On traite le signal SIGCHLD
                        perror("sigaction");
                        exit(EXIT_FAILURE);
                    }
                    int ident = ajoutProcessus(pidFils, commande);
                    if (commande->backgrounded == NULL) {
                        pid_fg = pidFils;
                        id_fg = ident;
                        int status;
                        waitpid(pidFils, &status, WUNTRACED);
                        pid_fg = -1;

                        if ((listeP[ident]->etat == 1) && (retirerProcessus(pidFils) == -1)) {
                            perror("pidFils non trouvé");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        } else {
            printf("\n");
        }
    }
}
