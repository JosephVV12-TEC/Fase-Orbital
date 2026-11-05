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
    Jugador* jugadorActual;// Un puntero al jugador que está jugando
    ListaEnemigos listaEnemigos;
    ListaBalas listaBalas;
    ColaSpawn colaSpawnOleada;

    int tickActual;
    int oleadaActual;  
    int idJugador;             // indice del jugador actual en Partida.jugadores (para la bitacora)
    BitacoraEventos* bitacora; // donde se registran los eventos; nullptr = no registrar// numero de la oleada que se esta jugando (define la velocidad)
    double anguloNave;

    // ---- Acciones: las escribe la vista / el bot ANTES de cada tick ----
    double anguloObjetivo;// hacia donde debe girar la nave (max 6 grados por tick)
    bool pulsoSolicitado; // true = el jugador pidio activar el pulso

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

//Bitacora de eventos: se usa para estadisticas y para la vista (sonido, particulas, etc)
enum class TipoEvento { BAJA, IMPACTO_ERRONEO, DANO_NUCLEO, CHOQUE_NAVE, PULSO, FIN_OLEADA, ELIMINACION };

struct Evento
{
    int tick;
    int oleada;
    int jugador;       // indice del jugador dentro de Partida.jugadores
    TipoEvento tipo;
    int tipoEnemigo;   // 0..5 (Dron..Camaleon); -1 si no aplica
    int color;         // 0..2 (C, M, A); -1 si no aplica
    int valor;         // BAJA: puntos ganados; DANO_NUCLEO y CHOQUE_NAVE: dano; FIN_OLEADA y ELIMINACION: puntaje; otros: 0
};

struct NodoEvento
{
    Evento dato;
    NodoEvento* sig;
};

struct BitacoraEventos
{
    NodoEvento* cabeza;
    NodoEvento* cola;
    int tam;

    BitacoraEventos();                                   // bitacora vacia
    ~BitacoraEventos();                                  // libera todos los nodos
    BitacoraEventos(const BitacoraEventos&) = delete;    // no tiene sentido duplicar la bitacora
    BitacoraEventos& operator=(const BitacoraEventos&) = delete;
};

void agregarFinal(BitacoraEventos& b, const Evento& e);                  // O(1)
void vaciarBitacora(BitacoraEventos& b);
// Devuelve el primer nodo, desde 'desde' en adelante, del jugador y la oleada pedidos (-1 = cualquiera)
NodoEvento* siguienteEvento(NodoEvento* desde, int jugador, int oleada);
int contarEventos(const BitacoraEventos& b, int jugador, int oleada, TipoEvento tipo);
int sumarValorEventos(const BitacoraEventos& b, int jugador, int oleada, TipoEvento tipo);

// Funciones básicas del motor
// Resultado de un tick: la vista solo lo lee, el Motor es quien decide
enum class ResultadoTick { Sigue, OleadaCompleta, NucleoDestruido };

void inicializarMotor(Motor& m);
void iniciarTurnoMotor(Motor& m, Jugador* nuevoJugador, const ColaSpawn& colaOleada, int oleada, int idJugador, BitacoraEventos* bitacora);
ResultadoTick procesarTickMotor(Motor& m, int nuevoColor);
int multiplicadorPuntos(int combo);   // mult = min(8, 1 + combo / 6)