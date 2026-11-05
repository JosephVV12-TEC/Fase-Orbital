#include "Motor.h"
#include <cmath>

namespace {
    const double ANGULO_MAXIMO_ROTACION = 6.0;
    const double RADIO_NUCLEO = 20.0;
    const double RADIO_ANILLO = 100.0;
    const double RADIO_BORRADO_BALA = 320.0;
    const double VELOCIDAD_BALA = 9.0;

    double normalizarAngulo(double angulo)
    {
        while (angulo < 0.0)
            angulo += 360.0;
        while (angulo >= 360.0)
            angulo -= 360.0;
        return angulo;
    }

    double diferenciaAngularMasCorta(double origen, double destino)
    {
        double delta = normalizarAngulo(destino - origen);
        if (delta > 180.0)
            delta -= 360.0;
        return delta;
    }

    void aplicarRotacionNave(Motor& motor, double objetivo)
    {
        if (motor.jugadorActual == nullptr)
            return;

        double delta = diferenciaAngularMasCorta(motor.anguloNave, objetivo);
        if (delta > ANGULO_MAXIMO_ROTACION)
            delta = ANGULO_MAXIMO_ROTACION;
        else if (delta < -ANGULO_MAXIMO_ROTACION)
            delta = -ANGULO_MAXIMO_ROTACION;

        motor.anguloNave = normalizarAngulo(motor.anguloNave + delta);
    }

    int proximoIdEnemigo = 1;

    // v = 0.8 + 0.08*n, multiplicada por el factor de cada tipo (seccion 3.3)
    double velocidadEnemigo(TipoEnemigo tipo, int oleada)
    {
        const double v = 0.8 + 0.08 * oleada;
        switch (tipo)
        {
        case TipoEnemigo::Dron:     return 1.00 * v;
        case TipoEnemigo::Girador:  return 0.85 * v;
        case TipoEnemigo::Divisor:  return 0.80 * v;
        case TipoEnemigo::Mini:     return 1.25 * v;
        case TipoEnemigo::Tanque:   return 0.50 * v;
        case TipoEnemigo::Camaleon: return 0.85 * v;
        }
        return v;
    }

    int puntosDelTipo(TipoEnemigo tipo)
    {
        switch (tipo)
        {
        case TipoEnemigo::Dron:     return 100;
        case TipoEnemigo::Girador:  return 150;
        case TipoEnemigo::Divisor:  return 200;
        case TipoEnemigo::Mini:     return 75;
        case TipoEnemigo::Tanque:   return 400;
        case TipoEnemigo::Camaleon: return 300;
        }
        return 0;
    }

    // mult = min(8, 1 + combo / 6)
    int multiplicadorPuntos(int combo)
    {
        int mult = 1 + combo / 6;
        return mult > 8 ? 8 : mult;
    }

    Enemigo crearEnemigoDesdeSpawn(const Spawn& sp, int oleada)
    {
        Enemigo e;
        e.id = proximoIdEnemigo++;
        e.tipo = static_cast<TipoEnemigo>(sp.tipoEnemigo);
        e.color = sp.color;
        e.r = 300.0;
        e.theta = static_cast<double>(sp.angulo);
        e.omega = 0.0;
        e.vida = 1;
        e.radio = 6;
        e.ticksColor = 0;

        if (e.tipo == TipoEnemigo::Girador)
        {
            const double w = 0.9 + 0.05 * oleada;
            e.omega = (sp.sentido == 0) ? -w : w; // sentido 0 = horario (theta decrece)
        }
        else if (e.tipo == TipoEnemigo::Divisor)
        {
            e.radio = 8;
        }
        else if (e.tipo == TipoEnemigo::Mini)
        {
            e.radio = 4;
        }
        else if (e.tipo == TipoEnemigo::Tanque)
        {
            e.vida = 3;
            e.radio = 10;
        }
        else if (e.tipo == TipoEnemigo::Camaleon)
        {
            e.radio = 7;
        }

        return e;
    }

    void moverEnemigo(Enemigo& e, int oleada)
    {
        e.r -= velocidadEnemigo(e.tipo, oleada);

        if (e.tipo == TipoEnemigo::Girador || e.tipo == TipoEnemigo::Mini)
        {
            e.theta = normalizarAngulo(e.theta + e.omega);
        }
        else if (e.tipo == TipoEnemigo::Camaleon)
        {
            e.ticksColor += 1;
            if (e.ticksColor >= 42)
            {
                e.color = (e.color + 1) % 3; // C -> M -> A -> C
                e.ticksColor = 0;
            }
        }
    }

    void daniarNucleo(Motor& motor, int dano)
    {
        if (motor.jugadorActual == nullptr)
            return;

        if (motor.jugadorActual->vidaNucleo > 0)
        {
            motor.jugadorActual->vidaNucleo -= dano;
            if (motor.jugadorActual->vidaNucleo < 0)
                motor.jugadorActual->vidaNucleo = 0;
        }
        motor.jugadorActual->combo = 0;
        motor.evDanoNucleo = true;
    }

    void posicionPolarACartesiana(double r, double theta, double& x, double& y)
    {
        const double rad = (theta * 3.14159265358979323846) / 180.0;
        x = std::cos(rad) * r;
        y = std::sin(rad) * r;
    }

    // Guarda donde murio un enemigo para que la vista dibuje la explosion
    void registrarBaja(Motor& m, const Enemigo& e)
    {
        if (m.nBajas < 32)
        {
            m.bajas[m.nBajas].r = e.r;
            m.bajas[m.nBajas].theta = e.theta;
            m.bajas[m.nBajas].color = e.color;
            m.nBajas += 1;
        }
    }

    // Deja los campos de acciones y eventos en su valor inicial
    void reiniciarAccionesYEventos(Motor& m)
    {
        m.anguloObjetivo = 270.0;
        m.pulsoSolicitado = false;
        m.ondaActiva = false;
        m.ondaR = 0.0;
        m.evDisparo = false;
        m.evErroneo = false;
        m.evDanoNucleo = false;
        m.evPulso = false;
        m.nBajas = 0;
    }

    // Vacia enemigos y balas que hayan quedado del turno anterior
    void limpiarCampo(Motor& m)
    {
        while (!m.listaEnemigos.vacia())
            m.listaEnemigos.eliminar(m.listaEnemigos.primero());

        while (m.listaBalas.cabeza != nullptr)
        {
            NodoBala* b = m.listaBalas.cabeza;
            m.listaBalas.cabeza = b->sig;
            delete b;
            m.listaBalas.tam--;
        }
    }

    bool balaImpactaEnemigo(const Bala& bala, const Enemigo& enemigo)
    {
        double xEnemigo = 0.0;
        double yEnemigo = 0.0;
        posicionPolarACartesiana(enemigo.r, enemigo.theta, xEnemigo, yEnemigo);

        const double dx = bala.x - xEnemigo;
        const double dy = bala.y - yEnemigo;
        const double distancia = std::sqrt((dx * dx) + (dy * dy));

        return distancia <= static_cast<double>(enemigo.radio + 4);
    }
}

void inicializarMotor(Motor& m)
{
    m.jugadorActual = nullptr;
    m.tickActual = 0;
    m.oleadaActual = 1;
    m.anguloNave = 270.0;
    inicializarCola(m.colaSpawnOleada);
    reiniciarAccionesYEventos(m);
}

void iniciarTurnoMotor(Motor& m, Jugador* nuevoJugador, ColaSpawn colaOleada, int oleada)
{
    m.jugadorActual = nuevoJugador;
    m.oleadaActual = oleada;
    copiarCola(colaOleada, m.colaSpawnOleada);
    m.tickActual = 0;
    m.anguloNave = 270.0;
    limpiarCampo(m);
    reiniciarAccionesYEventos(m);

    if (m.jugadorActual != nullptr)
    {
        // Seccion 5.1: el combo se reinicia, la vida y la carga de pulso se CONSERVAN
        m.jugadorActual->combo = 0;
    }
}

void procesarTickMotor(Motor& m, int nuevoColor)
{
    m.evDisparo = false;
    m.evErroneo = false;
    m.evDanoNucleo = false;
    m.evPulso = false;
    m.nBajas = 0;

    if (m.jugadorActual == nullptr)
        return;

    if (m.jugadorActual->vidaNucleo <= 0)
        return;

    if (nuevoColor >= 0 && nuevoColor <= 2)
        m.jugadorActual->colorActivo = nuevoColor;

    aplicarRotacionNave(m, m.anguloObjetivo);

    // Activar pulso: solo con carga = 100 y si la vista/bot lo pidio (seccion 3.4)
    if (m.pulsoSolicitado && !m.ondaActiva && m.jugadorActual->cargaPulso >= 100)
    {
        m.jugadorActual->cargaPulso = 0;
        m.ondaActiva = true;
        m.ondaR = 20.0;
        m.evPulso = true;
    }
    m.pulsoSolicitado = false;

    m.tickActual += 1;

    // Se mira el frente SIN sacarlo; si todavia no le toca, la cola no se altera
    Spawn spawnActual;
    while (!colaVacia(m.colaSpawnOleada))
    {
        PtrSpawn frente = verFrente(m.colaSpawnOleada);
        if (frente == nullptr || frente->tickSpawn > m.tickActual)
            break;

        desencolar(m.colaSpawnOleada, spawnActual);
        m.listaEnemigos.insertarFinal(crearEnemigoDesdeSpawn(spawnActual, m.oleadaActual));
    }

    if ((m.tickActual % 4) == 0)
    {
        Bala bala;
        const double rad = (m.anguloNave * 3.14159265358979323846) / 180.0;
        bala.x = std::cos(rad) * 100.0;
        bala.y = std::sin(rad) * 100.0;
        bala.vx = std::cos(rad) * VELOCIDAD_BALA;
        bala.vy = std::sin(rad) * VELOCIDAD_BALA;
        bala.color = m.jugadorActual->colorActivo;
        bala.duenio = 0;
        m.listaBalas.insertarInicio(bala);
        m.evDisparo = true;
    }

    // Paso 5: mover enemigos (y cambiar el color de los Camaleones)
    NodoEnemigo* enemigoActual = m.listaEnemigos.primero();
    while (enemigoActual != nullptr)
    {
        moverEnemigo(enemigoActual->dato, m.oleadaActual);
        enemigoActual = enemigoActual->sig;
    }

    // Paso 6: choques enemigo-nave y enemigo-nucleo
    double naveX = 0.0;
    double naveY = 0.0;
    posicionPolarACartesiana(RADIO_ANILLO, m.anguloNave, naveX, naveY);

    enemigoActual = m.listaEnemigos.primero();
    while (enemigoActual != nullptr)
    {
        const Enemigo& e = enemigoActual->dato;
        double ex = 0.0;
        double ey = 0.0;
        posicionPolarACartesiana(e.r, e.theta, ex, ey);
        const double distNave = std::sqrt((ex - naveX) * (ex - naveX) + (ey - naveY) * (ey - naveY));

        if (distNave < static_cast<double>(e.radio + 5))
        {
            daniarNucleo(m, 12);
            enemigoActual = m.listaEnemigos.eliminar(enemigoActual);
        }
        else if (e.r <= RADIO_NUCLEO + 0.4 * e.radio)
        {
            daniarNucleo(m, e.tipo == TipoEnemigo::Tanque ? 22 : 10);
            enemigoActual = m.listaEnemigos.eliminar(enemigoActual);
        }
        else
        {
            enemigoActual = enemigoActual->sig;
        }
    }

    NodoBala* balaActual = m.listaBalas.cabeza;
    while (balaActual != nullptr)
    {
        NodoBala* siguienteBala = balaActual->sig;
        Bala& bala = balaActual->dato;
        bala.x += bala.vx;
        bala.y += bala.vy;

        double distancia = std::sqrt((bala.x * bala.x) + (bala.y * bala.y));
        bool impacto = false;

        NodoEnemigo* enemigoColision = m.listaEnemigos.primero();
        while (enemigoColision != nullptr && !impacto)
        {
            if (balaImpactaEnemigo(bala, enemigoColision->dato))
            {
                impacto = true;

                if (enemigoColision->dato.color == bala.color)
                {
                    enemigoColision->dato.vida -= 1;

                    if (enemigoColision->dato.vida <= 0)
                    {
                        registrarBaja(m, enemigoColision->dato);
                        m.jugadorActual->puntaje += puntosDelTipo(enemigoColision->dato.tipo) * multiplicadorPuntos(m.jugadorActual->combo);
                        m.jugadorActual->combo += 1;
                        m.jugadorActual->cargaPulso += (enemigoColision->dato.tipo == TipoEnemigo::Tanque) ? 14 : 5;
                        if (m.jugadorActual->cargaPulso > 100)
                            m.jugadorActual->cargaPulso = 100;

                        if (enemigoColision->dato.tipo == TipoEnemigo::Divisor)
                        {
                            Enemigo miniA = enemigoColision->dato;
                            miniA.tipo = TipoEnemigo::Mini;
                            miniA.id = proximoIdEnemigo++;
                            miniA.vida = 1;
                            miniA.radio = 4;
                            miniA.r = enemigoColision->dato.r;
                            miniA.theta = normalizarAngulo(enemigoColision->dato.theta - 4.0);
                            miniA.omega = -1.7;
                            miniA.color = (enemigoColision->dato.color + 1) % 3;

                            Enemigo miniB = enemigoColision->dato;
                            miniB.tipo = TipoEnemigo::Mini;
                            miniB.id = proximoIdEnemigo++;
                            miniB.vida = 1;
                            miniB.radio = 4;
                            miniB.r = enemigoColision->dato.r;
                            miniB.theta = normalizarAngulo(enemigoColision->dato.theta + 4.0);
                            miniB.omega = 1.7;
                            miniB.color = (enemigoColision->dato.color + 2) % 3;

                            NodoEnemigo* nodoMiniA = m.listaEnemigos.insertarDespues(enemigoColision, miniA);
                            m.listaEnemigos.insertarDespues(nodoMiniA, miniB);
                            m.listaEnemigos.eliminar(enemigoColision);
                        }
                        else
                        {
                            m.listaEnemigos.eliminar(enemigoColision);
                        }
                    }
                }
                else
                {
                    enemigoColision->dato.r += 9.0;
                    m.jugadorActual->combo = 0;
                    m.evErroneo = true;
                }
            }
            else
            {
                enemigoColision = enemigoColision->sig;
            }
        }

        if (distancia > RADIO_BORRADO_BALA || impacto)
        {
            NodoBala* anterior = nullptr;
            NodoBala* actual = m.listaBalas.cabeza;
            while (actual != nullptr && actual != balaActual)
            {
                anterior = actual;
                actual = actual->sig;
            }

            if (anterior == nullptr)
                m.listaBalas.cabeza = balaActual->sig;
            else
                anterior->sig = balaActual->sig;

            delete balaActual;
            m.listaBalas.tam--;
        }

        balaActual = siguienteBala;
    }

    // Onda de pulso (seccion 3.4): nace en r = 20, crece 12 u/tick y destruye
    // sin importar el color a todo enemigo con |r - r_onda| < radio + 3
    if (m.ondaActiva)
    {
        m.ondaR += 12.0;

        NodoEnemigo* pulsoActual = m.listaEnemigos.primero();
        while (pulsoActual != nullptr)
        {
            NodoEnemigo* sigPulso = pulsoActual->sig;
            if (std::fabs(pulsoActual->dato.r - m.ondaR) < static_cast<double>(pulsoActual->dato.radio + 3))
            {
                m.jugadorActual->puntaje += puntosDelTipo(pulsoActual->dato.tipo) * multiplicadorPuntos(m.jugadorActual->combo); // TODO reglas: puntos del tipo x multiplicador
                registrarBaja(m, pulsoActual->dato);
                m.listaEnemigos.eliminar(pulsoActual);
            }
            pulsoActual = sigPulso;
        }

        if (m.ondaR > 320.0)
            m.ondaActiva = false;
    }
}
