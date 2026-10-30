// Partida.h - Flujo de una partida multijugador por turnos (seccion 5.1 del PDF).
// NO usa Allegro: la vista solo la consulta y le avisa cuando termina un turno.
#pragma once
#include <string>
#include "Jugador.h"
#include "RondaJugadores.h"
#include "ColaSpawn.h"
#include "Motor.h"

const int MAX_JUGADORES = 6;

enum class ResultadoTurno
{
    SigueRonda,       // todavia faltan jugadores por jugar esta oleada
    NuevaRonda,       // todos jugaron; empieza la siguiente oleada
    PartidaTerminada  // fin de la partida
};

struct Partida
{
    Jugador jugadores[MAX_JUGADORES]; // todos los registrados (tambien los eliminados)
    int oleadaAlcanzada[MAX_JUGADORES];
    int cantidad;                     // cuantos se registraron
    RondaJugadores ronda;             // lista circular doble: solo los que siguen vivos
    ColaSpawn colaOleada;             // cola ORIGINAL de la oleada (cada turno juega una copia)
    unsigned int semilla;
    int oleada;
    int limiteOleadas;
    bool horario;                     // sentido de la ronda actual (impar = horario)
    int turnosPendientes;             // turnos que faltan en esta ronda
    bool enCurso;
    int ganador;                      // indice en jugadores[], -1 si no hay
};

void inicializarPartida(Partida& p);
void destruirPartida(Partida& p);
bool esAliasBot(const std::string& alias);               // true si empieza con BOT
bool registrarJugador(Partida& p, const std::string& alias);
void comenzarPartida(Partida& p, Motor& m, unsigned int semilla, int limiteOleadas);
void iniciarTurnoActual(Partida& p, Motor& m);
ResultadoTurno terminarTurno(Partida& p, bool oleadaCompletada);
int indiceDeJugador(const Partida& p, const Jugador* j);
void calcularPosiciones(const Partida& p, int orden[MAX_JUGADORES]);
