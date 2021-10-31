#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <json-c/json.h>
#include "server.h"

#define MAX_PENDING_CONNECTIONS (5)

int socketEcoute, socketDonnees;
int bytes_read;
char buffer[1024] = { 0 };
static bool dataToReceive = true;

static void writeUserDB(char * buffer);
static void writeTourDB(void);

extern void Server_start(void)
{
    struct sockaddr_rc loc_addr, rem_addr; // RFCOMM socket address
    socklen_t opt = sizeof(rem_addr);

    // allocate socket
    socketEcoute = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // bind socket to port 1 of the first available 
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    loc_addr.rc_bdaddr = *BDADDR_ANY;
    loc_addr.rc_channel = (uint8_t) 1;
    bind(socketEcoute, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

    // put socket into listening mode
    listen(socketEcoute, MAX_PENDING_CONNECTIONS);

    // accept one connection
    socketDonnees = accept(socketEcoute, (struct sockaddr *)&rem_addr, &opt);

    ba2str( &rem_addr.rc_bdaddr, buffer );
    fprintf(stderr, "accepted connection from %s\n", buffer);
    memset(buffer, 0, sizeof(buffer));

    while(dataToReceive)
    {
        Server_readMsg();
    }
}

static void writeUserDB(char * buffer)
{
    const char * filename = "./baseAbonnes.json";
    struct json_object * baseAbonnes;
    baseAbonnes = json_tokener_parse(buffer);
    json_object_to_file_ext(filename, baseAbonnes, JSON_C_TO_STRING_PRETTY);
    writeTourDB();
}

static void writeTourDB()
{
    const char * filename = "./baseTournee.json";

    struct json_object * baseTournee;
    struct json_object * benne1, * benne2;

    baseTournee = json_object_new_object();
    benne1 = json_object_new_object();
    benne2 = json_object_new_object();
    
    json_object_object_add(baseTournee, "benne1", benne1);
    json_object_object_add(benne1, "id", json_object_new_string("1F20E412"));
    json_object_object_add(benne1, "weight", json_object_new_int(30));

    json_object_object_add(baseTournee, "benne2", benne2);
    json_object_object_add(benne2, "id", json_object_new_string("40AA9C02"));
    json_object_object_add(benne2, "weight", json_object_new_int(48));

    // json_object_object_get_ex(object_json, "name", &name); // to get the value of a key
    
    json_object_to_file_ext(filename, baseTournee, JSON_C_TO_STRING_PRETTY);
    
    // json_object_put(parsed_json); // to free a json object
}

extern void Server_sendMsg()
{

}

extern void Server_readMsg()
{
    while(dataToReceive)
    {
        // read data from the client
        int ret = read(socketDonnees, buffer, sizeof(buffer));
        if(ret <= 0)
        {
            dataToReceive = false;
        }
        else
        {
            writeUserDB(buffer);
            //printf("received [%s]\n", buf);
        }
    }
    
}

extern void Server_stop()
{
    // close connection
    close(socketDonnees);
    close(socketEcoute);
}