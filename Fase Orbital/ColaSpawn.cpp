#include "ColaSpawn.h"

//Deja la cola vacia porque si no puede tener basura en el frente y final, y en tamano 
void inicializarCola(ColaSpawn& c) 
{
    c.frente = NULL;
    c.final = NULL;
    c.tam = 0;
}

//Ya aca se definen las funciones que se pusieron en el .h, y se implementan en el .cpp, que es lo que hace que se vea mas ordenado y no se vea todo el codigo en un solo archivo

void encolar(ColaSpawn& c, Spawn s) 
{

    PtrNodoSpawn Nuevo = new NodoSpawn;
    Nuevo->dato = s;
    Nuevo->siguiente = NULL;

    PtrNodoSpawn Aux = c.frente;
    if (Aux != NULL) {
        while (Aux->siguiente != NULL) 
        {
            Aux = Aux->siguiente;
        }
        Aux->siguiente = Nuevo;
    }
    else {
        c.frente = Nuevo;
    }

    c.final = Nuevo;//Mantener final al dia
	c.tam = c.tam + 1;//Mantener el contador de tamaño actualizado
}

//Saca el primer elemento y lo deja en 'salida'. Retorna false si la cola estaba vacia
bool desencolar(ColaSpawn& c) {

    if (c.frente == NULL) 
    {         
        return false;
    }

    PtrNodoSpawn Aux = c.frente;
    c.frente = c.frente->siguiente;
    Aux->siguiente = NULL;

    if (c.frente == NULL) 
    {          
        c.final = NULL;
    }

    c.tam = c.tam - 1;
    return Aux;
}

//Retorna un puntero al dato del frente sin sacarlo, o NULL si esta vacia
PtrSpawn verFrente(ColaSpawn& c) 
{
    if (c.frente == NULL) 
    {
        return NULL;
    }
    return &(c.frente->dato);
}

bool colaVacia(ColaSpawn& c) {
    return c.tam == 0;
}

//Libera todos los nodos y deja la cola vacia
void vaciarCola(ColaSpawn& c) {

    while (c.frente != NULL) {

        //Guardamos el nodo, avanzamos y recien ahi lo borramos
        PtrNodoSpawn aux = c.frente;
        c.frente = c.frente->siguiente;
        delete aux;
    }

    c.final = NULL;
    c.tam = 0;
}

//En palabras simples, esta funcion copia el contenido de una cola a otra, creando nuevos nodos para cada elemento. Esto es útil para duplicar la cola sin modificar la original
void copiarCola(ColaSpawn& origen, ColaSpawn& destino)
{

    //Si es la misma cola, no hay nada que copiar y asi se optimiza
    if (origen.frente == destino.frente)
    {
        return;
    }

    //Primero limpiamos lo que tuviera el destino
    vaciarCola(destino);

    //Recorremos el origen y creamos un nodo nuevo por cada elemento
    PtrNodoSpawn aux = origen.frente;
    while (aux != NULL) 
    {
        encolar(destino, aux->dato);
        aux = aux->siguiente;
    }
}