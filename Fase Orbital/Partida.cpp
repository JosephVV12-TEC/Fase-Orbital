#include "Partida.h"
#include "Oleada.h"
#include <cctype>

using namespace std;

bool esAliasBot(const string& alias)
{
    return alias.size() >= 3 &&
           toupper(static_cast<unsigned char>(alias[0])) == 'B' &&
           toupper(static_cast<unsigned char>(alias[1])) == 'O' &&
           toupper(static_cast<unsigned char>(alias[2])) == 'T';
}

void inicializarPartida(Partida& p)
{
    p.cantidad = 0;
    inicializarRonda(p.ronda);
    inicializarCola(p.colaOleada);
    p.semilla = 42;
    p.oleada = 0;
    p.limiteOleadas = 10;
    p.horario = true;
    p.turnosPendientes = 0;
    p.enCurso = false;
    p.ganador = -1;
    for (int i = 0; i < MAX_JUGADORES; ++i)
        p.oleadaAlcanzada[i] = 0;
}

void destruirPartida(Partida& p)
{
    vaciarRonda(p.ronda);   // libera los nodos, NO los jugadores (viven en el arreglo)
    vaciarCola(p.colaOleada);
    p.cantidad = 0;
    p.enCurso = false;
}

bool registrarJugador(Partida& p, const string& alias)
{
    if (p.cantidad >= MAX_JUGADORES)
        return false;

    inicializarJugador(p.jugadores[p.cantidad], alias, esAliasBot(alias));
    p.oleadaAlcanzada[p.cantidad] = 0;
    agregarRonda(p.ronda, &p.jugadores[p.cantidad]); // queda al final de la ronda
    p.cantidad++;
    return true;
}

int indiceDeJugador(const Partida& p, const Jugador* j)
{
    for (int i = 0; i < p.cantidad; ++i)
        if (&p.jugadores[i] == j)
            return i;
    return -1;
}

void comenzarPartida(Partida& p, Motor& m, unsigned int semilla, int limiteOleadas)
{
    p.semilla = semilla;
    p.limiteOleadas = limiteOleadas;
    p.oleada = 1;
    p.horario = true;
    p.turnosPendientes = p.ronda.tam;
    p.enCurso = true;
    p.ganador = -1;

    generarOleada(p.oleada, p.semilla, p.colaOleada); // la cola de la oleada se genera UNA vez
    iniciarTurnoActual(p, m);
}

void iniciarTurnoActual(Partida& p, Motor& m)
{
    Jugador* j = verActualRonda(p.ronda);
    int i = indiceDeJugador(p, j);
    if (i >= 0)
        p.oleadaAlcanzada[i] = p.oleada;

    // El Motor hace una copia profunda de la cola, la original queda intacta
    iniciarTurnoMotor(m, j, p.colaOleada);
}

// Orden por puntaje; empate: mayor oleada alcanzada.
// TODO (Estadisticas): como tercer criterio usar la mayor precision de color.
void calcularPosiciones(const Partida& p, int orden[MAX_JUGADORES])
{
    for (int i = 0; i < p.cantidad; ++i)
        orden[i] = i;

    // Insercion sobre un arreglo de 6 enteros (no se usa std::sort)
    for (int i = 1; i < p.cantidad; ++i)
    {
        int actual = orden[i];
        int k = i - 1;
        while (k >= 0)
        {
            const Jugador& a = p.jugadores[orden[k]];
            const Jugador& b = p.jugadores[actual];
            bool bMejor = (b.puntaje > a.puntaje) ||
                          (b.puntaje == a.puntaje && p.oleadaAlcanzada[actual] > p.oleadaAlcanzada[orden[k]]);
            if (!bMejor)
                break;
            orden[k + 1] = orden[k];
            --k;
        }
        orden[k + 1] = actual;
    }
}

ResultadoTurno terminarTurno(Partida& p, bool oleadaCompletada)
{
    Jugador* j = verActualRonda(p.ronda);

    // Reparacion del nucleo al completar la oleada: +8 (maximo 100)
    if (j != NULL && oleadaCompletada && j->vidaNucleo > 0)
    {
        j->vidaNucleo += 8;
        if (j->vidaNucleo > 100)
            j->vidaNucleo = 100;
    }

    p.turnosPendientes--;
    const bool ultimoDeLaRonda = (p.turnosPendientes <= 0);
    const bool murio = (j != NULL && j->vidaNucleo <= 0);

    // Orden serpenteante: la ronda siguiente va en sentido contrario y empieza con
    // el ultimo que jugo. Si el ultimo en jugar es el eliminado, el puntero debe
    // quedar en su vecino segun el sentido de la ronda siguiente.
    const bool sentidoDelPuntero = ultimoDeLaRonda ? !p.horario : p.horario;

    if (murio)
        eliminarActualRonda(p.ronda, sentidoDelPuntero);
    else if (!ultimoDeLaRonda)
        avanzarRonda(p.ronda, p.horario);

    if (!ultimoDeLaRonda)
        return ResultadoTurno::SigueRonda;

    // --- La ronda se completo ---
    const bool quedaUnoSolo = (p.ronda.tam == 1 && p.cantidad > 1);
    const bool sinJugadores = (p.ronda.tam == 0);
    const bool limite = (p.oleada >= p.limiteOleadas);

    if (sinJugadores || quedaUnoSolo || limite)
    {
        p.enCurso = false;
        int orden[MAX_JUGADORES];
        calcularPosiciones(p, orden);
        p.ganador = (p.cantidad > 0) ? orden[0] : -1;
        return ResultadoTurno::PartidaTerminada;
    }

    p.oleada++;
    p.horario = !p.horario;
    p.turnosPendientes = p.ronda.tam;
    generarOleada(p.oleada, p.semilla, p.colaOleada);
    return ResultadoTurno::NuevaRonda;
}
