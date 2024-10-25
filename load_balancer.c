
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "server.h"
#include "hashtable.h"
#include "utils.h"
#define NR 100000
#define NRH 3
#define NRS 1

struct load_balancer {
    // vectorul in care retinem fiecare server si cele doua replici
    int *hashring;
    // vector de pointeri catre serverele date (fara replici)
    server_memory **server;
    // numarul total de elemente din hashring
    int nr_total;
    // numarul total de servere
    int nr_server;
};

// functia care returneaza hash-ul unei etichete
unsigned int hash_function_servers(void *a) {
    unsigned int uint_a = *((unsigned int *)a);

    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

// functia care returneaza hash-ul unei chei
unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *)a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

// functia de initializare a unui load_balancer
load_balancer *init_load_balancer() {
    load_balancer *load_balancer = malloc(sizeof(*load_balancer));

    // initial vom aloca un numar de trei elemente pentru hashring
    // trei elemente => serverul principal si cele doua replici

    load_balancer->hashring = malloc(NRH * sizeof(int));
    DIE(!load_balancer->hashring, "Malloc failed.\n");

    // initial vom aloca memorie pentru un singur server
    load_balancer->server = malloc(NRS * sizeof(struct server_memory*));
    DIE(!load_balancer->server, "Malloc failed.\n");

    load_balancer->nr_total = 0;
    load_balancer->nr_server = 0;

    return load_balancer;
}

// functie care returneaza pozitia pe care se regaseste un server
// se cauta pozitia in vectorul de servere
int return_pos(load_balancer *main, int server_id)
{
    int pos_current = -1;

    for (int p = 0; p < main->nr_server; p++)
        if (main->server[p]->id == server_id)
            pos_current = p;

    return pos_current;
}

// stocheaza un obiect pe un server disponibil
void loader_store(load_balancer *main, char *key, char *value, int *server_id) {
    int pos_current, nr, var;
    unsigned int hash_current;

    hash_current = hash_function_key(key);

    var = 0;
    nr = main->nr_total;
    pos_current = -1;

    // parcurgem hashring-ul
    for (int i = 0; i < nr; i++) {
        int ticket = main->hashring[i];

        *server_id = ticket % NR;

        unsigned int hash_ticket = hash_function_servers(&ticket);

        // verificam unde trebuie adaugat obiectul
        if (hash_current < hash_ticket) {
            var = 1;
            // aflam pozitia serverului in care trebuie adugat obiectul
            pos_current = return_pos(main, *server_id);
            // adugam obiectul in serverul dorit
            server_store(main->server[pos_current], key, value);
            break;
        }
    }
    // obiectul nu a fost inca adaugat
    // adugam in serverul care corespunde primei etichete din hashring
    if (var == 0) {
        int ticket = main->hashring[0];
        *server_id = ticket % NR;

        pos_current = return_pos(main, *server_id);

        server_store(main->server[pos_current], key, value);
    }
}

// aflam pe ce server se afla cheia corespunzatoare si ii extrage valorea
char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
    int pos_current, var;
    unsigned int hash_current;

    hash_current = hash_function_key(key);

    pos_current = -1;
    var = 0;

    for (int i = 0; i < main->nr_total; i++) {
        int ticket = main->hashring[i];

        *server_id = ticket % NR;

        unsigned int hash_ticket = hash_function_servers(&ticket);

        if (hash_current < hash_ticket) {
            var = 1;
            // aflam pozitia corespunzatoare serverului in vectorul de servere
            pos_current = return_pos(main, *server_id);
            break;
        }
    }
    // se va lua serverul corespunzator etichetei de pe prima pozitie
    if (var == 0) {
        int ticket = main->hashring[0];
        *server_id = ticket % NR;
        pos_current = return_pos(main, *server_id);
    }

    return server_retrieve(main->server[pos_current], key);
}

// functie care muta toate serverele cu o pozitie
// functia se foloseste atunci cand se sterge un server din vectorul de servere
void move_element(load_balancer *main, int pos_current)
{
    for (int k = pos_current; k < main->nr_server - 1; k++)
        main->server[k] = main->server[k + 1];
}

// functie care muta toate elementele din hashring la stanga
void move_element_left(load_balancer *main, int pos)
{
    for (int i = pos; i < main->nr_total - 1; i++)
        main->hashring[i] = main->hashring[i + 1];
}

// functie care muta toate elementele din hashring la dreapta
void move_element_right(load_balancer *main, int i)
{
    for (int j = main->nr_total; j > i; j--)
        main->hashring[j] = main->hashring[j - 1];
}

// functie care adauga o eticheta in hashring
void put_hashring(load_balancer *main, int ticket)
{
    int nr = main->nr_total;
    // daca nu avem niciun element in vector, adugam pe prima pozitie
    if (nr == 0) {
        main->hashring[0] = ticket;
        main->nr_total++;
        return;
    }

    unsigned int hash = hash_function_servers(&ticket);

    // trebuie sa adugam etichetele in functie de hash
    // vectorul trebuie mentinut sortat
    for (int i = 0; i < nr; i++) {
        int label_in;
        unsigned int label_hash;
        // obtinem hash-ul primului element din hashring
        label_in = main->hashring[0];
        label_hash = hash_function_servers(&label_in);

        // cazul cand se adauga eticheta pe prima pozitie in hashring
        if (hash < label_hash) {
            move_element_right(main, i);
            main->hashring[0] = ticket;
            main->nr_total++;

            return;
        } else {
            unsigned int label_input_hash;

            int label_input = main->hashring[i];
            label_input_hash = hash_function_servers(&label_input);

            // sortare in functie de id
            if (hash == label_input_hash) {
                int current_id = ticket % NR;
                int id_cmp = label_input % NR;
                if (current_id > id_cmp) {
                    move_element_right(main, i);
                    main->hashring[i] = ticket;
                    main->nr_total++;
                } else {
                    move_element_right(main, i + 1);
                    main->hashring[i + 1] = ticket;
                    main->nr_total++;
                }
            }
            // verificam unde trebuie sa adugam eticheta
            if (hash < label_input_hash) {
                // mutam elementele cu o pozitie la dreapta
                move_element_right(main, i);

                // adugam eticheta in hashring
                main->hashring[i] = ticket;

                // crestem numarul de elemente din hashring
                main->nr_total++;

                return;
            }
        }
    }

    // eticheta nu a fost adugata nici pe prima pozitie, dar nici in interior
    // se va adauga pe ultima pozitie
    main->hashring[main->nr_total++] = ticket;
}

// functie care redistribuie elementele atunci cand se aduga un nou server
void map(load_balancer *main, int ticket)
{
    int aux_pos, pos, right, left, nr, pos_prev;
    int current_id, next_id;
    unsigned int hash_current;

    // calculam hash-ul etichetei date
    hash_current = hash_function_servers(&ticket);
    current_id = ticket % NR;

    nr = main->nr_total;
    pos = -1;
    right = nr - 1;
    left = 0;

    // folosim cautarea binara pentru a gasi in hashring pozitia etichetei
    while (left <= right) {
        int m = (left + right) / 2;

        unsigned int hash_cmp = hash_function_servers(&main->hashring[m]);

        if (hash_cmp == hash_current) {
            pos = m;
            // retinem in aux_pos urmatoarea pozitie din hashring
            if (pos == main->nr_total - 1)
                aux_pos = 0;
            else
                aux_pos = pos + 1;

            if (pos == 0)
                pos_prev = main->nr_total - 1;
            else
                pos_prev = pos - 1;
            // retinem id-ul urmatoarei etichete care urmeaza in hashring
            next_id = main->hashring[aux_pos] % NR;
        }

        if (hash_cmp > hash_current)
            right = m - 1;
        else
            left = m + 1;
    }

    // in cazul in care urmeaza in hashring o replica a serverului, ne oprim
    // obiectele trebuie redistribuite catre noul server
    if (current_id == next_id)
        return;

    int  pos_current, pos_next;

    // gasim pozitia pe care se afla serverul curent in vectorul de servere
    pos_current = return_pos(main, current_id);
    pos_next = return_pos(main, next_id);

    server_memory *next_server = main->server[pos_next];
    unsigned int hash = hash_function_servers(&ticket);

    int label = main->hashring[pos_prev];
    unsigned int hash_prev = hash_function_servers(&label);

    // vom muta elementele din server vechi in cel nou
    for (int i = 0; i < HMAX; i++) {
        hashtable_t *next_ht = next_server->hashtable;
        linked_list_t *list_bucket;

        list_bucket = next_ht->buckets[i];

        ll_node_t *current_node;
        current_node = list_bucket->head;
        while (current_node) {
            ll_node_t *next_node = current_node->next;

            char *key = (char *)(((info *)current_node->data)->key);
            char *value = (char *)(((info *)current_node->data)->value);

            unsigned int hash_key = hash_function_key(key);

            if ((hash_key < hash || hash_key > hash_prev)
                || (hash_key > hash_prev && hash_key < hash)) {
            current_node = next_node;

            // stocam elementele pe serverul nou
            server_memory *server_current = main->server[pos_current];
            server_store(server_current, key, value);
            } else {
                current_node = current_node->next;
            }

            if (hash_key > hash_prev && hash_key < hash) {
                current_node = next_node;
                server_store(main->server[pos_current], key, value);
                // stergem elementele din vechiul server
                server_remove(main->server[pos_next], key);
            }
        }
    }
}

// functie care adauga un nou server in sistem
void loader_add_server(load_balancer *main, int server_id)
{
    int nr_hash = main->nr_total + NRH;
    int nr = main->nr_server + NRS;

    // realocam vectorul hashring deoarece trebuie sa adaugam cele trei etichete
    main->hashring = realloc(main->hashring, nr_hash * sizeof(int));

    // realocam vectorul de servere deoarece vom aduga un server in plus
    main->server = realloc(main->server, nr * sizeof(struct server_memory*));

    main->server[main->nr_server] = init_server_memory();
    main->server[main->nr_server]->id = server_id;
    main->nr_server++;

    int k = 0;
    while (k < NRH)
    {
        // calculam cele trei etichete pe care le vom pune in hashring
        int ticket = k * NR + server_id;
        put_hashring(main, ticket);

        // redistribuirea elementelor
        map(main, ticket);
        k++;
    }
}

// stergerea unui element din hashring
void delete_hashring(load_balancer *main, int ticket)
{
    unsigned int hash_current;
    int nr, pos, right, left;

    hash_current = hash_function_servers(&ticket);

    nr = main->nr_total;
    pos = -1;
    right = nr - 1;
    left = 0;

    // cautam pozitia pe care se afla eticheta data, folosind cautare binara
    while (left <= right) {
        int m = (left + right) / 2;
        unsigned int hash_cmp = hash_function_servers(&main->hashring[m]);

        if (hash_cmp == hash_current)
            pos = m;

        if (hash_cmp > hash_current)
            right = m - 1;
        else
            left = m + 1;
    }

    // mutam elementele cu o pozitie la stanga
    move_element_left(main, pos);

    main->nr_total--;
}

// functie de redisitribuie a elementelor dupa stergere
void map_delete(load_balancer *main, int server_id)
{
    // aflam pozitia din vectorul de servere
    int pos_current = return_pos(main, server_id);

    server_memory *current_server = main->server[pos_current];

    hashtable_t *current_ht = current_server->hashtable;

    // vom parcurge intregul hashtable
    for (int i = 0; i < HMAX; i++) {
        linked_list_t *list_bucket;

        list_bucket = current_ht->buckets[i];

        ll_node_t *current_node;
        current_node = list_bucket->head;

        int size = list_bucket->size;

        for (int j = 0; j < size; j++) {
            ll_node_t *next_node = current_node->next;
            char *key = (char *)(((info *)current_node->data)->key);
            char *value = (char *)(((info *)current_node->data)->value);
            // stocam elemtele ramase pe serverele disponibile
            loader_store(main, key, value, &server_id);

            current_node = next_node;
        }
    }
    // eliberam memoria pentru serverul care urmeaza a fi sters
    free_server_memory(main->server[pos_current]);
    main->server[pos_current] = NULL;

    // mutam elementele din vectorul de servere
    move_element(main, pos_current);

    main->nr_server--;
}

// functie care sterge un server si rebalanseaza elementele
void loader_remove_server(load_balancer *main, int server_id)
{
    int k = 0;

    while (k < NRH)
    {   int ticket;
        ticket = k * NR + server_id;
        // stergem din hashring toate etichetele corespunzatoare serverului
        delete_hashring(main, ticket);
        k++;
    }
    // redistribuim elementele
    map_delete(main, server_id);

    // facem realocarea pentru vectorul cu etichte si pentru cel cu servere
    main->hashring = realloc(main->hashring, main->nr_total * sizeof(int));
    int nr = main->nr_server;
    main->server = realloc(main->server, nr * sizeof(struct server_memory*));
}

// eliberarea memoriei pentru load_balancer
void free_load_balancer(load_balancer *main)
{
    free(main->hashring);
    for (int i = 0; i < main->nr_server; i++)
        free_server_memory(main->server[i]);

    free(main->server);
    free(main);
}
