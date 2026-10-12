#include "Motor.h"

void inicializarMotor(Motor& m) {
    m.jugadorActual = nullptr;
    m.tickActual = 0;
    m.anguloNave = 270.0; // Ángulo inicial de la nave[cite: 16]
}

void iniciarTurnoMotor(Motor& m, Jugador* nuevoJugador, ColaSpawn colaOleada) {
    m.jugadorActual = nuevoJugador;
    m.colaSpawnOleada = colaOleada; // Copia la cola para este jugador[cite: 20]
    m.tickActual = 0;
    m.anguloNave = 270.0;
}

void procesarTickMotor(Motor& m, int nuevoColor) {
    // Si no hay jugador o ya perdió, no hace nada
    if (m.jugadorActual == nullptr) {
        return;
    }

    if (m.jugadorActual->vidaNucleo <= 0) {
        return;
    }

    // Avanza el tiempo del juego (un tick)
    m.tickActual = m.tickActual + 1;

    // Cambia el color de la nave si el jugador presionó una tecla válida (0, 1 o 2)
    if (nuevoColor >= 0 && nuevoColor <= 2) {
        m.jugadorActual->colorActivo = nuevoColor;
    }
}