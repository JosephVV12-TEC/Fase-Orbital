#include "Oleada.h"
#include <cstdint>

namespace
{
    // Generador congruencial lineal de la seccion 3.6:
    // x(k+1) = (1103515245 * x(k) + 12345) mod 2^31
    struct Lcg
    {
        uint32_t x;
        uint32_t siguiente()
        {
            x = (1103515245u * x + 12345u) & 0x7FFFFFFFu;
            return x;
        }
    };

    // Valores de TipoEnemigo: Dron 0, Girador 1, Divisor 2, Mini 3, Tanque 4, Camaleon 5
    const int T_DRON = 0, T_GIRADOR = 1, T_DIVISOR = 2, T_TANQUE = 4, T_CAMALEON = 5;
}

void generarOleada(int n, unsigned int semillaPartida, ColaSpawn& destino)
{
    vaciarCola(destino);

    Lcg g;
    g.x = static_cast<uint32_t>(semillaPartida) * 1000u + static_cast<uint32_t>(n);

    // Conjunto de tipos disponibles, en el orden que indica el PDF
    int tipos[8];
    int cantTipos = 0;
    tipos[cantTipos++] = T_DRON;
    tipos[cantTipos++] = T_DRON;
    if (n >= 2) tipos[cantTipos++] = T_GIRADOR;
    if (n >= 3) tipos[cantTipos++] = T_DIVISOR;
    if (n >= 4) tipos[cantTipos++] = T_TANQUE;
    if (n >= 5) { tipos[cantTipos++] = T_CAMALEON; tipos[cantTipos++] = T_GIRADOR; }

    const int total = 6 + 3 * n;
    int base = 35 - 2 * n;
    if (base < 9) base = 9;

    int tick = 60; // la primera aparicion ocurre en el tick 60
    for (int i = 0; i < total; ++i)
    {
        Spawn s;
        s.tickSpawn = tick;
        s.tipoEnemigo = tipos[g.siguiente() % static_cast<uint32_t>(cantTipos)];
        s.color = static_cast<int>(g.siguiente() % 3u);
        s.angulo = static_cast<int>(g.siguiente() % 360u);
        s.sentido = 0;
        if (s.tipoEnemigo == T_GIRADOR)
            s.sentido = static_cast<int>(g.siguiente() % 2u); // 0 = horario
        encolar(destino, s);

        // Intervalo real = floor(base * (60 + x mod 81) / 100)
        int intervalo = (base * (60 + static_cast<int>(g.siguiente() % 81u))) / 100;
        // Desde la oleada 7, si x mod 4 = 0 la siguiente aparicion es en el mismo tick
        if (n >= 7 && (g.siguiente() % 4u) == 0u)
            intervalo = 0;
        tick += intervalo;
    }
}
