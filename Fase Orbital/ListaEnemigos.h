#pragma once
#include <stdio.h>

// Tipos de enemigos que pueden aparecer durante el juego.
enum class TipoEnemigo { Dron, Girador, Divisor, Mini, Tanque, Camaleon };

// Datos de un enemigo; se copian dentro de un nodo de ListaEnemigos.
struct Enemigo
{
	int id = 0; // Identificador unico del enemigo.
	TipoEnemigo tipo = TipoEnemigo::Dron; // Clase o comportamiento del enemigo.
	int color = 0; // Color codificado: 0 = C, 1 = M, 2 = A.
	double r = 0.0; // Radio de la posicion polar.
	double theta = 0.0; // Angulo de la posicion polar.
	double omega = 0.0; // Velocidad angular.
	int vida = 0; // Puntos de vida restantes.
	int radio = 0; // Radio de colision del enemigo.
	int ticksColor = 0; // Tiempo restante antes de cambiar el color.
};

//Denificion del nodo para los enemigos
struct NodoEnemigo
{
	Enemigo dato; // Datos que guarda este nodo.
	NodoEnemigo* ant = NULL; // Nodo anterior en la lista doblemente enlazada.
	NodoEnemigo* sig = NULL; // Nodo siguiente en la lista doblemente enlazada.

	//Al destruirse, se ejecuta esta funcion, seguro la voy a usar para eliminar la lista o algo, lo recomendo Copilot yo que se
	~NodoEnemigo();
};

//Definicion de la lista de enemigos
struct ListaEnemigos
{
	//Lista doblemente enlazada, ya que tiene punteros a la cabeza y la cola
	NodoEnemigo* cabeza = NULL;
	NodoEnemigo* cola = NULL;
	int tam = 0; // Cantidad actual de nodos.

	// Inicia una lista vacia.
	ListaEnemigos();
	//Destruye la lista de enemigos
	~ListaEnemigos();
	// Impide copias superficiales que compartirian los mismos nodos.
	ListaEnemigos(const ListaEnemigos&) = delete;
	ListaEnemigos& operator=(const ListaEnemigos&) = delete;

	// Inserta al final en O(1) y devuelve el nodo creado.
	NodoEnemigo* insertarFinal(const Enemigo& e);
	// Inserta despues del nodo indicado en O(1) y devuelve el nodo creado.
	NodoEnemigo* insertarDespues(NodoEnemigo* n, const Enemigo& e);
	// Desconecta y libera el nodo en O(1); devuelve el nodo que le seguia.
	NodoEnemigo* eliminar(NodoEnemigo* n);
	// Devuelve el primer o ultimo nodo sin modificar la lista.
	NodoEnemigo* primero();
	NodoEnemigo* ultimo();
	// Devuelve la cantidad de nodos y permite consultar si esta vacia.
	int tamano();
	bool vacia();
	//Elimina un enemigo si colisiona con una bala de ese color
	// Recorre la lista y elimina todos los enemigos con el codigo de color indicado.
	void eliminarSi(int color);
};
