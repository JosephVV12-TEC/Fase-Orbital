#pragma once
#include "Jugador.h"
#include "ListaEnemigos.h"
#include "ColaSpawn.h"
#include "ListaBalas.h"

// Posicion (polar) donde murio un enemigo; la vista la usa para particulas
struct PuntoBaja
{
    double r;
    double theta;
    int color;
};

struct Motor 
{
    Jugador* jugadorActual;      // Un puntero al jugador que está jugando
    ListaEnemigos listaEnemigos;
    ListaBalas listaBalas;
    ColaSpawn colaSpawnOleada;

    int tickActual;
    double anguloNave;

    // ---- Acciones: las escribe la vista / el bot ANTES de cada tick ----
    double anguloObjetivo;   // hacia donde debe girar la nave (max 6 grados por tick)
    bool pulsoSolicitado;    // true = el jugador pidio activar el pulso

    // ---- Onda de pulso (la vista solo la dibuja) ----
    bool ondaActiva;
    double ondaR;

    // ---- Eventos del ultimo tick (la vista los lee para sonido y efectos) ----
    bool evDisparo;
    bool evErroneo;
    bool evDanoNucleo;
    bool evPulso;
    int nBajas;
    PuntoBaja bajas[32];
};

// Funciones básicas del motor
void inicializarMotor(Motor& m);
void iniciarTurnoMotor(Motor& m, Jugador* nuevoJugador, ColaSpawn colaOleada);
void procesarTickMotor(Motor& m, int nuevoColor);