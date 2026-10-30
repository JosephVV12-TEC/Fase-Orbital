// Oleada.h - Genera la ColaSpawn de la oleada n segun la seccion 3.6 del PDF.
// No usa Allegro, asi que sirve igual para el modo grafico y el modo por lotes.
#pragma once
#include "ColaSpawn.h"

// Llena 'destino' (la vacia primero) con las apariciones de la oleada n.
void generarOleada(int n, unsigned int semillaPartida, ColaSpawn& destino);
