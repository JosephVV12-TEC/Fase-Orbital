//Este es en donde estara todo el archivo del juego como tal con la interfaz grafica y todo lo que se necesite para el juego
#pragma once

#include <string>
#include <cstdint>

using namespace std;

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

// Evento que indica cuando y como debe aparecer un enemigo.
struct Spawn
{
	int tick = 0; // Tick del juego en que aparece.
	TipoEnemigo tipo = TipoEnemigo::Dron; // Tipo de enemigo que aparece.
	int color = 0; // Codigo de color del enemigo.
	int angulo = 0; // Angulo inicial de aparicion.
	int sentido = 0; // Sentido de giro o movimiento.
};

// Cola enlazada de eventos Spawn; sus nodos son propiedad de la cola.
struct ColaSpawn
{
	struct Nodo
	{
		Spawn dato; // Evento almacenado.
		Nodo* sig = NULL; // Siguiente evento de la cola.
	};

	Nodo* frente = NULL; // Primer evento pendiente.
	Nodo* finalNodo = NULL; // Ultimo evento para encolar en O(1).
	int tam = 0; // Cantidad actual de eventos.

	// Crea una cola vacia y libera sus nodos al destruirse.
	ColaSpawn();
	~ColaSpawn();
	// Copia eventos creando nodos independientes.
	ColaSpawn(const ColaSpawn& otra);
	ColaSpawn& operator=(const ColaSpawn& otra);
	// Agrega al final o extrae del frente en O(1).
	void encolar(const Spawn& s);
	bool desencolar(Spawn& out);
	// Devuelve el frente sin extraerlo, o NULL si esta vacia.
	const Spawn* verFrente();
	bool vacia();
	// Libera todos los nodos y deja la cola vacia.
	void vaciar();
};

// Tipo del jugador definido por el sistema de juego.
struct Jugador;

// Lista circular doblemente enlazada que conserva punteros no propietarios a jugadores.
struct RondaJugadores
{
	struct Nodo
	{
		Jugador* j; // Jugador guardado; no se destruye desde la ronda.
		Nodo* ant; // Jugador anterior en la ronda circular.
		Nodo* sig; // Jugador siguiente en la ronda circular.
	};

	Nodo* actual = NULL; // Nodo cuyo turno esta activo.
	int tam = 0; // Cantidad de jugadores registrados.

	// Crea una ronda vacia; el destructor libera solo los nodos internos.
	RondaJugadores();
	~RondaJugadores();
	RondaJugadores(const RondaJugadores&) = delete;
	RondaJugadores& operator=(const RondaJugadores&) = delete;
	// Inserta el jugador antes del actual en O(1).
	void agregar(Jugador* j);
	// Mueve el turno en sentido horario (sig) o antihorario (ant).
	void avanzar(bool horario);
	// Quita el jugador actual, mueve el turno y devuelve el puntero sin destruirlo.
	Jugador* eliminarActual(bool horario);
	// Consulta el jugador actual o devuelve NULL si no hay jugadores.
	Jugador* verActual();
	int tamano();
};

// Generador congruencial lineal usado por la seccion 3.6.
struct Generador
{
	uint32_t x; // Estado interno del generador.

	// Inicializa el estado con la semilla indicada.
	Generador(uint32_t semilla);
	// Avanza el estado y devuelve el siguiente valor pseudoaleatorio.
	uint32_t siguiente();
};

//Llamamos a la otra libreria que ya hicimos para que se pueda usar en este archivo
#include "FuncionesJuego.h"


