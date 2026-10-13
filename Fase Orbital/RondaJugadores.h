#pragma once
#include <stdio.h>   
#include "Jugador.h"
//Ronda jugadores es para el modo multijugador, ya que es una lista circular doblemente enlazada que contiene punteros a los jugadores y permite avanzar el turno de manera sencilla, ademas de eliminar jugadores de la ronda si se necesita
//Se puede imaginar como un circulo donde cada jugador es un nodo y por ejemplo si le toca a jugador 1 este jugara y luego se avanzara al jugador 2 y asi sucesivamente, si un jugador es eliminado de la ronda el siguiente jugador sera el que le sigue en el circulo

typedef struct nodoRonda //Este es el struct que representa un nodo de la ronda, que contiene un puntero al jugador y punteros a los nodos anterior y siguiente
{
    Jugador* j;
    nodoRonda* Anterior;
    nodoRonda* Siguiente;
}*PtrNodoRonda;


struct RondaJugadores //Este segundo struct te da la ronda en si, aca estan los punteros a los nodos del anterior struct
{
    PtrNodoRonda actual;   //El jugador al que le toca
    int tam;               //Cuantos jugadores hay
};

//En la siguiente parte se encuentran las funciones que se pueden usar para manipular la ronda de jugadores, como inicializarla, agregar jugadores, avanzar el turno, eliminar jugadores, ver el jugador actual, vaciar la ronda y verificar si los enlaces son válidos (esta ultima es mas que nada para pruebas internas del programa)

void inicializarRonda(RondaJugadores& r);
void agregarRonda(RondaJugadores& r, Jugador* j);
void avanzarRonda(RondaJugadores& r, bool horario);
Jugador* eliminarActualRonda(RondaJugadores& r, bool horario);
Jugador* verActualRonda(RondaJugadores& r);
void vaciarRonda(RondaJugadores& r);
bool enlacesValidosRonda(RondaJugadores& r);

