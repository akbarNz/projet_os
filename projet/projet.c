/* 
    NOM DU PROJET: PROJET OS 1T JUIN 2024

    GROUPE/CLASSE/ELEVES : GROUPE 25 CLASSE 1TM2 NZEYIMANA AKBAR, MOHAMED EL HOSNI, MAXIME CHRISPEELS

    DESCRIPTION ET CONCLUSION : ce projet nous a permis de mettre en pratique les notions vu 
    au cours théorique notamment le fonctionnement des fichiers et leurs implémentations subjacentes et 
    la communication entre processus. Tout au long de ce projet, nous nous sommes familiarisés avec les concepts du langage C,
    comme l'allocation dynamique de la mémoire, l'utilisation des librairies permettant de faire divers choses. Pour finir,
    le projet nous a permis d'élaborer une méthode de travail cohérente et ordonnées qui consiste au découpage du projet en plusieurs petites 
    "mini-projet" permettant de se partager les taches et implémenter les bout des codes lorsqu'on est sûr de leur bon fonctionnement.

 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h> // strlen strncmp
#include <stdlib.h> // system
#include <stdbool.h> // _Bool true false
#include <ctype.h> // tolower

const int MAX_TAB_LENGTH = 256;


int main(int argc, char *argv[]) {
    int countCmd = 1; // la premiere commande est reçu via argv

    // recuperer depuis argv et afficher la commande 
    /* for (int i = 1; i < argc; i++) {
        printf("%s\n", argv[i]); // afficher les arguments
    } */

    // ecrire la commande passé dans argv dans un fichier
    int fd;
    size_t bytesNbr, rd, wr; // nombre de bytes dans argv, nombre des bytes lu, nombre de bytes écrit
    fd = open("commandes.txt", O_RDWR | __O_DSYNC | O_APPEND); // ouverture en ecriture; fd : file descriptor 

    for (int i = 1; i < argc; i++) {
        bytesNbr = strlen(argv[i]);
        wr = write(fd, argv[i], bytesNbr);
        if (i + 1 < argc) {
            wr = write(fd, " ", 1); // separateur entre la commande et ses arguments
        } else {
            wr = write(fd, "\n", 1); // separateur des commandes
        }
    }

    
    // demander si l'utilisateur veut entrer des nouvelles commandes
    puts("voulez vous continuer [y/n] : ");
    char c;
    scanf("%c", &c);

    if (c == 'y') {
        
        char buf[MAX_TAB_LENGTH];
        _Bool ext = true;

        do
        {
            printf("entrez une commande ou exit pour quitter : \n");
            
            rd = read(0, buf, MAX_TAB_LENGTH);
            wr = write(fd, buf, rd);
            countCmd += 1; // incriment du nombre de commandes

            // verifie si l'utilisateur a entré exit
            if (strncmp(buf, "exit", 4) == 0) ext = false;

        } while (ext);
    
    } else {
        printf("-------exit le prompteur-------\n");
    }


    // PARTIE 2
    printf("-------éxécution-------\n");

    struct stat fbuf_status; // la structure contenant les stats du fichier
    int fs = fstat(fd, &fbuf_status); // obtenir les stats du fichier
    /* 
    if (fs == -1) {
        perror("fstat error : ");
    } */

    size_t f_size = fbuf_status.st_size; // la taille du fichier en bytes
    char *buf = malloc(sizeof(char[f_size+1])); // alloue de la memoire pour un tableau de char de la taille du fichier
    lseek(fd, 0, 0); // avant la lecture, se positionner au debut du fichier
    rd = read(fd, buf, f_size); // lis f_size bytes dans buf

    //verification des erreurs
    if (rd == 0) {
        perror("erreur read EOF");
    } else if (rd == -1) {
        perror("erreur read ");
    }
    
    // envoyer la commande à exec et l'executer
    char *sub_str; // la string d'une commande
    char **commandes = malloc(f_size*sizeof(char*)); // un tableau qui contient les commandes 
    int idx = 0; 
    sub_str = strtok(buf, "\n"); // recuperer une commande et ses paramètres
    
    // recuperer les commandes restant
    while (sub_str)
    {
        commandes[idx] = sub_str;
        idx += 1;
        sub_str=strtok(NULL, "\n");
    }

    // creer le pipe où écrire et lire. les resultats des commandes éxécutés sont rédirigé dans le pipe
    int pdes[2];
    int pipe_ret = pipe(pdes); // création du pipe
    if (pipe_ret == -1) {
        perror("error pipe ");
    }

    // forker le processus et éxécuter la commande
    for (int i = 0; i < idx; i++) {
        pid_t pid = fork();
        if (pid == 0)
        {   
            //processus enfant

            char *cmd[4] = {"/bin/sh", "-c"}; // la commande à executer
            cmd[2] = commandes[i]; // command_string à executer 
            close(pdes[0]); // fermer le pipe en lecture
            dup2(pdes[1], STDOUT_FILENO); // rediriger le resultat (écrire) dans le pipe au lieu de la sortie standard
            
            int ret = execvp(cmd[0], cmd); // execution de la commande 
        
            if (ret == -1) {
                perror("erreur execv");
            }

            close(pdes[1]);// fermer le pipe en ecriture
            
            free(commandes); // desalouer la memoire
        } else {
            // retour dans le proccess parent
            // afficher le resultat depuis le pipe
            close(pdes[1]); // fermer le pipe en ecriture
            char buffer;
            while (read(pdes[0], &buffer, 1) > 0)
            {
                write(STDOUT_FILENO, &buffer, 1);
            }
            
            write(STDOUT_FILENO, "\n", 1);

            close(pdes[0]); // ferme lecture du pipe
        }

    }

    close(fd); // fermer le fichier
    return 0;

}