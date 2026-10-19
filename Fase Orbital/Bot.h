#pragma once
#include "Motor.h"

//Aca se crea el bot que decide que se va ahacer en cada tick, en que direccion disparar, si disparar o no, y que color de bala disparar.
struct AccionBot 
{
    bool hayObjetivo;
    double anguloObjetivo;
    int color;
    bool pulso;
};

AccionBot decidirBot(Motor& m); //Esta funcion es la que decide que va a hacer el bot en cada tick, y retorna una estructura con la informacion de lo que va a hacer