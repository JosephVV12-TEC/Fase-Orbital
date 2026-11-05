#pragma once
#include <string>
using namespace std;

//6.1 
struct EstadisticasJugador {
    int disparos;          // balas emitidas
    int correctos;         // impactos con color igual al del enemigo
    int erroneos;          // impactos con color distinto
    int impactosFase[3];   // impactos totales segun la fase de la bala (C, M, A)
    int correctosFase[3];  // impactos correctos segun la fase de la bala
    int bajasTipo[6];      // bajas por tipo (Dron, Girador, Divisor, Mini, Tanque, Camaleon), incluye pulso
    int bajasColor[3];     // bajas por color del enemigo, incluye pulso
    int comboMax;          // mayor combo alcanzado
    int danoTotal;         // dano recibido en el nucleo
    int danoTipo[6];       // dano recibido, por tipo de enemigo que lo causo
    int pulsos;            // activaciones del pulso
    int ticks;             // ticks jugados en todos sus turnos
};

struct Jugador {
    string alias;
    bool esBot;
    int vidaNucleo;
    int puntaje;
    int combo;
    int cargaPulso;
    EstadisticasJugador est;
    int colorActivo; // 0 = Cian, 1 = Magenta, 2 = Ámbar
};
void reiniciarEstadisticas(EstadisticasJugador& e);

// Cada una devuelve un numero entre 0 y 1 (0 si todavia no hay datos)
double precisionColor(const Jugador& j);            // correctos / (correctos + erroneos)
double eficienciaDisparo(const Jugador& j);         // correctos / disparos
double precisionDeFase(const Jugador& j, int fase); // correctos con esa fase / impactos con esa fase
void inicializarJugador(Jugador& j, string nombre, bool bot);
