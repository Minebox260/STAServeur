/******************************************************************************/
/* APPLICATION : Serveur Mini Projet Voiture Autonome STA 2023                */
/* Date : 19/10/2023                                                          */
/* Version : 1.0                                                              */
/* Compilation : gcc Serveur.c -o Serveur                                     */
/* usage : ./Serveur <port serveur>                                           */
/******************************************************************************/

#include "include/server.h"
#include "include/server_comm.h"
#include "include/server_utils.h"

struct car *cars_list[MAXVOITURES] = {NULL};
struct ressource *ressources_list[NBRESSOURCES] = {NULL};

int sd;

char * server_adr_str;
int server_port;

int main(int argc, char *argv[]) {
  if (argc != 2)
    printf("SERVEUR | Utilisation : ./serveur <port serveur> \n");
  else {
    struct sockaddr_in server_adr; // Adresse du serveur
    pthread_t *tids;
    int erreur; // Gestion des erreurs
    char hostbuffer[MAXOCTETS];
    struct hostent *host_entry;

    // Récupération de l'adresse IP de la machine
    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer));
    strcat(hostbuffer, ".local");
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    server_adr_str = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));


    printf("SERVEUR - Initialisation\n");

    printf("SERVEUR - Initialisation des ressources\n");
    for (int i = 0; i < NBRESSOURCES; i++) {
      ressources_list[i] = (struct ressource *)malloc(sizeof(struct ressource));
      ressources_list[i]->car_id = -1;
      ressources_list[i]->reserved = 0;
    }

    printf("SERVEUR - Initialisation du socket de dialogue\n");

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    printf("SERVEUR - N° du socket dialogue : %d \n", sd);

    server_port = atoi(argv[1]);
    // Préparation de l'adresse du serveur
    server_adr.sin_family = AF_INET;
    server_adr.sin_port = htons(server_port);
    server_adr.sin_addr.s_addr = inet_addr(server_adr_str);

    // Affectation de l'adresse au socket
    erreur = bind(sd, (const struct sockaddr *)&server_adr, sizeof(server_adr));
    CHECK_ERROR(erreur, -1,
                "SERVEUR - Erreur lors du bind du socket de dialogue\n");

    printf("SERVEUR - Initialisation des threads\n");

    tids = (pthread_t *)malloc(2 * sizeof(pthread_t));
    pthread_create(&tids[0], NULL, receive_data, NULL);
    pthread_create(&tids[1], NULL, user_menu, NULL);

    pthread_join(tids[0], NULL);
    pthread_join(tids[1], NULL);

    printf("Arrêt du serveur");

    close(sd);
  }
  exit(EXIT_SUCCESS);
}

void *user_menu(void *t_data) {
  int i;
  char menu_input[MAXOCTETS + 1];

  sleep(1);
  while (1) {
    system("clear");
    printf("\033[0;37m"
           "//////////////////////////////////\n"
           "//                              //\n"
           "//  SERVEUR VOITURES AUTONOMES  //\n"
           "//                              //\n"
           "//////////////////////////////////\n\n"
           "IP : \033[0;32m %s:%d\033[0;37m\n\n", server_adr_str, server_port);

    printf("//      VOITURES CONNECTEES     //\n\n");
    for (i = 0; i < MAXVOITURES; i++) {
      if (cars_list[i] != NULL) {
        printf("[\033[0;32m%d\033[0;37m] | IP : %s:%d | X : %d | Y : %d | Z : %d", i,
               inet_ntoa(cars_list[i]->addr.sin_addr), ntohs(cars_list[i]->addr.sin_port), cars_list[i]->pos_x,
               cars_list[i]->pos_y, cars_list[i]->pos_z);
        if (cars_list[i]->mission_required) printf(" \033[0;31m(AUCUNE MISSION)\033[0;37m");
        printf("\n");
      } else
        printf("[\033[0;31m-\033[0;37m] Non connectée\n");
    }
    printf("\n\n");
    printf("//            RESSOURCES           //\n\n");
    for (i = 0; i < NBRESSOURCES; i++) {
      if (!ressources_list[i]->reserved) {
        printf("[\033[0;32m%d\033[0;37m] | Reservée par : %d (%s:%s)", i,
              ressources_list[i]->car_id,
               inet_ntoa(cars_list[ressources_list[i]->car_id]->addr.sin_addr), ntohs(cars_list[ressources_list[i]->car_id]->addr.sin_port));
        printf("\n");
      } else
        printf("[\033[0;31m%d\033[0;37m] Non reservée\n", i);
    }
    printf("\n");

    printf("//             MENU             //\n\n");
    printf("SENDTO : Envoyer une commande de déplacement à une voiture\n");
    printf("QUIT : Eteindre le serveur\n\n");
    printf("SERVEUR > ");

    fgets(menu_input, MAXOCTETS + 1, stdin);
    menu_input[strlen(menu_input) - 1] = '\0';
    handle_input(menu_input);
    memset(menu_input, 0, MAXOCTETS + 1);
  }
}

void handle_input(char *menu_input) {

  int car_id;
  char buffer[MAXOCTETS - 3];
  char send[MAXOCTETS + 1];
  int X,Y;
  
  if (!strcmp(menu_input, "SENDTO")) {

    printf("\nQuelle voiture souhaitez vous commander ?\nSERVEUR > ");
    fgets(buffer, MAXOCTETS - 3, stdin);
    car_id = atoi(buffer);

    if (car_id >= 0 && car_id < MAXVOITURES && cars_list[car_id] != NULL) {

      printf("\nPosition X de destination ?\nSERVEUR > ");
      fgets(buffer, MAXOCTETS - 3, stdin);
      X = atoi(buffer);

      buffer[0] = '\0';
      printf("\nPosition Y de destination ?\nSERVEUR > ");
      fgets(buffer, MAXOCTETS - 3, stdin);
      Y = atoi(buffer);
      
      sprintf(send, "%d:%d:%d", 105, X, Y);
      send_data(send, cars_list[car_id]->addr);
      cars_list[car_id]->mission_required = 0;

    } else
      printf("\nID Invalide\n");

  } else if (!strcmp(menu_input, "QUIT"))
    exit(EXIT_SUCCESS);
}