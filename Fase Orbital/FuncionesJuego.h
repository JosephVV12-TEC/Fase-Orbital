//Aca vamos a poner todas las funciones que vamos a usar en el juego, para que no se vea tan desordenado el Fase Orbital
#pragma once



//Librerias de C++
#include <iostream>
#include <cmath>

//Librerias de Allegro
#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

//Se incluye al juego en si
#include "Juego.h"

using namespace std;

// El nodo no posee recursos externos; Enemigo se destruye automaticamente.
NodoEnemigo::~NodoEnemigo()
{
}

// Deja los extremos en NULL y el contador en cero.
ListaEnemigos::ListaEnemigos() : cabeza(NULL), cola(NULL), tam(0)
{
}

// Libera todos los nodos que pertenecen a la lista.
ListaEnemigos::~ListaEnemigos()
{
	NodoEnemigo* actual = cabeza;
	while (actual != NULL)
	{
		// Guarda el enlace antes de liberar el nodo actual.
		NodoEnemigo* siguiente = actual->sig;
		delete actual;
		actual = siguiente;
	}
}

// Agrega un nodo al final ajustando cabeza/cola y los enlaces en O(1).
NodoEnemigo* ListaEnemigos::insertarFinal(const Enemigo& e)
{
	NodoEnemigo* nodo = new NodoEnemigo;
	nodo->dato = e;
	nodo->ant = cola;
	nodo->sig = NULL;

	if (cola != NULL)
		cola->sig = nodo;
	else
		cabeza = nodo;

	cola = nodo;
	++tam;
	return nodo;
}

// Inserta despues de n en O(1) y repara el enlace anterior del sucesor.
NodoEnemigo* ListaEnemigos::insertarDespues(NodoEnemigo* n, const Enemigo& e)
{
	if (n == NULL)
		return NULL;

	NodoEnemigo* nodo = new NodoEnemigo;
	nodo->dato = e;
	nodo->ant = n;
	nodo->sig = n->sig;

	if (n->sig != NULL)
		n->sig->ant = nodo;
	else
		cola = nodo;

	n->sig = nodo;
	++tam;
	return nodo;
}

// Desconecta n, actualiza los extremos y devuelve su sucesor antes de borrarlo.
NodoEnemigo* ListaEnemigos::eliminar(NodoEnemigo* n)
{
	if (n == NULL)
		return NULL;

	NodoEnemigo* siguiente = n->sig;
	if (n->ant != NULL)
		n->ant->sig = n->sig;
	else
		cabeza = n->sig;

	if (n->sig != NULL)
		n->sig->ant = n->ant;
	else
		cola = n->ant;

	delete n;
	--tam;
	return siguiente;
}

// Accesores para recorrer la lista desde cada extremo.
NodoEnemigo* ListaEnemigos::primero()
{
	return cabeza;
}

NodoEnemigo* ListaEnemigos::ultimo()
{
	return cola;
}

int ListaEnemigos::tamano()
{
	return tam;
}

bool ListaEnemigos::vacia()
{
	return tam == 0;
}

// Recorre todos los enemigos y elimina los que coinciden con el color indicado.
void ListaEnemigos::eliminarSi(int color)
{
	NodoEnemigo* actual = cabeza;
	while (actual != NULL)
	{
		if (actual->dato.color == color)
			actual = eliminar(actual);
		else
			actual = actual->sig;
	}
}

NodoBala::~NodoBala()
{
}

// Crea una lista vacia de balas.
ListaBalas::ListaBalas() : cabeza(NULL), tam(0)
{
}

// Libera cada nodo de bala que sigue enlazado en la lista.
ListaBalas::~ListaBalas()
{
	NodoBala* actual = cabeza;
	while (actual != NULL)
	{
		NodoBala* siguiente = actual->sig;
		delete actual;
		actual = siguiente;
	}
}

// Copia la bala en un nodo nuevo y lo enlaza antes de la cabeza actual.
void ListaBalas::insertarInicio(const Bala& b)
{
	NodoBala* nodo = new NodoBala;
	nodo->dato = b;
	nodo->sig = cabeza;
	cabeza = nodo;
	++tam;
}

template <class Pred>
int ListaBalas::eliminarSi(Pred debeEliminarse)
{
	// actual apunta al puntero que enlaza el siguiente nodo, incluida cabeza.
	NodoBala** actual = &cabeza;
	int eliminadas = 0;
	while (*actual != NULL)
	{
		if (debeEliminarse((*actual)->dato))
		{
			// Salta el nodo coincidente, libera su memoria y no avanza el enlace.
			NodoBala* eliminado = *actual;
			*actual = eliminado->sig;
			delete eliminado;
			--tam;
			++eliminadas;
		}
		else
		{
			// Avanza al campo sig del nodo que se conserva.
			NodoBala* nodoActual = *actual;
			actual = &nodoActual->sig;
		}
	}
	return eliminadas;
}

// Inicia la cola sin eventos pendientes.
ColaSpawn::ColaSpawn() : frente(NULL), finalNodo(NULL), tam(0)
{
}

// La cola es propietaria de sus nodos y los libera al destruirse.
ColaSpawn::~ColaSpawn()
{
	vaciar();
}

// Copia cada evento a un nodo nuevo para que ambas colas sean independientes.
ColaSpawn::ColaSpawn(const ColaSpawn& otra) : frente(NULL), finalNodo(NULL), tam(0)
{
	Nodo* actual = otra.frente;
	while (actual != NULL)
	{
		encolar(actual->dato);
		actual = actual->sig;
	}
}

// Reemplaza el contenido por una copia profunda y protege la autoasignacion.
ColaSpawn& ColaSpawn::operator=(const ColaSpawn& otra)
{
	if (this == &otra)
		return *this;

	vaciar();
	Nodo* actual = otra.frente;
	while (actual != NULL)
	{
		encolar(actual->dato);
		actual = actual->sig;
	}
	return *this;
}

// Agrega un evento despues del ultimo nodo en tiempo O(1).
void ColaSpawn::encolar(const Spawn& s)
{
	Nodo* nodo = new Nodo;
	nodo->dato = s;
	nodo->sig = NULL;

	if (finalNodo != NULL)
		finalNodo->sig = nodo;
	else
		frente = nodo;

	finalNodo = nodo;
	++tam;
}

// Extrae el primer evento; devuelve false si no hay ninguno.
bool ColaSpawn::desencolar(Spawn& out)
{
	if (frente == NULL)
		return false;

	Nodo* eliminado = frente;
	out = eliminado->dato;
	frente = eliminado->sig;
	if (frente == NULL)
		finalNodo = NULL;

	delete eliminado;
	--tam;
	return true;
}

// Permite consultar el evento proximo sin quitarlo de la cola.
const Spawn* ColaSpawn::verFrente()
{
	if (frente == NULL)
		return NULL;
	return &frente->dato;
}

// Indica si la cola no contiene eventos.
bool ColaSpawn::vacia()
{
	return frente == NULL;
}

// Elimina todos los nodos y restablece ambos extremos y el contador.
void ColaSpawn::vaciar()
{
	while (frente != NULL)
	{
		Nodo* eliminado = frente;
		frente = frente->sig;
		delete eliminado;
	}
	finalNodo = NULL;
	tam = 0;
}

// Inicia una ronda sin jugadores.
RondaJugadores::RondaJugadores() : actual(NULL), tam(0)
{
}

// Libera los nodos internos sin destruir los objetos Jugador apuntados.
RondaJugadores::~RondaJugadores()
{
	while (actual != NULL)
	{
		Nodo* eliminado = actual;
		if (tam == 1)
			actual = NULL;
		else
		{
			actual = actual->sig;
			actual->ant = eliminado->ant;
			eliminado->ant->sig = actual;
		}
		delete eliminado;
		--tam;
	}
}

// Inserta antes del actual; el primero forma un anillo apuntandose a si mismo.
void RondaJugadores::agregar(Jugador* j)
{
	if (j == NULL)
		return;

	Nodo* nodo = new Nodo;
	nodo->j = j;
	if (actual == NULL)
	{
		nodo->ant = nodo;
		nodo->sig = nodo;
		actual = nodo;
	}
	else
	{
		Nodo* anterior = actual->ant;
		nodo->ant = anterior;
		nodo->sig = actual;
		anterior->sig = nodo;
		actual->ant = nodo;
	}
	++tam;
}

// Cambia el nodo actual al vecino segun el sentido solicitado.
void RondaJugadores::avanzar(bool horario)
{
	if (actual == NULL)
		return;
	actual = horario ? actual->sig : actual->ant;
}

// Desconecta el actual, mueve el turno en el sentido indicado y devuelve j.
Jugador* RondaJugadores::eliminarActual(bool horario)
{
	if (actual == NULL)
		return NULL;

	Nodo* eliminado = actual;
	Jugador* jugador = eliminado->j;
	if (tam == 1)
		actual = NULL;
	else
	{
		Nodo* nuevoActual = horario ? eliminado->sig : eliminado->ant;
		eliminado->ant->sig = eliminado->sig;
		eliminado->sig->ant = eliminado->ant;
		actual = nuevoActual;
	}

	delete eliminado;
	--tam;
	return jugador;
}

// Devuelve el jugador del turno actual, o NULL cuando el anillo esta vacio.
Jugador* RondaJugadores::verActual()
{
	if (actual == NULL)
		return NULL;
	return actual->j;
}

// Devuelve cuantos jugadores hay en el anillo.
int RondaJugadores::tamano()
{
	return tam;
}

// Inicializa el estado interno con la semilla recibida.
Generador::Generador(uint32_t semilla) : x(semilla)
{
}

// Aplica el generador congruencial lineal especificado en la seccion 3.6.
uint32_t Generador::siguiente()
{
	// La aritmetica sin signo y la mascara mantienen el resultado en 31 bits.
	x = (1103515245u * x + 12345u) & 0x7FFFFFFFu;
	return x;
}
