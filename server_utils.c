#include "include/server.h"
#include "include/server_comm.h"
#include "include/server_utils.h"

void itoa(int val, char * dest) {
    dest[0] = '\0';
    sprintf(dest, "%d", val);
}

int get_car_id(char * ip_client) {
    int car_id = -1;
    int i = 0;
    
    while (i < MAXVOITURES && cars_list[i] != NULL) {
        if (!strcmp(inet_ntoa(cars_list[i]->addr.sin_addr), ip_client))  {
          car_id = i;
          break;
        } else i++;
    }

    return car_id;
    }