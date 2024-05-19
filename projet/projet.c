#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h> // strlen strncmp
#include <stdlib.h> // system
#include <stdbool.h> // _Bool true false
#include <ctype.h> // tolower
#include <sys/wait.h> // wait


const int MAX_TAB_LENGTH = 256;


int main(int argc, char *argv[]) {
    int countCmd = 1; // la premiere commande est recu via argv

    // recuperer et afficher les commandes
    printf("%i\n", argc); // le nombre d'arguments passés le premier est le nom du programme
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]); // afficher les arguments
    }

    // ecrire la commande passé dans argv dans un fichier
    
    int fd;
    size_t bytesNbr, rd, wr; // nombre de bytes dans argv, nombre des bytes lu, nombre de bytes écrit
    fd = open("commandes.txt", O_RDWR | __O_DSYNC | O_APPEND); // ouverture en ecriture; fd : file descriptor 

    for (int i = 1; i < argc; i++) {
        bytesNbr = strlen(argv[i]);
        wr = write(fd, argv[i], bytesNbr);
        if (i + 1 < argc) {
            wr = write(fd, " ", 1);
        } else {
            wr = write(fd, "\n", 1);
        }
    }

    
    // demander si l'utilisateur veut entrer des nouvelles commandes

    puts("voulez vous continuer [y] : ");
    char c;
    scanf("%c", &c);
    //printf("le char entré est : %c\n", c);

    if (c == 'y') {
        
        char buf[MAX_TAB_LENGTH];
        _Bool ext = true;
        //printf("vous avez écrit %s", buf);
        do
        {
            printf("entrez une commande ou exit pour quitter : \n");
            
            rd = read(0, buf, MAX_TAB_LENGTH);
            wr = write(fd, buf, rd);
            countCmd += 1; // incriment du nombre de commandes
            if (strncmp(buf, "exit", 4) == 0) ext = false;
            if (wr <= 0) {
            printf("%ld bytes écrits\n", wr);
            } else if (wr > 0) {
                printf("%ld bytes écrits\n", wr);
            }
        } while (ext);
    
    } else {
        printf("au revoir\n");
    }
    //close(fd); // fermeture du fichier

    // PARTIE 2
    struct stat fbuf_status; // la structure contenant le status du fichier
    int fs = fstat(fd, &fbuf_status); 
    /* 
    if (fs == -1) {
        perror("fstat error : ");
    } */
    size_t f_size = fbuf_status.st_size; // la taille du fichier en bytes
    //printf("la taille du fichier est %ld\n", f_size);
    char *buf = malloc(sizeof(char[f_size+1])); // alloue de la memoire pour un tableau de char de la taille du fichier
    lseek(fd, 0, 0); // avant la lecture, se positionner au debut du fichier
    rd = read(fd, buf, f_size); // lis f_size bytes dans buf
    //printf("la taille de buf est %ld\n", strlen(buf));
    if (rd == 0) {
        perror("erreur read EOF");
    } else if (rd == -1) {
        perror("erreur read ");
    }
    //printf("%s", buf);
    // envoyer la commande à exec et l'executer
    char *sub_str;
    char **commandes = malloc(f_size*sizeof(char*));
    int idx = 0;
    sub_str = strtok(buf, "\n"); // recuperer une commande et ses paramètres
    
    while (sub_str)
    {
        commandes[idx] = sub_str;
        idx += 1;
        sub_str=strtok(NULL, "\n");
    }
    // faut ouvrir le named pipe fifo en lecture 
    int pdes[2];
    int pipe_ret = pipe(pdes);
    if (pipe_ret == -1) {
        perror("error pipe ");
    }

    for (int i = 0; i < idx; i++) {
        pid_t pid = fork();
        if (pid == 0)
        {   
            // ovrir le named pipe en écriture
            //printf("idx : %d", idx);
            // executer les commandes l'une après l'autre
            char *cmd[4] = {"/bin/sh", "-c"}; // la commande à executer
            int ret;
            //printf("la commande : %d contenu : %s\n",i ,commandes[i]);
            cmd[2] = commandes[i]; // command_string à executer 
            //printf("apres le cmd\n");
            close(pdes[0]); // fermer lecture pipe
            dup2(pdes[1], STDOUT_FILENO); // rediriger le resultat (écrire) dans le pipe
            close(pdes[1]);// fermer ecriture du pipe
            ret = execvp(cmd[0], cmd); // execution de la commande 
            
            if (ret == -1) {
                perror("erreur execv");
            }
            
            free(commandes);
            free(buf); // desalouer la memoire pour le buf
        } else {
            // retour dans le proccess parent
            // afficher le resultat depuis le pipe
            close(pdes[1]); // fermer l'ecriture du pipe
            struct stat st;
            fstat(pdes[0], &st);
            //printf("la taille du pipe %ld\n", st.st_size);
            char *buffer = malloc(st.st_size*sizeof(char));
            
            int r = read(pdes[0], buffer, st.st_size);
            if (r == -1) perror("read dans parent ");
            //printf("buffer parent %s\n", buffer);
            int w = write(STDOUT_FILENO, buffer, st.st_size);
            if (w == -1) perror("error write dans parent ");
            close(pdes[0]); // ferme lecture du pipe

            free(buffer);

            //wait(NULL); // attend la fin du processus enfant
            //exit(EXIT_SUCCESS);
            //printf("retour dans le process parent\n");
        }

    }


    close(fd); // fermer le fichier
    return 0;

}