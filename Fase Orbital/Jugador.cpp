#include "Jugador.h"

// Esta función recibe un jugador y le pone sus valores iniciales
void inicializarJugador(Jugador& j, string nombre, bool bot) {
    j.alias = nombre;          // El punto significa: "dentro de j, guarda el alias"
    j.esBot = bot;             // Dentro de j, guarda si es bot o no
    j.vidaNucleo = 100;        // Vida inicial del núcleo según el proyecto[cite: 16]
    j.puntaje = 0;
    j.combo = 0;
    j.cargaPulso = 0;
    j.colorActivo = 0;         // Empieza en Cian
}