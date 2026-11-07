// ModoLotes.h - Partida completa SIN ventana (seccion 7.1). No depende de Allegro.
#pragma once
#include <string>
#include <iosfwd>

struct OpcionesLote
{
    std::string jugadores;            // "Ana,Luis,BOT1,BOT2" (1 a 6, separados por coma)
    unsigned int semilla = 42;
    int oleadas = 10;
    std::string script;               // ruta del script de acciones ("" = solo bots)
    std::string historial = "historial.csv";   // "" = no guardar
};

// Juega la partida completa, imprime el reporte en 'salida' y agrega las filas al CSV.
// Los avisos (lineas invalidas del script, errores) van a 'avisos'. Devuelve 0 si todo salio bien.
int ejecutarLotes(const OpcionesLote& op, std::ostream& salida, std::ostream& avisos);

// Genera n partidas de bots (para tener historial.csv con >= 30 partidas). Devuelve 0 si salio bien.
int generarPartidasBots(int cantidad, const std::string& rutaCSV, unsigned int semillaInicial, int oleadas, std::ostream& avisos);
