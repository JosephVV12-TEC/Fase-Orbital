//Este es en donde estara todo el archivo del juego como tal con la interfaz grafica y todo lo que se necesite para el juego
#pragma once

#include <string>
#include <cstdint>

//Llamamos a la otra libreria que ya hicimos para que se pueda usar en este archivo
#include "FuncionesJuego.h"
#include "Jugador.h"
#include "ColaSpawn.h"
#include "RondaJugadores.h"
#include "Motor.h"

using namespace std;

// Generador congruencial lineal usado por la seccion 3.6.
struct Generador
{
	uint32_t x; // Estado interno del generador.

	// Inicializa el estado con la semilla indicada.
	Generador(uint32_t semilla);
	// Avanza el estado y devuelve el siguiente valor pseudoaleatorio.
	uint32_t siguiente();
};

