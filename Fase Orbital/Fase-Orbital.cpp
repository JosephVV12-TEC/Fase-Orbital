// Fase-Orbital.cpp - Punto de entrada: lee los argumentos y elige el modo 

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>       // en Debug de Visual Studio informa las fugas de memoria al salir
#endif

#include <fstream>
#include <iostream>
#include <string>

// Juego.h debe seguir incluido aqui: FuncionesJuego.h trae las definiciones de ListaEnemigos y ListaBalas
// y solo puede compilarse en UN .cpp.
#include "Juego.h"
#include "Partida.h"
#include "VistaAllegro.h"
#include "ModoLotes.h"
#include "Pruebas.h"

using namespace std;

namespace
{
    void mostrarAyuda()
    {
        cout << "Uso:\n"
                "  faseorbital                                  abre la interfaz grafica\n"
                "  faseorbital --lotes --jugadores Ana,Luis,BOT1,BOT2 --semilla 42 --oleadas 10\n"
                "              [--script partida.txt] [--historial historial.csv]\n"
                "  faseorbital --estadisticas historial.csv\n"
                "  faseorbital --ranking historial.csv\n"
                "  faseorbital --pruebas\n"
                "  faseorbital --generar-bots 30 historial.csv\n";
    }

    int modoLotes(int argc, char** argv)
    {
        OpcionesLote op;
        for (int i = 2; i < argc; ++i)
        {
            const string a = argv[i];
            const bool hayValor = (i + 1 < argc);
            if (a == "--jugadores" && hayValor) op.jugadores = argv[++i];
            else if (a == "--semilla" && hayValor) op.semilla = static_cast<unsigned int>(strtoul(argv[++i], nullptr, 10));
            else if (a == "--oleadas" && hayValor) op.oleadas = atoi(argv[++i]);
            else if (a == "--script" && hayValor) op.script = argv[++i];
            else if (a == "--historial" && hayValor) op.historial = argv[++i];
            else
            {
                cerr << "Argumento desconocido o sin valor: " << a << "\n";
                mostrarAyuda();
                return 1;
            }
        }
        if (op.jugadores.empty())
        {
            cerr << "Falta --jugadores\n";
            mostrarAyuda();
            return 1;
        }
        return ejecutarLotes(op, cout, cerr);
    }

    int modoReporte(const string& ruta, bool ranking)
    {
        ifstream existe(ruta.c_str());
        if (!existe)
        {
            cerr << "ERROR: no se pudo abrir " << ruta << "\n";
            return 1;
        }
        HistorialPartidas h;
        const int invalidas = cargarHistorial(h, ruta, cerr);   // las filas mal formadas se reportan y se omiten
        cout << h.tam << " filas cargadas de " << ruta << " (" << invalidas << " invalidas omitidas)\n\n";
        if (ranking)
            imprimirRanking(h, cout);
        else
            imprimirEstadisticas(h, cout);
        return 0;
    }
}

int main(int argc, char** argv)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (argc == 1)
        return ejecutarVistaAllegro();

    const string modo = argv[1];
    if (modo == "--lotes") return modoLotes(argc, argv);
    if (modo == "--estadisticas" && argc >= 3) return modoReporte(argv[2], false);
    if (modo == "--ranking" && argc >= 3) return modoReporte(argv[2], true);
    if (modo == "--pruebas") return ejecutarPruebas();
    if (modo == "--generar-bots" && argc >= 4)
        return generarPartidasBots(atoi(argv[2]), argv[3], 1000, 10, cerr);

    mostrarAyuda();
    return (modo == "--ayuda" || modo == "-h") ? 0 : 1;
}
