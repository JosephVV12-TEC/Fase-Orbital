#pragma once
#include "Jugador.h"
#include "ListaEnemigos.h"
#include "ListaBalas.h"
#include "ColaSpawn.h"

struct Motor {
    Jugador* jugadorActual;      // Un puntero al jugador que está jugando
    ListaEnemigos listaEnemigos;
    ListaBalas listaBalas;
    ColaSpawn colaSpawnOleada;

    int tickActual;
    double anguloNave;
};

// Funciones básicas del motor
void inicializarMotor(Motor& m);
void iniciarTurnoMotor(Motor& m, Jugador* nuevoJugador, ColaSpawn colaOleada);
void procesarTickMotor(Motor& m, int nuevoColor);