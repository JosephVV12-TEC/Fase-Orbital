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

    // Predicado para ListaBalas::eliminarSi: borra las balas que ya impactaron o salieron
    bool balaInactiva(const Bala& b)
    {
        return b.inactiva;
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

    // Agrega un evento a la bitacora (si el Motor tiene una)
    void registrarEvento(Motor& m, TipoEvento tipo, int tipoEnemigo, int color, int valor)
    {
        if (m.bitacora == nullptr)
            return;

        Evento ev;
        ev.tick = m.tickActual;
        ev.oleada = m.oleadaActual;
        ev.jugador = m.idJugador;
        ev.tipo = tipo;
        ev.tipoEnemigo = tipoEnemigo;
        ev.color = color;
        ev.valor = valor;
        agregarFinal(*m.bitacora, ev);
    }

    // Dana al nucleo (por llegar al nucleo o por chocar con la nave) lo cuenta en las estadisticas
    void daniarNucleo(Motor& motor, const Enemigo& enemigo, int dano, TipoEvento tipoEvento)
    {
        if (motor.jugadorActual == nullptr)
            return;

        Jugador& j = *motor.jugadorActual;
        if (j.vidaNucleo > 0)
        {
            j.vidaNucleo -= dano;
            if (j.vidaNucleo < 0)
                j.vidaNucleo = 0;
            j.est.danoTotal += dano;
            j.est.danoTipo[static_cast<int>(enemigo.tipo)] += dano;
            registrarEvento(motor, tipoEvento, static_cast<int>(enemigo.tipo), enemigo.color, dano);
        }
        j.combo = 0;
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

    // Suma los puntos de una baja y la cuenta en las estadisticas
    // porPulso = true: no sube el combo ni la carga del pulso
    void contarBaja(Motor& m, const Enemigo& e, bool porPulso)
    {
        Jugador& j = *m.jugadorActual;
        const int puntos = puntosDelTipo(e.tipo) * multiplicadorPuntos(j.combo);
        j.puntaje += puntos;
        j.est.bajasTipo[static_cast<int>(e.tipo)] += 1;
        j.est.bajasColor[e.color] += 1;
        registrarBaja(m, e);
        registrarEvento(m, TipoEvento::BAJA, static_cast<int>(e.tipo), e.color, puntos);

        if (!porPulso)
        {
            j.combo += 1;
            if (j.combo > j.est.comboMax)
                j.est.comboMax = j.combo;
            j.cargaPulso += (e.tipo == TipoEnemigo::Tanque) ? 14 : 5;
            if (j.cargaPulso > 100)
                j.cargaPulso = 100;
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

// mult = min(8, 1 + combo / 6)
int multiplicadorPuntos(int combo)
{
    int mult = 1 + combo / 6;
    return mult > 8 ? 8 : mult;
}

// Aca van las funciones de la vitacora de eventos
BitacoraEventos::BitacoraEventos() : cabeza(nullptr), cola(nullptr), tam(0)
{
}

BitacoraEventos::~BitacoraEventos()
{
    vaciarBitacora(*this);
}

void agregarFinal(BitacoraEventos& b, const Evento& e)
{
    NodoEvento* nuevo = new NodoEvento;
    nuevo->dato = e;
    nuevo->sig = nullptr;

    if (b.cola == nullptr)
        b.cabeza = nuevo;
    else
        b.cola->sig = nuevo;

    b.cola = nuevo;
    b.tam += 1;
}

void vaciarBitacora(BitacoraEventos& b)
{
    while (b.cabeza != nullptr)
    {
        NodoEvento* aux = b.cabeza;
        b.cabeza = b.cabeza->sig;
        delete aux;
    }
    b.cola = nullptr;
    b.tam = 0;
}

NodoEvento* siguienteEvento(NodoEvento* desde, int jugador, int oleada)
{
    NodoEvento* n = desde;
    while (n != nullptr)
    {
        if ((jugador < 0 || n->dato.jugador == jugador) && (oleada < 0 || n->dato.oleada == oleada))
            return n;
        n = n->sig;
    }
    return nullptr;
}

int contarEventos(const BitacoraEventos& b, int jugador, int oleada, TipoEvento tipo)
{
    int total = 0;
    NodoEvento* n = siguienteEvento(b.cabeza, jugador, oleada);
    while (n != nullptr)
    {
        if (n->dato.tipo == tipo)
            total += 1;
        n = siguienteEvento(n->sig, jugador, oleada);
    }
    return total;
}

int sumarValorEventos(const BitacoraEventos& b, int jugador, int oleada, TipoEvento tipo)
{
    int suma = 0;
    NodoEvento* n = siguienteEvento(b.cabeza, jugador, oleada);
    while (n != nullptr)
    {
        if (n->dato.tipo == tipo)
            suma += n->dato.valor;
        n = siguienteEvento(n->sig, jugador, oleada);
    }
    return suma;
}

void inicializarMotor(Motor& m)
{
    m.jugadorActual = nullptr;
    m.tickActual = 0;
    m.oleadaActual = 1;
    m.idJugador = -1;
    m.bitacora = nullptr;
    m.anguloNave = 270.0;
    inicializarCola(m.colaSpawnOleada);
    reiniciarAccionesYEventos(m);
}

void iniciarTurnoMotor(Motor& m, Jugador* nuevoJugador, const ColaSpawn& colaOleada, int oleada, int idJugador, BitacoraEventos* bitacora)
{
    m.jugadorActual = nuevoJugador;
    m.oleadaActual = oleada;
    m.idJugador = idJugador;
    m.bitacora = bitacora;
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

ResultadoTick procesarTickMotor(Motor& m, int nuevoColor)
{
    m.evDisparo = false;
    m.jugadorActual->est.disparos += 1;
    m.evErroneo = false;
    m.evDanoNucleo = false;
    m.evPulso = false;
    m.nBajas = 0;

    if (m.jugadorActual == nullptr)
        return ResultadoTick::Sigue;

    if (m.jugadorActual->vidaNucleo <= 0)
        return ResultadoTick::NucleoDestruido;

    if (nuevoColor >= 0 && nuevoColor <= 2)
        m.jugadorActual->colorActivo = nuevoColor;

    aplicarRotacionNave(m, m.anguloObjetivo);

    // Activar pulso: solo con carga = 100 y si la vista/bot lo pidio (seccion 3.4)
        // Activar pulso: solo con carga = 100 y si la vista/bot lo pidio (seccion 3.4)
    if (m.pulsoSolicitado && !m.ondaActiva && m.jugadorActual->cargaPulso >= 100)
    {
        m.jugadorActual->cargaPulso = 0;
        m.ondaActiva = true;
        m.ondaR = 20.0;
        m.evPulso = true;
        m.jugadorActual->est.pulsos += 1;                 
        registrarEvento(m, TipoEvento::PULSO, -1, -1, 0); 
    }
    m.pulsoSolicitado = false;

    m.tickActual += 1;
    m.jugadorActual->est.ticks += 1;

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
            daniarNucleo(m, e, 12, TipoEvento::CHOQUE_NAVE);
            enemigoActual = m.listaEnemigos.eliminar(enemigoActual);
        }
        else if (e.r <= RADIO_NUCLEO + 0.4 * e.radio)
        {
            daniarNucleo(m, e, e.tipo == TipoEnemigo::Tanque ? 22 : 10, TipoEvento::DANO_NUCLEO);
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
        while (enemigoColision != nullptr && !impacto && distancia <= RADIO_BORRADO_BALA) //Esto hace que si la bala pasa r = 320 y no impacta que se borre
        {
            if (balaImpactaEnemigo(bala, enemigoColision->dato))
            {
                impacto = true;
                m.jugadorActual->est.impactosFase[bala.color] += 1;

                if (enemigoColision->dato.color == bala.color)
                {
                    m.jugadorActual->est.correctos += 1;
                    m.jugadorActual->est.correctosFase[bala.color] += 1;
                    enemigoColision->dato.vida -= 1;

                    if (enemigoColision->dato.vida <= 0)
                    {
                        contarBaja(m, enemigoColision->dato, false);

                        if (enemigoColision->dato.tipo == TipoEnemigo::Divisor)
                        {
							Enemigo miniA = enemigoColision->dato;
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
                    m.jugadorActual->est.erroneos += 1;
                    registrarEvento(m, TipoEvento::IMPACTO_ERRONEO, static_cast<int>(enemigoColision->dato.tipo), bala.color, 0);
                    m.evErroneo = true;
                }
            }
            else
            {
                enemigoColision = enemigoColision->sig;
            }
        }

		if (distancia > RADIO_BORRADO_BALA || impacto)// si la bala se fue del campo o impacto, se marca como inactiva para que la lista la borre
            bala.inactiva = true;

        balaActual = siguienteBala;
    }

    // Una sola pasada con NodoBala**: borra todas las marcadas, sin caso especial para la cabeza
    m.listaBalas.eliminarSi(balaInactiva);

    // Onda de pulso (seccion 3.4): nace en r = 20, crece 12 u/tick y destruye
    // sin importar el color a todo enemigo con |r - r_onda| < radio + 3
    if (m.ondaActiva)
    {
        m.ondaR += 12.0;

        NodoEnemigo* pulsoActual = m.listaEnemigos.primero();
        while (pulsoActual != nullptr)
        {
            NodoEnemigo* sigPulso = pulsoActual->sig;
            if (fabs(pulsoActual->dato.r - m.ondaR) < static_cast<double>(pulsoActual->dato.radio + 3))
            {
                contarBaja(m, pulsoActual->dato, true);
                m.listaEnemigos.eliminar(pulsoActual);
            }
            pulsoActual = sigPulso;
        }

        if (m.ondaR > 320.0)
            m.ondaActiva = false;
    }

    // Paso 10: fin de turno del jugador (lo decide el Motor, no la vista)
    if (m.jugadorActual->vidaNucleo <= 0)
    {
        registrarEvento(m, TipoEvento::ELIMINACION, -1, -1, m.jugadorActual->puntaje);
        return ResultadoTick::NucleoDestruido;
    }
    if (colaVacia(m.colaSpawnOleada) && m.listaEnemigos.vacia())
    {
        registrarEvento(m, TipoEvento::FIN_OLEADA, -1, -1, m.jugadorActual->puntaje);
        return ResultadoTick::OleadaCompleta;
    }
    return ResultadoTick::Sigue;
}
}