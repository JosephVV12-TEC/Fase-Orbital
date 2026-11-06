// Partida.h - Flujo de una partida multijugador por turnos (seccion 5.1 del PDF).
// NO usa Allegro: la vista solo la consulta y le avisa cuando termina un turno.
#pragma once
#include <string>
#include <ostream>
#include "Jugador.h"
#include "RondaJugadores.h"
#include "ColaSpawn.h"
#include "Motor.h"

const int MAX_JUGADORES = 6;

enum class ResultadoTurno
{
    SigueRonda,// todavia faltan jugadores por jugar esta oleada
    NuevaRonda,// todos jugaron; empieza la siguiente oleada
    PartidaTerminada// fin de la partida
};

struct Partida
{
    Jugador jugadores[MAX_JUGADORES]; // todos los registrados (tambien los eliminados)
    int oleadaAlcanzada[MAX_JUGADORES];
    int cantidad;// cuantos se registraron
    RondaJugadores ronda;// lista circular doble: solo los que siguen vivos
    ColaSpawn colaOleada;// cola ORIGINAL de la oleada (cada turno juega una copia)
    BitacoraEventos bitacora;// eventos de TODA la partida
    unsigned int semilla;
    int oleada;
    int limiteOleadas;
    bool horario;// sentido de la ronda actual (impar = horario)
    int turnosPendientes;// turnos que faltan en esta ronda
    bool enCurso;
    int ganador;// indice en jugadores[], -1 si no hay
};

void inicializarPartida(Partida& p);
void destruirPartida(Partida& p);
bool esAliasBot(const string& alias);// true si empieza con BOT
bool registrarJugador(Partida& p, const string& alias);
void comenzarPartida(Partida& p, Motor& m, unsigned int semilla, int limiteOleadas);
void iniciarTurnoActual(Partida& p, Motor& m);
ResultadoTurno terminarTurno(Partida& p, bool oleadaCompletada);
int indiceDeJugador(const Partida& p, const Jugador* j);
void calcularPosiciones(const Partida& p, int orden[MAX_JUGADORES]);
// Imprime posiciones, metricas por jugador y la linea de tiempo por oleada, en resyumen lee la bitacora
void imprimirReportePartida(const Partida& p, ostream& os);

//HISTORIAL

const int NUM_COLUMNAS = 33;
const int MAX_ALIAS_HIST = 50; //Max de jugadores (pa q no guarde 800000 mil)

// Posicion de cada columna numerica dentro de FilaHistorial::num
enum ColumnaHistorial
{
    COL_ID = 0, COL_SEMILLA = 1, COL_ES_BOT = 4, COL_POSICION = 5, COL_PUNTAJE = 6, COL_OLEADA = 7,
    COL_DISPAROS = 8, COL_CORRECTOS = 9, COL_ERRONEOS = 10, COL_COMBO_MAX = 11, COL_PULSOS = 12,
    COL_DANO_TOTAL = 13, COL_TICKS = 14,
    COL_BAJAS = 15,// 6 columnas: dron, girador, divisor, mini, tanque, camaleon
    COL_IMPACTOS_FASE = 21,// 3 columnas (extra): impactos con fase C, M, A
    COL_CORRECTOS_FASE = 24,// 3 columnas (extra): impactos correctos con fase C, M, A
    COL_DANO_TIPO = 27// 6 columnas (extra): dano recibido por tipo de enemigo
};

// Una fila de historial.csv = un jugador en una partida
struct FilaHistorial
{
    string modo;
    string jugador;
    int num[NUM_COLUMNAS];// columnas numericas (las posiciones 2 y 3 son texto y no se usan porque no se pueden guardar en un CSV (no se porque))
};

struct NodoFila
{
    FilaHistorial dato;
    NodoFila* sig;
};

// Lista simple con puntero a la cola para guardar el historial
struct HistorialPartidas
{
    NodoFila* cabeza;
    NodoFila* cola;
    int tam;

    HistorialPartidas();
    ~HistorialPartidas();
    HistorialPartidas(const HistorialPartidas&) = delete;
    HistorialPartidas& operator=(const HistorialPartidas&) = delete;
};

void agregarFila(HistorialPartidas& h, const FilaHistorial& f);   // O(1)
void vaciarHistorial(HistorialPartidas& h);
int cargarHistorial(HistorialPartidas& h, const string& archivo, ostream& errores);
bool guardarPartidaEnHistorial(HistorialPartidas& h, const Partida& p, const string& modo, const string& archivo);

//Merge Sort para calcular la mediana y la desviacion estandar de un jugador
struct NodoNum
{
    double valor;
    NodoNum* sig;
};
NodoNum* mergeSortLista(NodoNum* cabeza);// devuelve la nueva cabeza ordenada de menor a mayor
void liberarNumeros(NodoNum* cabeza);

//Estadisticas de un jugador a lo largo de todas sus partidas
struct ResumenJugador
{
    std::string alias;
    int partidas, victorias;
    int mejor, peor;// mejor y peor puntaje
    double media, mediana, desviacion;
    double precisionProm;//promedio de correctos / (correctos + erroneos)
    int mejorColor;// -1 si no hay datos
    int tipoMasDano;
    int histograma[10];//10 intervalos iguales entre el minimo y el maximo
};

void resumenDeJugador(const HistorialPartidas& h, const string& alias, ResumenJugador& r);
int calcularResumenes(const HistorialPartidas& h, ResumenJugador salida[], int max);   // devuelve cuantos jugadores
void imprimirEstadisticas(const HistorialPartidas& h, ostream& os);

// ---- Ranking: lista simple ordenada (insercion ordenada) ----
struct EntradaRanking
{
    string alias;
    int puntaje;       // mejor puntaje del jugador
    double precision;  // precision de color promedio
};

struct NodoRanking
{
    EntradaRanking dato;
    NodoRanking* sig;
};

struct Ranking
{
    NodoRanking* cabeza;
    int tam;

    Ranking();
    ~Ranking();
    Ranking(const Ranking&) = delete;
    Ranking& operator=(const Ranking&) = delete;
};

void insertarOrdenado(Ranking& r, const EntradaRanking& e);   // O(n)
void construirRanking(const ResumenJugador res[], int cantidad, Ranking& r);
void imprimirTop(const Ranking& r, int k, ostream& os);   // los k primeros
void imprimirRanking(const HistorialPartidas& h, ostream& os);   // top 10 del historial