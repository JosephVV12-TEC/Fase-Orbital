#include "ModoLotes.h"
#include "Partida.h"
#include "Bot.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

using namespace std;

namespace
{
    const int MAX_TICKS_TURNO = 20000;    // seguro contra turnos que no terminan (p. ej. script que dispara siempre con otro color)

 
    enum class TipoAccion { Color, GirarA, Pulso };

    struct AccionScript
    {
        string jugador;
        int oleada;
        int tick;
        TipoAccion tipo;
        double parametro;
        AccionScript* sig;
    };

    class Script
    {
        AccionScript* cabeza;
        AccionScript* cola;
    public:
        Script() : cabeza(nullptr), cola(nullptr) {}
        ~Script()
        {
            while (cabeza != nullptr)
            {
                AccionScript* s = cabeza->sig;
                delete cabeza;
                cabeza = s;
            }
        }
        Script(const Script&) = delete;
        Script& operator=(const Script&) = delete;

        void agregar(const AccionScript& a)
        {
            AccionScript* n = new AccionScript(a);
            n->sig = nullptr;
            if (cola == nullptr) cabeza = n; else cola->sig = n;
            cola = n;
        }

        // Lee el archivo; las lineas invalidas se reportan con su numero y se omiten. false si no abre.
        bool cargar(const string& ruta, ostream& avisos)
        {
            ifstream in(ruta.c_str());
            if (!in.is_open()) return false;

            string linea;
            int numLinea = 0;
            while (getline(in, linea))
            {
                ++numLinea;
                if (!linea.empty() && linea[linea.size() - 1] == '\r') linea.erase(linea.size() - 1);

                istringstream ss(linea);
                string jugador, accion, param;
                int oleada, tick;
                if (!(ss >> jugador) || jugador[0] == '#')
                    continue;                                  // vacia o comentario

                if (!(ss >> oleada >> tick >> accion) || oleada < 1 || tick < 0)
                {
                    avisos << "AVISO: linea " << numLinea << " del script omitida (formato: jugador oleada tick ACCION [parametro])\n";
                    continue;
                }
                ss >> param;

                AccionScript a;
                a.jugador = jugador; a.oleada = oleada; a.tick = tick; a.parametro = 0.0; a.sig = nullptr;

                if (accion == "COLOR" && (param == "C" || param == "c" || param == "0")) { a.tipo = TipoAccion::Color; a.parametro = 0; }
                else if (accion == "COLOR" && (param == "M" || param == "m" || param == "1")) { a.tipo = TipoAccion::Color; a.parametro = 1; }
                else if (accion == "COLOR" && (param == "A" || param == "a" || param == "2")) { a.tipo = TipoAccion::Color; a.parametro = 2; }
                else if (accion == "GIRAR_A" && !param.empty())
                {
                    char* fin = nullptr;
                    const double grados = strtod(param.c_str(), &fin);
                    if (*fin != '\0')
                    {
                        avisos << "AVISO: linea " << numLinea << " del script omitida (angulo invalido: " << param << ")\n";
                        continue;
                    }
                    a.tipo = TipoAccion::GirarA;
                    a.parametro = grados;
                }
                else if (accion == "PULSO") a.tipo = TipoAccion::Pulso;
                else
                {
                    avisos << "AVISO: linea " << numLinea << " del script omitida (accion o parametro invalido: " << accion << " " << param << ")\n";
                    continue;
                }
                agregar(a);
            }
            return true;
        }

        // Aplica las acciones de ese jugador / oleada / tick (en el orden del archivo)
        void aplicar(const string& jugador, int oleada, int tick, Motor& m, int& colorNuevo) const
        {
            for (AccionScript* a = cabeza; a != nullptr; a = a->sig)
            {
                if (a->oleada != oleada || a->tick != tick || a->jugador != jugador) continue;
                switch (a->tipo)
                {
                    case TipoAccion::Color: colorNuevo = static_cast<int>(a->parametro); break;
                    case TipoAccion::GirarA: m.anguloObjetivo = a->parametro; break;
                    case TipoAccion::Pulso: m.pulsoSolicitado = true; break;
                }
            }
        }
    };

    // Separa "Ana,Luis,BOT1" en la partida. false si hay un error (se explica en avisos).
    bool registrarDesdeLista(Partida& p, const string& lista, ostream& avisos)
    {
        size_t inicio = 0;
        while (inicio <= lista.size())
        {
            size_t coma = lista.find(',', inicio);
            if (coma == string::npos) coma = lista.size();
            const string alias = lista.substr(inicio, coma - inicio);
            inicio = coma + 1;

            if (alias.empty()) { avisos << "ERROR: alias vacio en --jugadores\n"; return false; }
            for (int i = 0; i < p.cantidad; ++i)
                if (p.jugadores[i].alias == alias) { avisos << "ERROR: alias repetido: " << alias << "\n"; return false; }
            if (!registrarJugador(p, alias)) { avisos << "ERROR: maximo " << MAX_JUGADORES << " jugadores\n"; return false; }
            if (coma >= lista.size()) break;
        }
        return p.cantidad >= 1;
    }

    // Juega un turno completo del jugador actual (bot o script). true si limpio la oleada.
    bool jugarTurno(Partida& p, Motor& m, const Script& script, bool hayScript)
    {
        Jugador* j = m.jugadorActual;
        int ticks = 0;
        ResultadoTick estado = ResultadoTick::Sigue;
        while (estado == ResultadoTick::Sigue && ticks < MAX_TICKS_TURNO)
        {
            int color = -1;
            if (j->esBot)
            {
                const AccionBot a = decidirBot(m);          // heuristica fija (seccion 5.2)
                m.anguloObjetivo = a.anguloObjetivo;
                m.pulsoSolicitado = a.pulso;
                color = a.color;
            }
            else if (hayScript)
            {
                script.aplicar(j->alias, p.oleada, m.tickActual, m, color);
            }
            estado = procesarTickMotor(m, color);   // el Motor dice si la oleada termino o el nucleo cayo
            ++ticks;
        }
        return estado == ResultadoTick::OleadaCompleta;
    }
}

int ejecutarLotes(const OpcionesLote& op, ostream& salida, ostream& avisos)
{
    if (op.oleadas < 1 || op.oleadas > 200)
    {
        avisos << "ERROR: --oleadas debe estar entre 1 y 200\n";
        return 1;
    }

    Partida p;
    inicializarPartida(p);
    if (!registrarDesdeLista(p, op.jugadores, avisos))
    {
        avisos << "ERROR: use --jugadores Ana,Luis,BOT1 (1 a 6 alias distintos)\n";
        return 1;
    }

    Script script;
    bool hayScript = false;
    if (!op.script.empty())
    {
        if (!script.cargar(op.script, avisos))
        {
            avisos << "ERROR: no se pudo abrir el script " << op.script << "\n";
            return 1;
        }
        hayScript = true;
    }

    Motor m;
    inicializarMotor(m);
    comenzarPartida(p, m, op.semilla, op.oleadas);

    bool terminada = false;
    while (!terminada)
    {
        const bool completada = jugarTurno(p, m, script, hayScript);
        const ResultadoTurno r = terminarTurno(p, completada);
        if (r == ResultadoTurno::PartidaTerminada)
            terminada = true;
        else
            iniciarTurnoActual(p, m);
    }

    imprimirReportePartida(p, salida);

    int codigo = 0;
    if (!op.historial.empty())
    {
        // Se carga el historial existente (las lineas invalidas se reportan) para calcular el id y se agregan las filas
        HistorialPartidas h;
        cargarHistorial(h, op.historial, avisos);
        if (guardarPartidaEnHistorial(h, p, "lotes", op.historial))
            salida << "\nPartida agregada a " << op.historial << "\n";
        else
        {
            avisos << "ERROR: no se pudo escribir " << op.historial << "\n";
            codigo = 1;
        }
    }

    vaciarCola(m.colaSpawnOleada);
    destruirPartida(p);
    return codigo;
}

int generarPartidasBots(int cantidad, const string& rutaCSV, unsigned int semillaInicial, int oleadas, ostream& avisos)
{
    for (int i = 0; i < cantidad; ++i)
    {
        OpcionesLote op;
        op.jugadores = "BOT1,BOT2,BOT3,BOT4";
        op.semilla = semillaInicial + static_cast<unsigned int>(i);
        op.oleadas = oleadas;
        op.historial = rutaCSV;
        ostringstream descartar;               // el reporte de cada partida no se imprime
        const int r = ejecutarLotes(op, descartar, avisos);
        if (r != 0) return r;
    }
    return 0;
}
