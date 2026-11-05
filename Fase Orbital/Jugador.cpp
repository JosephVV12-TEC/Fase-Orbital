#include "Jugador.h"

// Esta función recibe un jugador y le pone sus valores iniciales
void inicializarJugador(Jugador& j, string nombre, bool bot) {
    j.alias = nombre;          // El punto significa: "dentro de j, guarda el alias"
    j.esBot = bot;             // Dentro de j, guarda si es bot o no
    j.vidaNucleo = 100;        // Vida inicial del núcleo según el proyecto
    j.puntaje = 0;
    j.combo = 0;
    j.cargaPulso = 0;
    j.colorActivo = 0; //Empieza en Cian
    reiniciarEstadisticas(j.est);
}

void reiniciarEstadisticas(EstadisticasJugador& e)
{
    //Basicamente esta funcion pone todo el 0 de nuevo
    e.disparos = 0;
    e.correctos = 0;
    e.erroneos = 0;
    e.comboMax = 0;
    e.danoTotal = 0;
    e.pulsos = 0;
    e.ticks = 0;
    for (int i = 0; i < 3; ++i)
    {
        e.impactosFase[i] = 0;
        e.correctosFase[i] = 0;
        e.bajasColor[i] = 0;
    }
    for (int i = 0; i < 6; ++i)
    {
        e.bajasTipo[i] = 0;
        e.danoTipo[i] = 0;
    }
}

double precisionColor(const Jugador& j)
{
    const int total = j.est.correctos + j.est.erroneos;
    if (total == 0)
        return 0.0;
    return static_cast<double>(j.est.correctos) / total;
}

double eficienciaDisparo(const Jugador& j)
{
    if (j.est.disparos == 0)
        return 0.0;
    return static_cast<double>(j.est.correctos) / j.est.disparos;
}

double precisionDeFase(const Jugador& j, int fase)
{
    if (fase < 0 || fase > 2 || j.est.impactosFase[fase] == 0)
        return 0.0;
    return static_cast<double>(j.est.correctosFase[fase]) / j.est.impactosFase[fase];
}