//Aca vamos a poner todas las funciones que vamos a usar en el juego, para que no se vea tan desordenado el Fase Orbital
#pragma once



//Librerias de C++
#include <iostream>
#include <cmath>

//Librerias de Allegro
#include "ListaEnemigos.h"
#include "ListaBalas.h"

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
	cabeza = NULL;
	cola = NULL;
	tam = 0;
}

// Agrega un nodo al final ajustando cabeza/cola y los enlaces en O(1).
NodoEnemigo* ListaEnemigos::insertarFinal(const Enemigo& e)
{
	NodoEnemigo* nodo = new NodoEnemigo;
	nodo->dato = e;
	nodo->ant = NULL;
	nodo->sig = NULL;

	if (vacia())
	{
		cabeza = nodo;
		cola = nodo;
	}
	else
	{
		nodo->ant = cola;
		cola->sig = nodo;
		cola = nodo;
	}

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
NodoEnemigo* ListaEnemigos::primero() const
{
	return cabeza;
}

NodoEnemigo* ListaEnemigos::ultimo() const
{
	return cola;
}

int ListaEnemigos::tamano() const
{
	return tam;
}

bool ListaEnemigos::vacia() const
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


