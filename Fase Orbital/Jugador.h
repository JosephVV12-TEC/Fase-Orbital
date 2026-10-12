#pragma once
#include <string>
using namespace std;

struct Jugador {
    string alias;
    bool esBot;
    int vidaNucleo;
    int puntaje;
    int combo;
    int cargaPulso;
    int colorActivo; // 0 = Cian, 1 = Magenta, 2 = Ámbar
};