/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2024
 * Marc-André Gardner
 * 
 * Fichier principal
 ******************************************************************************/

#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "utils.h"
#include "emulateurClavier.h"
#include "tamponCirculaire.h"

// static void test(int foo, char *buff);


static void* threadFonctionClavier(void* args){
    // Implementez ici votre fonction de thread pour l'ecriture sur le bus USB
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadClavier *infos = (struct infoThreadClavier *)args;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    pthread_barrier_wait(infos->barriere);
    
    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Tenter d'obtenir une requete depuis le tampon circulaire avec consommerDonnee()
    // 2) S'il n'y en a pas, attendre un cours laps de temps (par exemple usleep(500))
    // 3) S'il y en a une, appeler ecrireCaracteres avec les informations requises
    // 4) Liberer la memoire du champ data de la requete avec la fonction free(), puisque
    //      la requete est maintenant terminee


    while(1){
        
        struct requete req = {0};
        
        if (consommerDonnee(&req)) {
            ecrireCaracteres(infos->pointeurClavier, req.data, req.taille, infos->tempsTraitementParCaractereMicroSecondes);
            free(req.data);
        }
        else {
            usleep(500);
        }
    }
    return NULL;
}

static void* threadFonctionLecture(void *args){

    // Implementez ici votre fonction de thread pour la lecture sur le named pipe
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)

    struct infoThreadLecture *infos = (struct infoThreadLecture *)args;
    
    // Ces champs vous seront utiles pour l'appel a select()
    fd_set setFd;
    int nfds = infos->pipeFd + 1;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    pthread_barrier_wait(infos->barriere);

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Remplir setFd en utilisant FD_ZERO et FD_SET correctement, pour faire en sorte
    //      d'attendre sur infos->pipeFd
    // 2) Appeler select(), sans timeout, avec setFd comme argument de lecture (on veut bien
    //      lire sur le pipe)
    // 3) Lire les valeurs sur le named pipe
    // 4) Si une de ses valeurs est le caracteres ASCII EOT (0x4), alors c'est la fin d'un
    //      message. Vous creez alors une nouvelle requete et utilisez insererDonnee() pour
    //      l'inserer dans le tampon circulaire. Notez que le caractere EOT ne doit PAS se
    //      retrouver dans le champ data de la requete! N'oubliez pas egalement de donner
    //      la bonne valeur aux champs taille et tempsReception.

    int idx;
    int charRead;
    char EOT;
    charRead = 64;
    char* buffer = malloc(charRead);
    struct requete req = {0};

    EOT = 4;

    while(1){
        FD_ZERO(&setFd);
        FD_SET(infos->pipeFd, &setFd);

        if (select(nfds, &setFd, NULL, NULL, NULL) < 0) {
            perror("woupsidou!");
        }

        idx = 0;
        int flag = 1;
        while(flag != 0) {
            idx += read(infos->pipeFd, buffer + idx, 1);
            flag = memcmp(&buffer[idx - 1], &EOT, 1);
            if (idx >= charRead) {
                charRead *= 2;
                if  (realloc(buffer, charRead * sizeof(char)) == NULL) {
                    perror("realloc error");
                }
            }
        }

        char *localBuff = malloc(charRead - 1);
        memcpy(localBuff, buffer, charRead - 1);

        buffer[idx - 1] = '\0';

        req.data = localBuff;
        req.taille = idx - 1;
        req.tempsReception = get_time();

        insererDonnee(&req);
    }
    return NULL;
}

// static void test(int foo, char *buff) {
//     printf("%i", foo);
// }

int main(int argc, char* argv[]){
    if(argc < 4){
        printf("Pas assez d'arguments! Attendu : ./emulateurClavier cheminPipe tempsAttenteParPaquet tailleTamponCirculaire\n");
    }

    // A ce stade, vous pouvez consider que:
    // argv[1] contient un chemin valide vers un named pipe
    // argv[2] contient un entier valide (que vous pouvez convertir avec atoi()) representant le nombre de microsecondes a
    //      attendre a chaque envoi de paquet
    // argv[3] contient un entier valide (que vous pouvez convertir avec atoi()) contenant la taille voulue pour le tampon
    //      circulaire

    // Vous avez plusieurs taches d'initialisation a faire :
    //
    // 1) Ouvrir le named pipe

    const int THREAD_NB = 2;

    char* path = argv[1];
    int waitTime = atoi(argv[2]);
    int bufferSize = atoi(argv[3]);

    int fd;
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open error");
        return -1;
    }

    // 2) Declarer et initialiser la barriere
    
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, THREAD_NB);

    // 3) Initialiser le tampon circulaire avec la bonne taille

    initTamponCirculaire(bufferSize);

    // 4) Creer et lancer les threads clavier et lecteur, en leur passant les bons arguments dans leur struct de configuration respective

    pthread_t keybThread;
    pthread_t readThread;

    struct infoThreadClavier keybInfos = { 0 };
    struct infoThreadLecture readInfos = { 0 };
    
    keybInfos.pointeurClavier = initClavier();
    keybInfos.tempsTraitementParCaractereMicroSecondes = waitTime;
    keybInfos.barriere = &barrier;

    readInfos.pipeFd = fd;
    readInfos.barriere = &barrier;

    pthread_create(&keybThread, NULL, threadFonctionClavier, &keybInfos);
    pthread_create(&readThread, NULL, threadFonctionLecture, &readInfos);

    // La boucle de traitement est deja implementee pour vous. Toutefois, si vous voulez eviter l'affichage des statistiques
    // (qui efface le terminal a chaque fois), vous pouvez commenter la ligne afficherStats().
    struct statistiques stats;
    double tempsDebut = get_time();
    while(1){
        // Affichage des statistiques toutes les 2 secondes
        calculeStats(&stats);
        afficherStats((unsigned int)(round(get_time() - tempsDebut)), &stats);
        resetStats();
        usleep(2e6);
    }
    return 0;
}
