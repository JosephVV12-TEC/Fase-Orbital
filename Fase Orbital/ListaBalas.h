#pragma once
#include <stdio.h>

// Datos de una bala; las velocidades se expresan en los ejes x e y.
struct Bala
{
	double x = 0.0; // Posicion horizontal.
	double y = 0.0; // Posicion vertical.
	double vx = 0.0; // Velocidad horizontal.
	double vy = 0.0; // Velocidad vertical.
	int color = 0; // Codigo de color de la bala.
	int duenio = 0; // Identificador del jugador propietario.
};

//Denificion del nodo para las balas
struct NodoBala
{
	Bala dato; // Datos de la bala guardados en el nodo.
	NodoBala* sig = NULL; // Siguiente nodo de la lista simplemente enlazada.

	//Al destruirse, se ejecuta esta función
	~NodoBala();
};

//Definicion de la lista de las balas
struct ListaBalas
{
	//inicia la primera en NULL
	NodoBala* cabeza = NULL;
	int tam = 0; // Cantidad actual de nodos.

	// Inicia una lista de balas vacia.
	ListaBalas();
	//Destruye la lista de balas.
	~ListaBalas();
	// Impide copias superficiales que compartirian los mismos nodos.
	ListaBalas(const ListaBalas&) = delete;
	ListaBalas& operator=(const ListaBalas&) = delete;

	// Inserta una copia al inicio en O(1).
	void insertarInicio(const Bala& b);
	// Elimina nodos que satisfacen el predicado y devuelve cuantos elimino.
	template <class Pred>
	int eliminarSi(Pred debeEliminarse);
};