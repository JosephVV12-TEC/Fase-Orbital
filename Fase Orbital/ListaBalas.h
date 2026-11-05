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
	bool inactiva = false; // true = ya impacto o salio del campo; eliminarSi la borra.
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


//Esta es la plantilla de la funcion eliminarSi, que elimina los nodos que cumplan con el predicado (la condicion que se le pase) y devuelve la cantidad de nodos eliminados
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