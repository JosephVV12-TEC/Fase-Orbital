#pragma once

#include <stdio.h>
#include <iostream>

//Ahora se hace el struct del spawn de los enemigos, que es una cola de enemigos que se van a ir apareciendo en el juego
typedef struct Spawn 
{
	int tipoEnemigo; // 0 = Enemigo1, 1 = Enemigo2, 2 = Enemigo3
	int tickSpawn;   // El tick en el que debe aparecer
	int color;	   // El color del enemigo que debe aparecer
	int angulo;      // El ángulo en el que debe aparecer porque como orbita en un planeta entonces vendran de distintos angulos de 0 a 359
	int sentido;
} *PtrSpawn;

typedef struct NodoSpawn //Este struct sirve en palabras simples para crear una lista enlazada de spawns
{
	Spawn dato;
	NodoSpawn* siguiente;

} *PtrNodoSpawn;

struct ColaSpawn //Este struct es la cola de spawns, que es basicamente una lista enlazada de los nodosspawn (basicamente una lista donde va a haber como una filita de las cosas que se van a ir spawneando en el juego) y trae a nodospawn porque es la lista enlazada de spawns, que es lo que se va a usar para la cola de enemigos
{
	PtrNodoSpawn frente;
	PtrNodoSpawn final;
	int tam; // Tamaño de la cola, para saber cuántos elementos hay
};
//ColaSpawn y NodoSpawn se diferencian en que Cola es el que tiene el frente y el final de la lista, mientras que Nodo es el que tiene el dato y el siguiente, que es el que se va a ir moviendo en la lista es como poner una dentro de otra

//Ahora se hacen las funciones de la cola de spawns, que son las funciones que se van a usar para manejar la cola de enemigos que se van a ir apareciendo en el juego
//La c sera la cola de spawns, y el s sera el spawn que se va a encolar o desencolar, y el salida sera el spawn que se va a desencolar y se va a guardar en la variable salida
//No se necesita poner nada dentro de las funciones porque se van a implementar en el archivo .cpp, esto hace que se evite que se vea todo el codigo en un solo archivo y se vea mas ordenado, ademas de que hace que sea mas facil de leer y entender el codigo

void inicializarCola(ColaSpawn& c);
void encolar(ColaSpawn& c, Spawn s);
bool desencolar(ColaSpawn& c, Spawn& salida);
PtrSpawn verFrente(ColaSpawn& c);
bool colaVacia(ColaSpawn& c);
void vaciarCola(ColaSpawn& c);
void copiarCola(ColaSpawn& origen, ColaSpawn& destino);
