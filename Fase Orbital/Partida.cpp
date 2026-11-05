#include "Partida.h"
#include "Oleada.h"
#include <cctype>
#include <iomanip>

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
    vaciarBitacora(p.bitacora);
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
    vaciarBitacora(p.bitacora);
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
    iniciarTurnoMotor(m, j, p.colaOleada, p.oleada, i, &p.bitacora);
}

// Orden por puntaje; empate: mayor oleada alcanzada.TODO (Estadisticas): como tercer criterio usar la mayor precision de color.
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
            const int oleadaB = p.oleadaAlcanzada[actual];
            const int oleadaA = p.oleadaAlcanzada[orden[k]];
            bool bMejor = (b.puntaje > a.puntaje) ||
                (b.puntaje == a.puntaje && oleadaB > oleadaA) ||
                (b.puntaje == a.puntaje && oleadaB == oleadaA && precisionColor(b) > precisionColor(a));
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

namespace
{
    const char* NOMBRES_TIPO[6] = { "Dron", "Girador", "Divisor", "Mini", "Tanque", "Camaleon" };
    const char* NOMBRES_COLOR[3] = { "C", "M", "A" };

    double pct(double fraccion)
    {
        return fraccion * 100.0;
    }
}

//El reporte de la partida se imprime en un flujo de salida
void imprimirReportePartida(const Partida& p, ostream& os)
{
    const ios::fmtflags flagsGuardados = os.flags();
    const streamsize precisionGuardada = os.precision();

    int orden[MAX_JUGADORES];
    calcularPosiciones(p, orden);

    os << fixed << setprecision(1);
    os << "=== REPORTE DE LA PARTIDA ===\n";
    os << "Semilla: " << p.semilla << "   Oleada final: " << p.oleada << "   Limite: " << p.limiteOleadas << "\n";
    if (p.ganador >= 0)
        os << "Ganador: " << p.jugadores[p.ganador].alias << "\n";

    // ---- Posiciones ----
    os << "\nPOSICIONES\n";
    os << left << setw(5) << "Pos" << setw(14) << "Jugador" << setw(8) << "Tipo"
        << right << setw(9) << "Puntaje" << setw(8) << "Oleada" << setw(8) << "Nucleo" << "\n";
    for (int k = 0; k < p.cantidad; ++k)
    {
        const int i = orden[k];
        const Jugador& j = p.jugadores[i];
        os << left << setw(5) << (k + 1) << setw(14) << j.alias << setw(8) << (j.esBot ? "BOT" : "HUMANO")
            << right << setw(9) << j.puntaje << setw(8) << p.oleadaAlcanzada[i] << setw(8) << j.vidaNucleo << "\n";
    }

    // ---- Metricas por jugador (6.1) ----
    os << "\nMETRICAS POR JUGADOR\n";
    for (int k = 0; k < p.cantidad; ++k)
    {
        const int i = orden[k];
        const Jugador& j = p.jugadores[i];
        const EstadisticasJugador& e = j.est;

        os << (k + 1) << ". " << j.alias << (j.esBot ? " (BOT)" : "") << "\n";
        os << "  Puntaje: " << j.puntaje << "   Oleada alcanzada: " << p.oleadaAlcanzada[i] << "\n";
        os << "  Disparos: " << e.disparos << "   Impactos correctos: " << e.correctos
            << "   Impactos erroneos: " << e.erroneos << "\n";
        os << "  Precision de color: " << pct(precisionColor(j)) << "%   Eficiencia de disparo: "
            << pct(eficienciaDisparo(j)) << "%\n";
        os << "  Precision por color:";
        for (int c = 0; c < 3; ++c)
            os << " " << NOMBRES_COLOR[c] << " " << pct(precisionDeFase(j, c)) << "%" << (c < 2 ? " |" : "");
        os << "\n";
        os << "  Bajas por tipo:";
        for (int t = 0; t < 6; ++t)
            os << " " << NOMBRES_TIPO[t] << " " << e.bajasTipo[t] << (t < 5 ? " |" : "");
        os << "\n";
        os << "  Bajas por color:";
        for (int c = 0; c < 3; ++c)
            os << " " << NOMBRES_COLOR[c] << " " << e.bajasColor[c] << (c < 2 ? " |" : "");
        os << "\n";
        os << "  Combo maximo: " << e.comboMax << "   Pulsos usados: " << e.pulsos
            << "   Ticks sobrevividos: " << e.ticks << "\n";
        os << "  Dano recibido: " << e.danoTotal << " (";
        for (int t = 0; t < 6; ++t)
            os << NOMBRES_TIPO[t] << " " << e.danoTipo[t] << (t < 5 ? " | " : "");
        os << ")\n";
    }

    // ---- Linea de tiempo por oleada, calculada recorriendo la bitacora (6.2) ----
    os << "\nLINEA DE TIEMPO POR OLEADA\n";
    for (int k = 0; k < p.cantidad; ++k)
    {
        const int i = orden[k];
        os << p.jugadores[i].alias << "\n";
        os << "  " << left << setw(8) << "Oleada" << right << setw(8) << "Bajas" << setw(8) << "Dano"
            << setw(12) << "Puntaje" << "  Estado\n";

        int acumulado = 0;
        for (int o = 1; o <= p.oleadaAlcanzada[i]; ++o)
        {
            const int bajas = contarEventos(p.bitacora, i, o, TipoEvento::BAJA);
            const int dano = sumarValorEventos(p.bitacora, i, o, TipoEvento::DANO_NUCLEO)
                + sumarValorEventos(p.bitacora, i, o, TipoEvento::CHOQUE_NAVE);
            acumulado += sumarValorEventos(p.bitacora, i, o, TipoEvento::BAJA);

            const char* estado = "";
            if (contarEventos(p.bitacora, i, o, TipoEvento::ELIMINACION) > 0)
                estado = "eliminado";
            else if (contarEventos(p.bitacora, i, o, TipoEvento::FIN_OLEADA) > 0)
                estado = "completada";

            os << "  " << left << setw(8) << o << right << setw(8) << bajas << setw(8) << dano
                << setw(12) << acumulado << "  " << estado << "\n";
        }
    }

    os.flags(flagsGuardados);
    os.precision(precisionGuardada);
}