#include "Bot.h"

//Aca se especifican los valores fijos como la r minima, el r para el pulso, la cantidad de enemigos para el pulso y la carga del pulso
const double BOT_R_MINIMO = 100.0;
const double BOT_R_PULSO = 160.0;
const int BOT_ENEMIGOS_PULSO = 5;
const int BOT_CARGA_PULSO = 100;

AccionBot decidirBot(Motor& m)
{
    //Se pone como el caso base donde todo esta como estaria por defecto
    AccionBot accion;
    accion.hayObjetivo = false;
    accion.anguloObjetivo = m.anguloNave;
    accion.color = 0;
    accion.pulso = false;

    //Si no hay jugador no hay nada que decidir
    if (m.jugadorActual == NULL)
    {
        return accion;
    }
    accion.color = m.jugadorActual->colorActivo;

    NodoEnemigo* objetivo = NULL;//El mejor enemigo encontrado hasta ahora (empieza en NULL porque no hay ninguno)
    int cercanos = 0;//Cuantos enemigos hay con r < 160

    //Un solo recorrido de la lista de enemigos
    NodoEnemigo* n = m.listaEnemigos.cabeza;
	while (n != NULL) //Recorre la lista de enemigos hasta que llegue al final
    {
		if (n->dato.r > BOT_R_MINIMO) //Si el enemigo esta a mas de 100 de distancia, se considera como objetivo potencial
        {
            
            if (objetivo == NULL || n->dato.r < objetivo->dato.r)
            {
                objetivo = n;
            }
        }

        //Para el pulso se cuentan TODOS los enemigos cercanos, tengan o no r > 100
        if (n->dato.r < BOT_R_PULSO)
        {
            cercanos = cercanos + 1;
        }

        n = n->sig;
    }

    //Si hay un objetivo se gira hacia su angulo y se toma su color
    if (objetivo != NULL)
    {
        accion.hayObjetivo = true;
        accion.anguloObjetivo = objetivo->dato.theta;
        accion.color = objetivo->dato.color;
    }

    //Esto es para el pulso, pues carga completa y al menos 5 enemigos cerca (no depende de que haya objetivo)
    if (m.jugadorActual->cargaPulso >= BOT_CARGA_PULSO && cercanos >= BOT_ENEMIGOS_PULSO)
    {
        accion.pulso = true;
    }

    return accion;
}