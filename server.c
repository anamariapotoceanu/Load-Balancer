/* Copyright 2023 Potoceanu Ana-Maria 311CAb */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <errno.h>
#include "server.h"
#include "hashtable.h"

server_memory *init_server_memory()
{
	server_memory *server_mem;
	// alocam memorie pentru server
	server_mem = malloc(sizeof(server_memory));

	// verificam daca alocarea a avut succes
	DIE(!server_mem, "Malloc failed.\n");

	server_mem->hashtable =
	ht_create(HMAX, hash_function_string, compare_function_strings);
	return server_mem;
}

// stocam in hashtable datele primite
void server_store(server_memory *server, char *key, char *value) {
	unsigned int len_key = strlen(key) + 1;
	unsigned int len_value = strlen(value) + 1;
	// punem in hashtable cheia si valoarea
	ht_put(server->hashtable, key, len_key, value, len_value);
}

// returnam valoarea din hashtable asociata unei chei date
char *server_retrieve(server_memory *server, char *key) {
	return ht_get(server->hashtable, key);
}

// stergem o cheie data din hashtable
void server_remove(server_memory *server, char *key) {
	ht_remove_entry(server->hashtable, key);
}

// eliberam memoria pentru server
void free_server_memory(server_memory *server) {
	ht_free(server->hashtable);
	free(server);
}
