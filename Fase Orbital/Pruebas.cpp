#include "Pruebas.h"
#include "ListaEnemigos.h"
#include "ListaBalas.h"
#include "ColaSpawn.h"
#include "RondaJugadores.h"
#include "Motor.h"
#include "Partida.h"       // historial, estadisticas y ranking del equipo viven aqui
#include "ModoLotes.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cmath>

using namespace std;

namespace
{
    int pasaron = 0;
    int total = 0;

    void resultado(const char* id, const char* descripcion, bool ok)
    {
        ++total;
        if (ok) ++pasaron;
        cout << (ok ? "[ OK ] " : "[FALLA] ") << id << "  " << descripcion << endl;
    }

    Enemigo enemigo(int id, TipoEnemigo tipo, int color, double r, double theta, int vida, int radio)
    {
        Enemigo e;
        e.id = id; e.tipo = tipo; e.color = color; e.r = r; e.theta = theta; e.vida = vida; e.radio = radio;
        return e;
    }

    // Motor listo con un jugador y una cola vacia, para probar reglas de un solo tick
    struct Escenario
    {
        Motor m;
        Jugador j;
        BitacoraEventos bitacora;
        ColaSpawn vacia;
        Escenario()
        {
            inicializarMotor(m);
            inicializarJugador(j, "Prueba", false);
            iniciarTurnoMotor(m, &j, vacia, 1, 0, &bitacora);   // oleada 1, jugador 0
        }
    };

    // ---------------- P01 - P03: ListaEnemigos ----------------
    void p01()
    {
        ListaEnemigos lista;
        for (int i = 1; i <= 1000; ++i)
            lista.insertarFinal(enemigo(i, TipoEnemigo::Dron, 0, 100, 0, 1, 6));

        NodoEnemigo* n = lista.primero();
        while (n != nullptr)
            n = (n->dato.id % 2 == 0) ? lista.eliminar(n) : n->sig;    // eliminar devuelve el siguiente

        bool ok = (lista.tamano() == 500);
        int adelante[500], atras[500], a = 0, b = 499;
        for (NodoEnemigo* x = lista.primero(); x != nullptr && a < 500; x = x->sig) adelante[a++] = x->dato.id;
        for (NodoEnemigo* x = lista.ultimo(); x != nullptr && b >= 0; x = x->ant) atras[b--] = x->dato.id;
        ok = ok && a == 500 && b == -1;
        for (int i = 0; ok && i < 500; ++i)
            ok = (adelante[i] == atras[i]) && (adelante[i] % 2 == 1);
        resultado("P01", "ListaEnemigos: 1000 nodos, eliminar los pares durante el recorrido -> tamano 500, ida y vuelta coinciden", ok);
    }

    void p02()
    {
        ListaEnemigos lista;
        NodoEnemigo* a = lista.insertarFinal(enemigo(1, TipoEnemigo::Dron, 0, 1, 0, 1, 1));
        lista.insertarFinal(enemigo(2, TipoEnemigo::Dron, 0, 1, 0, 1, 1));
        NodoEnemigo* c = lista.insertarFinal(enemigo(3, TipoEnemigo::Dron, 0, 1, 0, 1, 1));

        lista.eliminar(a);                                              // cabeza
        bool ok = lista.primero()->dato.id == 2 && lista.primero()->ant == nullptr && lista.tamano() == 2;
        lista.eliminar(c);                                              // cola
        ok = ok && lista.ultimo()->dato.id == 2 && lista.ultimo()->sig == nullptr && lista.tamano() == 1;
        lista.eliminar(lista.primero());                                // nodo unico
        ok = ok && lista.primero() == nullptr && lista.ultimo() == nullptr && lista.vacia();
        resultado("P02", "ListaEnemigos: eliminar cabeza, cola y nodo unico -> punteros correctos, lista vacia con ambos nulos", ok);
    }

    void p03()
    {
        ListaEnemigos lista;
        lista.insertarFinal(enemigo(1, TipoEnemigo::Dron, 0, 1, 0, 1, 1));
        NodoEnemigo* ultimo = lista.insertarFinal(enemigo(2, TipoEnemigo::Dron, 0, 1, 0, 1, 1));
        NodoEnemigo* nuevo = lista.insertarDespues(ultimo, enemigo(3, TipoEnemigo::Mini, 0, 1, 0, 1, 1));
        bool ok = lista.ultimo() == nuevo && nuevo->sig == nullptr && nuevo->ant == ultimo && ultimo->sig == nuevo && lista.tamano() == 3;
        resultado("P03", "ListaEnemigos: insertarDespues sobre el ultimo nodo -> la cola se actualiza", ok);
    }

    // ---------------- P04: ListaBalas ----------------
    void p04()
    {
        ListaBalas balas;
        for (int i = 0; i < 10; ++i)       // la ultima insertada queda en la cabeza: orden 9,8,...,0
        {
            Bala b; b.color = i;
            balas.insertarInicio(b);
        }
        // Se marcan 9, 8, 7 (cabeza y consecutivas) y 4, 3 (consecutivas en medio)
        const int eliminadas = balas.eliminarSi([](Bala& b) { return b.color >= 7 || b.color == 4 || b.color == 3; });

        const int esperado[5] = { 6, 5, 2, 1, 0 };
        bool ok = (eliminadas == 5) && (balas.tam == 5);
        int k = 0;
        for (NodoBala* n = balas.cabeza; n != nullptr && k < 5; n = n->sig, ++k)
            ok = ok && (n->dato.color == esperado[k]);
        ok = ok && (k == 5);
        resultado("P04", "ListaBalas: eliminar balas consecutivas incluida la cabeza con NodoBala** -> ninguna marcada sobrevive, ninguna valida se pierde", ok);
    }

    // ---------------- P05 - P06: ColaSpawn ----------------
    Spawn spawn(int tick) { Spawn s; s.tipoEnemigo = 0; s.tickSpawn = tick; s.color = 0; s.angulo = 0; s.sentido = 0; return s; }

    void p05()
    {
        ColaSpawn cola;
        for (int i = 0; i < 4; ++i) encolar(cola, spawn(i));
        Spawn s;
        bool ok = true;
        for (int i = 0; i < 4; ++i)
            ok = ok && desencolar(cola, s) && s.tickSpawn == i;          // sale en orden FIFO
        ok = ok && cola.frente == nullptr && cola.final == nullptr && colaVacia(cola) && !desencolar(cola, s) && verFrente(cola) == nullptr;
        resultado("P05", "ColaSpawn: encolar y desencolar hasta vaciar -> frente y final nulos; desencolar en vacia retorna falso", ok);
    }

    void p06()
    {
        ColaSpawn original;
        for (int i = 0; i < 5; ++i) encolar(original, spawn(i * 10));

        bool ok = true;
        {
            ColaSpawn copia(original);                                  // constructor de copia
            ColaSpawn asignada;
            encolar(asignada, spawn(999));
            asignada = original;                                        // operator= (libera lo previo)
            asignada = asignada;                                        // autoasignacion

            Spawn s;
            while (desencolar(copia, s)) {}                             // se consume la copia
            ok = ok && colaVacia(copia) && asignada.tam == 5;
            ok = ok && (copia.frente == nullptr) && (original.frente != asignada.frente);   // nodos distintos
        }                                                               // se destruyen las copias
        int k = 0;
        for (PtrNodoSpawn n = original.frente; n != nullptr; n = n->siguiente, ++k)
            ok = ok && n->dato.tickSpawn == k * 10;
        ok = ok && k == 5 && original.tam == 5;
        resultado("P06", "ColaSpawn: copiar, consumir la copia, destruir ambas -> original intacta, sin doble liberacion", ok);
    }

    // ---------------- P07: RondaJugadores ----------------
    void p07()
    {
        Jugador a, b, c;
        inicializarJugador(a, "A", false);
        inicializarJugador(b, "B", false);
        inicializarJugador(c, "C", false);

        RondaJugadores r;
        agregarRonda(r, &a); agregarRonda(r, &b); agregarRonda(r, &c);
        bool ok = enlacesValidosRonda(r) && r.tam == 3 && verActualRonda(r) == &a;

        avanzarRonda(r, true);  ok = ok && verActualRonda(r) == &b && enlacesValidosRonda(r);
        avanzarRonda(r, true);  ok = ok && verActualRonda(r) == &c;
        avanzarRonda(r, true);  ok = ok && verActualRonda(r) == &a;                 // vuelve al inicio (circular)
        avanzarRonda(r, false); ok = ok && verActualRonda(r) == &c && enlacesValidosRonda(r);   // antihorario
        avanzarRonda(r, false); ok = ok && verActualRonda(r) == &b;

        Jugador* quitado = eliminarActualRonda(r, true);                            // elimina B
        ok = ok && quitado == &b && r.tam == 2 && enlacesValidosRonda(r) && verActualRonda(r) == &c;
        quitado = eliminarActualRonda(r, false);                                    // elimina C (hacia atras)
        ok = ok && quitado == &c && r.tam == 1 && verActualRonda(r) == &a && enlacesValidosRonda(r);
        avanzarRonda(r, true);  ok = ok && verActualRonda(r) == &a && enlacesValidosRonda(r);   // unico jugador
        quitado = eliminarActualRonda(r, true);                                     // el unico jugador
        ok = ok && quitado == &a && r.tam == 0 && r.actual == nullptr && enlacesValidosRonda(r);
        ok = ok && eliminarActualRonda(r, true) == nullptr;                         // ronda vacia
        resultado("P07", "RondaJugadores: eliminar el actual, el unico jugador y rotar en ambos sentidos -> enlaces circulares validos", ok);
    }

    // ---------------- P08: reproducibilidad ----------------
    void p08()
    {
        // 1) Solo bots
        OpcionesLote op;
        op.jugadores = "BOT1,BOT2,BOT3";
        op.semilla = 42;
        op.oleadas = 6;
        op.historial = "";
        ostringstream r1, r2, avisos;
        const int c1 = ejecutarLotes(op, r1, avisos);
        const int c2 = ejecutarLotes(op, r2, avisos);
        bool ok = (c1 == 0 && c2 == 0 && r1.str() == r2.str() && !r1.str().empty());

        // 2) Humanos controlados por script + un bot
        const char* ruta = "prueba_script_p08.txt";
        {
            ofstream f(ruta);
            f << "# jugador oleada tick ACCION [parametro]\n"
                 "Ana 1 0 COLOR M\n"
                 "Ana 1 75 GIRAR_A 135\n"
                 "Ana 1 140 COLOR A\n"
                 "Ana 2 300 PULSO\n"
                 "Luis 1 10 GIRAR_A 45\n"
                 "linea invalida de prueba\n";
        }
        OpcionesLote op2;
        op2.jugadores = "Ana,Luis,BOT1";
        op2.semilla = 7;
        op2.oleadas = 3;
        op2.script = ruta;
        op2.historial = "";
        ostringstream s1, s2, av1, av2;
        const int d1 = ejecutarLotes(op2, s1, av1);
        const int d2 = ejecutarLotes(op2, s2, av2);
        ok = ok && d1 == 0 && d2 == 0 && s1.str() == s2.str() && av1.str() == av2.str();
        ok = ok && av1.str().find("linea 7") != string::npos;        // la linea invalida se reporto con su numero
        remove(ruta);
        resultado("P08", "Motor: dos ejecuciones con la misma semilla (y con el mismo script) -> reportes identicos", ok);
    }

    // ---------------- P09 - P11: reglas del Motor ----------------
    void p09()
    {
        Escenario e;
        e.j.combo = 7;
        e.m.listaEnemigos.insertarFinal(enemigo(1, TipoEnemigo::Dron, 0, 200.0, 0.0, 1, 6));   // enemigo CIAN en (200, 0)
        Bala b; b.x = 190.0; b.y = 0.0; b.vx = 9.0; b.vy = 0.0; b.color = 1;                   // bala MAGENTA
        e.m.listaBalas.insertarInicio(b);

        const int vidaAntes = e.j.vidaNucleo;
        procesarTickMotor(e.m, -1);

        // el Dron avanza 0.88 u en el tick (v = 0.8 + 0.08) y luego retrocede 9 u por el impacto
        const double esperado = 200.0 - 0.88 + 9.0;
        NodoEnemigo* n = e.m.listaEnemigos.primero();
        bool ok = n != nullptr && std::fabs(n->dato.r - esperado) < 1e-9 && n->dato.vida == 1
                  && e.j.combo == 0 && e.j.vidaNucleo == vidaAntes && e.j.est.erroneos == 1
                  && contarEventos(e.bitacora, 0, 1, TipoEvento::IMPACTO_ERRONEO) == 1;
        resultado("P09", "Motor: impacto con color incorrecto -> r aumenta 9, combo = 0, vida sin cambio", ok);
    }

    void p10()
    {
        Escenario e;
        e.m.listaEnemigos.insertarFinal(enemigo(1, TipoEnemigo::Dron, 0, 250.0, 90.0, 1, 6));
        e.m.listaEnemigos.insertarFinal(enemigo(2, TipoEnemigo::Divisor, 1, 200.0, 0.0, 1, 8));   // Divisor MAGENTA
        e.m.listaEnemigos.insertarFinal(enemigo(3, TipoEnemigo::Dron, 0, 250.0, 180.0, 1, 6));
        Bala b; b.x = 190.0; b.y = 0.0; b.vx = 9.0; b.vy = 0.0; b.color = 1;                      // bala MAGENTA
        e.m.listaBalas.insertarInicio(b);

        procesarTickMotor(e.m, -1);

        NodoEnemigo* n0 = e.m.listaEnemigos.primero();
        NodoEnemigo* n1 = n0 ? n0->sig : nullptr;
        NodoEnemigo* n2 = n1 ? n1->sig : nullptr;
        NodoEnemigo* n3 = n2 ? n2->sig : nullptr;
        bool ok = e.m.listaEnemigos.tamano() == 4 && n0 && n1 && n2 && n3
                  && n0->dato.id == 1 && n3->dato.id == 3                                          // los vecinos siguen en su lugar
                  && n1->dato.tipo == TipoEnemigo::Mini && n2->dato.tipo == TipoEnemigo::Mini      // Mini en la posicion del Divisor
                  && n1->dato.color != 1 && n2->dato.color != 1 && n1->dato.color != n2->dato.color   // colores distintos al del padre
                  && std::fabs(n1->dato.theta - 356.0) < 1e-9 && std::fabs(n2->dato.theta - 4.0) < 1e-9
                  && e.j.combo == 1 && e.j.puntaje == 200;
        resultado("P10", "Motor: baja de un Divisor -> dos Mini en su posicion, con los dos colores distintos al del padre", ok);
    }

    void p11()
    {
        // Carga 99: el pulso no hace nada
        bool ok = true;
        {
            Escenario e;
            e.j.cargaPulso = 99;
            e.m.pulsoSolicitado = true;
            e.m.listaEnemigos.insertarFinal(enemigo(1, TipoEnemigo::Dron, 0, 33.0, 0.0, 1, 6));
            procesarTickMotor(e.m, -1);
            ok = ok && !e.m.ondaActiva && e.m.listaEnemigos.tamano() == 1 && e.j.cargaPulso == 99 && e.j.est.pulsos == 0;
        }
        // Carga 100: nace la onda y destruye lo que esta en la banda (|r - r_onda| < radio + 3), sin importar el color
        {
            Escenario e;
            e.j.cargaPulso = 100;
            e.j.combo = 3;
            e.m.pulsoSolicitado = true;
            e.m.listaEnemigos.insertarFinal(enemigo(1, TipoEnemigo::Dron, 2, 33.0, 0.0, 1, 6));     // en la banda (color ambar)
            e.m.listaEnemigos.insertarFinal(enemigo(2, TipoEnemigo::Dron, 0, 250.0, 0.0, 1, 6));    // lejos de la banda
            procesarTickMotor(e.m, -1);
            ok = ok && e.m.ondaActiva && e.j.cargaPulso == 0 && e.j.est.pulsos == 1
                 && e.m.listaEnemigos.tamano() == 1 && e.m.listaEnemigos.primero()->dato.id == 2
                 && e.j.puntaje == 100 && e.j.combo == 3;                                            // suma puntos, no toca combo
        }
        resultado("P11", "Motor: pulso con carga < 100 (sin efecto) y con carga 100 (bajas en la banda, sin subir combo)", ok);
    }

    // ---------------- P12: merge sort sobre lista, mediana, media y sigma ----------------
    NodoNum* listaDe(const double* v, int n)
    {
        NodoNum* cabeza = nullptr;
        for (int i = n - 1; i >= 0; --i)
        {
            NodoNum* nodo = new NodoNum;
            nodo->valor = v[i];
            nodo->sig = cabeza;
            cabeza = nodo;
        }
        return cabeza;
    }

    bool ordenaBien(const double* v, int n)
    {
        NodoNum* l = mergeSortLista(listaDe(v, n));
        int cuenta = 0;
        bool ok = true;
        for (NodoNum* x = l; x != nullptr; x = x->sig)
        {
            ++cuenta;
            if (x->sig != nullptr && x->valor > x->sig->valor) ok = false;
        }
        liberarNumeros(l);
        return ok && cuenta == n;
    }

    // Arma un historial con esos puntajes para un mismo jugador y calcula su resumen
    void resumenDePuntajes(const double* v, int n, ResumenJugador& r)
    {
        HistorialPartidas h;
        for (int i = 0; i < n; ++i)
        {
            FilaHistorial f;
            f.modo = "prueba";
            f.jugador = "X";
            for (int k = 0; k < NUM_COLUMNAS; ++k) f.num[k] = 0;
            f.num[COL_ID] = i + 1;
            f.num[COL_POSICION] = 1;
            f.num[COL_PUNTAJE] = static_cast<int>(v[i]);
            agregarFila(h, f);
        }
        resumenDeJugador(h, "X", r);
    }

    void p12()
    {
        const double uno[1] = { 5 };
        const double dos[2] = { 2, 1 };
        const double ordenada[4] = { 1, 2, 3, 4 };
        const double inversa[5] = { 9, 7, 5, 3, 1 };
        const double repetidos[6] = { 3, 1, 3, 2, 1, 3 };
        bool ok = ordenaBien(uno, 0) && ordenaBien(uno, 1) && ordenaBien(dos, 2) && ordenaBien(ordenada, 4)
                  && ordenaBien(inversa, 5) && ordenaBien(repetidos, 6);

        const double impar[3] = { 7, 1, 5 };
        const double par[4] = { 4, 1, 3, 2 };
        const double datos[8] = { 2, 4, 4, 4, 5, 5, 7, 9 };   // media 5, sigma poblacional 2
        ResumenJugador r;
        resumenDePuntajes(impar, 3, r);   ok = ok && std::fabs(r.mediana - 5.0) < 1e-9;
        resumenDePuntajes(par, 4, r);     ok = ok && std::fabs(r.mediana - 2.5) < 1e-9;
        resumenDePuntajes(repetidos, 6, r); ok = ok && std::fabs(r.mediana - 2.5) < 1e-9;
        resumenDePuntajes(uno, 1, r);     ok = ok && std::fabs(r.mediana - 5.0) < 1e-9 && r.desviacion == 0.0;
        resumenDePuntajes(datos, 8, r);
        ok = ok && std::fabs(r.media - 5.0) < 1e-9 && std::fabs(r.desviacion - 2.0) < 1e-9;
        int enHistograma = 0;
        for (int i = 0; i < 10; ++i) enHistograma += r.histograma[i];
        ok = ok && enHistograma == 8;                                   // el histograma reparte todos los puntajes
        resultado("P12", "Estadisticas: merge sort con lista vacia, 1, 2, ordenada, inversa y repetidos; mediana con N par e impar; media, sigma e histograma", ok);
    }

    // ---------------- P13: persistencia ----------------
    void p13()
    {
        const string ruta = "prueba_historial_p13.csv";
        remove(ruta.c_str());

        ostringstream avisosGen;
        bool ok = (generarPartidasBots(3, ruta, 100, 4, avisosGen) == 0);

        { ofstream f(ruta.c_str(), ios::app); f << "esto,no,es,una,fila,valida\n"; }                              // linea invalida (linea 14)
        { ofstream f(ruta.c_str(), ios::app); f << "9,1,turnos,Ana,0,1,100,1,10,5,5,1,0,0,10,0,0,0,0,0,xx\n"; }   // campo no numerico (linea 15)

        // "Reiniciar el programa": se carga dos veces con objetos nuevos y deben dar lo mismo
        HistorialPartidas h1, h2;
        ostringstream av1, av2;
        const int malas1 = cargarHistorial(h1, ruta, av1);
        const int malas2 = cargarHistorial(h2, ruta, av2);
        ok = ok && h1.tam == 12 && h2.tam == 12 && malas1 == 2 && malas2 == 2;                                   // 3 partidas x 4 jugadores
        ok = ok && av1.str().find("linea 14") != string::npos && av1.str().find("linea 15") != string::npos;

        ostringstream est1, est2, rk1, rk2;
        imprimirEstadisticas(h1, est1);
        imprimirEstadisticas(h2, est2);
        imprimirRanking(h1, rk1);
        imprimirRanking(h2, rk2);
        ok = ok && !est1.str().empty() && est1.str() == est2.str() && rk1.str() == rk2.str() && !rk1.str().empty();

        remove(ruta.c_str());
        resultado("P13", "Persistencia: guardar, reiniciar y cargar -> mismos resultados en estadisticas y ranking; lineas invalidas reportadas", ok);
    }

    // ---------------- P14: memoria ----------------
    void p14()
    {
        OpcionesLote op;
        op.jugadores = "BOT1,BOT2,BOT3,BOT4";
        op.semilla = 2026;
        op.oleadas = 10;
        op.historial = "";
        ostringstream salida, avisos;
        const bool ok = (ejecutarLotes(op, salida, avisos) == 0) && !salida.str().empty();
        resultado("P14", "Memoria: partida con 4 bots y 10 oleadas (la verificacion de fugas la hace Valgrind / AddressSanitizer / CRT)", ok);
    }
}

int ejecutarPruebas()
{
    pasaron = 0;
    total = 0;
    cout << "=== PRUEBAS DE FASE ORBITAL (P01 - P14) ===" << endl;
    p01(); p02(); p03(); p04(); p05(); p06(); p07(); p08(); p09(); p10(); p11(); p12(); p13(); p14();
    cout << "-------------------------------------------" << endl;
    cout << pasaron << " de " << total << " pruebas pasan." << endl;
    return (pasaron == total) ? 0 : 1;
}
