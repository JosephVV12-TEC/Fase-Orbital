#include "RondaJugadores.h"

void inicializarRonda(RondaJugadores& r) 
{
    r.actual = NULL;
    r.tam = 0;
}


void agregarRonda(RondaJugadores& r, Jugador* j) //Esta funcion agrega un jugador a la ronda, el jugador se agrega antes del actual y si no hay jugadores en la ronda se agrega como el unico jugador y se apunta a si mismo en los dos sentidos (Anterior y Siguiente)
{

    
    PtrNodoRonda nuevo = new nodoRonda;
    nuevo->j = j;

    
	if (r.actual == NULL)//Si no hay jugadores en la ronda, el nuevo jugador se agrega como el unico jugador y se apunta a si mismo en los dos sentidos (Anterior y Siguiente)(ES EL CASO BASE)
    {
        nuevo->Anterior = nuevo;
        nuevo->Siguiente = nuevo;
        r.actual = nuevo;
    }

    
    else //Si no, se mete entre el que esta antes del actual y el actual
    {
        PtrNodoRonda previo = r.actual->Anterior;

        previo->Siguiente = nuevo;
        nuevo->Anterior = previo;
        nuevo->Siguiente = r.actual;
        r.actual->Anterior = nuevo;
    }

	r.tam = r.tam + 1; //Se modifica el tamaño de la ronda, ya que se agrego un jugador
}


void avanzarRonda(RondaJugadores& r, bool horario) //Basicamente pasa al siguiente jugador si la variable horario es true, o al anterior si es false
{

    //Si no hay jugadores, no hay nada que avanzar
    if (r.actual == NULL) 
    {
        return;
    }

    if (horario) {
        r.actual = r.actual->Siguiente;
    }
    else {
        r.actual = r.actual->Anterior;
    }
}


Jugador* eliminarActualRonda(RondaJugadores& r, bool horario)
{

    if (r.actual == NULL) {
        return NULL;
    }

    PtrNodoRonda viejo = r.actual;
    Jugador* eliminado = viejo->j;

   
    if (r.tam == 1) 
    {
        r.actual = NULL;
    }

    else 
    {
        
        viejo->Anterior->Siguiente = viejo->Siguiente;
        viejo->Siguiente->Anterior = viejo->Anterior;

        
        if (horario) {
            r.actual = viejo->Siguiente;
        }
        else {
            r.actual = viejo->Anterior;
        }
    }

	delete viejo;//Se libera el nodo, pero NO el jugador (para que quede disponible para datos y asi pero no dentro de la ronda)
    r.tam = r.tam - 1;
    return eliminado;
}


Jugador* verActualRonda(RondaJugadores& r)
{
    if (r.actual == NULL) 
    {
        return NULL;
    }
    return r.actual->j;
}


void vaciarRonda(RondaJugadores& r)
{

    //Como antes se explico, esta funcion es un circulo, entonces para vaciarlo hay que ir borrando los nodos uno por uno hasta que no quede ninguno, y para eso se guarda la cantidad de nodos que hay en la ronda antes de empezar a borrarlos, ya que si se hace un while con r.actual != NULL nunca va a terminar porque es un circulo y siempre va a haber un nodo siguiente, nunca un NULL
    int cantidad = r.tam;
    for (int i = 0; i < cantidad; i++) 
    {
        PtrNodoRonda aux = r.actual;
        r.actual = r.actual->Siguiente;   //Primero avanzamos, despues borramos
		delete aux; //Recordar SIEMPRE borrar los punteros que se crean con new, sino se genera memory leak
    }

    r.actual = NULL;
    r.tam = 0;
}


bool enlacesValidosRonda(RondaJugadores& r) 
{

    if (r.tam == 0) 
    {
        return r.actual == NULL;
    }
    if (r.actual == NULL)
    {
        return false;
    }

    PtrNodoRonda n = r.actual;
    for (int i = 0; i < r.tam; i++) 
    {
        if (n->Siguiente == NULL || n->Anterior == NULL) 
        {
            return false;
        }

        
        if (n->Siguiente->Anterior != n || n->Anterior->Siguiente != n) 
        {
            return false;
        }

        n = n->Siguiente;
    }

    
    return n == r.actual;
}