#include "include/server.h"
#include "include/server_comm.h"
#include "include/server_utils.h"

void * receive_data(void * arg) {

    struct sockaddr_in adr_client; //adresse du client
    socklen_t longadr;
    int nbcar;
    static char buff_recv[MAXOCTETS+1]; // Buffer de réception d'octets
  
    pthread_t tid;
    request_t_data * handle_request_data;

    longadr=sizeof(adr_client);

    while (1) {
      // Remise à zéro
      memset(&adr_client,0,sizeof(struct sockaddr_in));
      buff_recv[0] = '\0';

      // Nouvelle réception
      nbcar=recvfrom(sd, buff_recv,MAXOCTETS+1,0,(struct sockaddr *) &adr_client,&longadr);
      CHECK_ERROR(nbcar,0,"\nSERVEUR - Problème lors de la réception\n");
      
      //Copie des données dans une nouvelle structure à passer au thread
      handle_request_data = (request_t_data *) malloc(sizeof(request_t_data));
      handle_request_data->p_adr_client = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
      handle_request_data->request = (char *)malloc(sizeof(char)*(MAXOCTETS+1));
      
      memcpy(handle_request_data->p_adr_client,&adr_client,sizeof(struct sockaddr_in));
      strcpy(handle_request_data->request,buff_recv);
      
      //Création du thread pour gérer la requête
      pthread_create(&tid, NULL, handle_request,(void *)handle_request_data);
    }
}

void * send_data(char data[MAXOCTETS+1], struct sockaddr_in adr_client) {
    int nbcar;
    nbcar=sendto(sd, data,strlen(data) + 1,0,(const struct sockaddr *) &adr_client,sizeof(adr_client));
    CHECK_ERROR(nbcar,0,"\nErreur lors de l'émission des données");
    return EXIT_SUCCESS;
}

void * handle_request(void * arg) {
    int code;
    int resp_code = 0;
    int car_id;
    char *ptr;
    char ip_client[MAXOCTETS+1];
    char current_ip[MAXOCTETS+1];
    char resp[MAXOCTETS+1];
    int i;
    int id_ressource;
    request_t_data * data = (request_t_data *) arg;

    printf("Received request from %s: %s\n", ip_client, data->request);
    
    // Récupération de l'adresse IP + port de la voiture émittrice de la requête
    ip_client[0] = '\0';
    sprintf(ip_client, "%s:%d", inet_ntoa(data->p_adr_client->sin_addr), ntohs(data->p_adr_client->sin_port));
  
    ptr = strtok(data->request, ":");

    // Analyse du code de requête
    code = atoi(ptr);
    
    if (code == 101) { // 101 : Demande d'enregistrement
        i = 0;
        while (i < MAXVOITURES && cars_list[i] != NULL) { // On cherche le prochain emplacement de voiture libre

            current_ip[0] = '\0';
            sprintf(current_ip, "%s:%d", inet_ntoa(cars_list[i]->addr.sin_addr), ntohs(cars_list[i]->addr.sin_port));

            if (!strcmp(ip_client, current_ip)) { // Si à un moment, on trouve la même ip, alors la voiture est déjà enregistrée
                resp_code = 401;
                break;
            }
            else i++;
        }
        if (i == MAXVOITURES) { // Si tous les emplacements de voiture sont déjà attribués, renvoi d'erreur
            resp_code = 402;
        } else if (resp_code != 401) { // Si on a trouvé un emplacement libre
            cars_list[i] = (struct car *)malloc(sizeof(car));
            memcpy(&(cars_list[i]->addr), (struct sockaddr*)data->p_adr_client, sizeof(struct sockaddr_in)); // On copie l'adresse
            cars_list[i]->pos_x = 0;
            cars_list[i]->pos_y = 0;
            cars_list[i]->pos_z = 0;
            cars_list[i]->mission_required = 0;
            resp_code = 201;
        }
        itoa(resp_code, resp);
    } 
    else { // Autres demandes : la voiture doit être enregistrée
        car_id = get_car_id(ip_client); // On récupère la position dans l'array des voitures de la voiture correspondant à cette adresse
        if (car_id == -1) { // On vérifie si la voiture est enregistrée 
            resp_code = 403;
            itoa(resp_code, resp);
        } else {
            switch(code) {
                case 102: // 102 : Réception d'un rapport de position
                    resp_code = 202;

                    ptr = strtok(NULL, ":"); // On parse la pos X
                    if (strlen(ptr)==0) resp_code = 404;
                    else {
                        cars_list[car_id]->pos_x = atoi(ptr);

                        ptr = strtok(NULL, ":"); // On parse la pos Y
                        if (strlen(ptr)==0) resp_code = 404;
                        else {
                          cars_list[car_id]->pos_y = atoi(ptr);

                          ptr = strtok(NULL, ":"); // On parse la pos Z
                          if (strlen(ptr)==0) resp_code = 404;
                          else cars_list[car_id]->pos_z = atoi(ptr);
                        }
                    }

                    itoa(resp_code, resp);
                    break;

                case 103: // 103 : Demande d'accès à une ressource
                    ptr = strtok(NULL, ":");
                    if (strlen(ptr)==0) resp_code = 500;
                    else {
                        id_ressource = atoi(ptr);
                        if (id_ressource < 0 || id_ressource >= NBRESSOURCES) resp_code = 500; // On vérifie que l'ID demandée existe
                        else {
                            if (ressources_list[id_ressource]->reserved) { // On vérifie si la ressource est déjà reservée 
                                if (ressources_list[id_ressource]->car_id == car_id) resp_code = 407; // Si c'est le cas, on vérifie aussi si c'est la même voiture
                                else resp_code = 405;
                            } else {
                                resp_code = 203; // On réserve la ressource
                                ressources_list[id_ressource]->reserved = 1;
                                ressources_list[id_ressource]->car_id = car_id;
                            }
                        }
                    }
                    itoa(resp_code, resp);
                    sprintf(resp, ":%d", id_ressource);
                    break;

                case 104: // 104 : Demande de libération  d'une ressource
                    ptr = strtok(NULL, ":");
                    if (strlen(ptr)==0) resp_code = 500;
                    else {
                        id_ressource = atoi(ptr);
                        if (id_ressource < 0 || id_ressource >= NBRESSOURCES) resp_code = 500; // On vérifie que l'ID demandée existe
                        else {
                            if (ressources_list[id_ressource]->reserved && ressources_list[id_ressource]->car_id == car_id) { // On vérifie que la ressource est réservée par cette voiture
                                resp_code = 204;
                                ressources_list[id_ressource]->reserved = 0;
                                ressources_list[id_ressource]->car_id = -1;
                            } else resp_code = 406;
                        }
                    }
                    itoa(resp_code, resp);
                    sprintf(resp, ":%d", id_ressource);
                    break;
                case 106: // 105 : Demande d'envoi de mission
                
                    cars_list[car_id]->mission_required = 1;
                    resp_code = 206;
                    
                    itoa(resp_code, resp);
                    break;
                case 205:
                    break;
                default: // Autres cas : mauvais requête
                    resp_code = 500;
                    itoa(resp_code, resp);
                    break;
            }
        }
    }
  
  // On envoie la réponse
  if (resp_code != 0) send_data(resp, *data->p_adr_client);
  pthread_exit(NULL);
}
