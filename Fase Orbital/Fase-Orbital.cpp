// Fase-Orbital.cpp - Punto de entrada.
// Se conserva el #include "Juego.h" porque FuncionesJuego.h (que trae las definiciones de
// ListaEnemigos y ListaBalas) debe compilarse en UN solo .cpp. Toda la interfaz vive en VistaAllegro.cpp.
#include "Juego.h"
#include "VistaAllegro.h"

int main()
{
    return ejecutarVistaAllegro();
}
