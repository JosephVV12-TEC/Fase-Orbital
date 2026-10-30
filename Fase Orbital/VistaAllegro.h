// VistaAllegro.h - Interfaz grafica (Allegro 5) de Fase Orbital.
// La vista SOLO lee el estado del Motor y le envia acciones; ninguna regla del juego vive aqui.
#pragma once

// Abre la ventana, corre el bucle principal y libera todos los recursos.
// Devuelve 0 si termino bien y 1 si fallo la inicializacion (ya informo cual recurso falto).
int ejecutarVistaAllegro();
