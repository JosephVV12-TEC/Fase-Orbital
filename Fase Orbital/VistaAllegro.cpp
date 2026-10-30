// VistaAllegro.cpp
// Ventana, dibujo, controles, pantallas y audio de Fase Orbital (Allegro 5).
//
// IMPORTANTE: este archivo NO debe incluir "Juego.h" ni "FuncionesJuego.h"
// (esos headers traen definiciones de funciones y solo pueden compilarse en UN .cpp).

#include "VistaAllegro.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>

#include "Motor.h"
#include "Bot.h"
#include "Partida.h"

using namespace std;

namespace
{
    // ------------------------------------------------------------------
    // Constantes
    // ------------------------------------------------------------------
    const int   PANTALLA_W = 1000;               // minimo exigido: 800 x 600
    const int   PANTALLA_H = 720;
    const float CX = PANTALLA_W / 2.0f;          // el nucleo esta en el centro
    const float CY = PANTALLA_H / 2.0f;
    const double PI_ = 3.14159265358979323846;

    const double RADIO_ANILLO = 100.0;           // orbita de la nave (seccion 3.1)
    const double RADIO_CAMPO = 320.0;            // borde del campo de juego

    const int TICKS_ANUNCIO = 75;                // 2.5 s de anuncio de turno
    const int TICKS_RESUMEN_BOT = 90;            // 3 s si el que jugo fue un bot

    const unsigned int SEMILLA_DEFECTO = 42;
    const int LIMITE_OLEADAS_DEFECTO = 10;
    const int MAX_ALIAS = 12;
    const int MAX_PARTICULAS = 300;

    // Rutas de recursos (relativas a la carpeta de trabajo; ver instrucciones de Visual Studio)
    const char* RUTA_FUENTE = "assets/fonts/fuente.ttf";
    const char* RUTA_FUENTE_WINDOWS = "C:\\Windows\\Fonts\\arial.ttf";
    const char* RUTA_FONDO = "assets/images/fondo.png";
    const char* RUTA_S_BAJA = "assets/sounds/baja.wav";
    const char* RUTA_S_ERRONEO = "assets/sounds/erroneo.wav";
    const char* RUTA_S_DANO = "assets/sounds/dano.wav";
    const char* RUTA_S_PULSO = "assets/sounds/pulso.wav";

    enum class Pantalla
    {
        Menu, Registro, AnuncioTurno, Juego, Pausa,
        ResumenOleada, FinPartida, Estadisticas, Ranking
    };

    struct Particula
    {
        float x, y, vx, vy;
        int vida, vidaMax;
        int color;
        bool activa;
    };

    struct Vista
    {
        // ---- Recursos de Allegro ----
        ALLEGRO_DISPLAY* display = nullptr;
        ALLEGRO_EVENT_QUEUE* cola = nullptr;
        ALLEGRO_TIMER* timer = nullptr;
        ALLEGRO_FONT* fuente = nullptr;
        ALLEGRO_FONT* fuenteTitulo = nullptr;
        ALLEGRO_FONT* fuentePeq = nullptr;
        ALLEGRO_BITMAP* fondo = nullptr;
        ALLEGRO_SAMPLE* sBaja = nullptr;
        ALLEGRO_SAMPLE* sErroneo = nullptr;
        ALLEGRO_SAMPLE* sDano = nullptr;
        ALLEGRO_SAMPLE* sPulso = nullptr;
        bool audioActivo = false;

        // ---- Estado del juego (la vista lo consulta, no lo decide) ----
        Motor motor;
        Partida partida;

        // ---- Estado propio de la interfaz ----
        Pantalla pantalla = Pantalla::Menu;
        bool ejecutando = true;
        int opcionMenu = 0;
        int ticksPantalla = 0;
        bool partidaJugada = false;

        // Registro
        string aliasEscrito;
        string mensajeRegistro;
        int limiteOleadas = LIMITE_OLEADAS_DEFECTO;

        // Entrada
        int colorPendiente = -1;
        bool pulsoPendiente = false;
        bool usarMouse = false;
        int mouseX = (int)CX;
        int mouseY = (int)CY;

        // Resumen del ultimo turno
        int idxTurno = -1;
        bool turnoCompletado = false;
        bool turnoFueBot = false;
        ResultadoTurno resultado = ResultadoTurno::SigueRonda;

        // Efectos visuales
        Particula particulas[MAX_PARTICULAS];
        int flashNucleo = 0;
        int destelloPulso = 0;
    };

    // ------------------------------------------------------------------
    // Utilidades
    // ------------------------------------------------------------------
    double normalizar(double a)
    {
        while (a < 0.0) a += 360.0;
        while (a >= 360.0) a -= 360.0;
        return a;
    }

    double aRadianes(double grados) { return grados * PI_ / 180.0; }

    ALLEGRO_COLOR colorFase(int c)
    {
        switch (c)
        {
            case 0: return al_map_rgb(0, 235, 255);   // C - cian
            case 1: return al_map_rgb(255, 40, 220);  // M - magenta
            case 2: return al_map_rgb(255, 180, 20);  // A - ambar
            default: return al_map_rgb(200, 200, 200);
        }
    }

    // Allegro 5.2 usa alfa premultiplicado: hay que multiplicar el color por el alfa.
    ALLEGRO_COLOR conAlfa(ALLEGRO_COLOR c, float a)
    {
        unsigned char r, g, b;
        al_unmap_rgb(c, &r, &g, &b);
        return al_map_rgba_f(r / 255.0f * a, g / 255.0f * a, b / 255.0f * a, a);
    }

    const char* nombreFase(int c)
    {
        switch (c)
        {
            case 0: return "CIAN";
            case 1: return "MAGENTA";
            case 2: return "AMBAR";
            default: return "?";
        }
    }

    int multiplicador(int combo)
    {
        int m = 1 + combo / 6;
        return m > 8 ? 8 : m;
    }

    // ------------------------------------------------------------------
    // Recursos
    // ------------------------------------------------------------------
    ALLEGRO_FONT* cargarFuente(int tam)
    {
        static bool avisado = false;
        ALLEGRO_FONT* f = al_load_ttf_font(RUTA_FUENTE, tam, 0);
        if (!f) f = al_load_ttf_font(RUTA_FUENTE_WINDOWS, tam, 0);
        if (!f)
        {
            if (!avisado)
            {
                cerr << "AVISO: no se encontro " << RUTA_FUENTE << " ni Arial; se usa la fuente interna." << endl;
                avisado = true;
            }
            f = al_create_builtin_font();
        }
        return f;
    }

    ALLEGRO_SAMPLE* cargarSonido(const char* ruta)
    {
        ALLEGRO_SAMPLE* s = al_load_sample(ruta);
        if (!s)
            throw runtime_error(string("Falta el recurso de sonido: ") + ruta);
        return s;
    }

    void inicializarVista(Vista& v)
    {
        if (!al_init())                        throw runtime_error("No se pudo inicializar Allegro.");
        if (!al_init_primitives_addon())       throw runtime_error("No se pudo inicializar el addon de primitivas.");
        if (!al_init_font_addon())             throw runtime_error("No se pudo inicializar el addon de fuentes.");
        if (!al_init_ttf_addon())              throw runtime_error("No se pudo inicializar el addon TTF.");
        if (!al_init_image_addon())            throw runtime_error("No se pudo inicializar el addon de imagenes.");
        if (!al_install_keyboard())            throw runtime_error("No se pudo instalar el teclado.");
        if (!al_install_mouse())               throw runtime_error("No se pudo instalar el mouse.");

        v.display = al_create_display(PANTALLA_W, PANTALLA_H);
        if (!v.display)                        throw runtime_error("No se pudo crear la ventana.");
        al_set_window_title(v.display, "Fase Orbital");

        v.cola = al_create_event_queue();
        if (!v.cola)                           throw runtime_error("No se pudo crear la cola de eventos.");

        v.timer = al_create_timer(1.0 / 30.0); // 30 ticks por segundo
        if (!v.timer)                          throw runtime_error("No se pudo crear el temporizador.");

        v.fuente = cargarFuente(18);
        v.fuenteTitulo = cargarFuente(40);
        v.fuentePeq = cargarFuente(14);
        if (!v.fuente || !v.fuenteTitulo || !v.fuentePeq)
            throw runtime_error("No se pudo cargar ninguna fuente.");

        // Imagen opcional: si no esta, se dibuja con primitivas
        v.fondo = al_load_bitmap(RUTA_FONDO);
        if (!v.fondo)
            cerr << "AVISO: no se encontro " << RUTA_FONDO << "; se usa fondo liso." << endl;

        // Audio: si no hay dispositivo se juega en silencio; si falta un archivo se informa y se termina
        if (al_install_audio() && al_init_acodec_addon() && al_reserve_samples(8))
        {
            v.audioActivo = true;
            v.sBaja = cargarSonido(RUTA_S_BAJA);
            v.sErroneo = cargarSonido(RUTA_S_ERRONEO);
            v.sDano = cargarSonido(RUTA_S_DANO);
            v.sPulso = cargarSonido(RUTA_S_PULSO);
        }
        else
        {
            cerr << "AVISO: audio no disponible; el juego continua sin sonido." << endl;
        }

        al_register_event_source(v.cola, al_get_display_event_source(v.display));
        al_register_event_source(v.cola, al_get_timer_event_source(v.timer));
        al_register_event_source(v.cola, al_get_keyboard_event_source());
        al_register_event_source(v.cola, al_get_mouse_event_source());

        srand(static_cast<unsigned int>(time(nullptr))); // solo para particulas (no afecta la logica)
        for (int i = 0; i < MAX_PARTICULAS; ++i)
            v.particulas[i].activa = false;
    }

    // Libera TODO lo que se haya creado (seguro aunque la inicializacion haya fallado a medias)
    void destruirVista(Vista& v)
    {
        vaciarCola(v.motor.colaSpawnOleada);
        destruirPartida(v.partida);

        if (v.sBaja) al_destroy_sample(v.sBaja);
        if (v.sErroneo) al_destroy_sample(v.sErroneo);
        if (v.sDano) al_destroy_sample(v.sDano);
        if (v.sPulso) al_destroy_sample(v.sPulso);
        if (v.fondo) al_destroy_bitmap(v.fondo);
        if (v.fuente) al_destroy_font(v.fuente);
        if (v.fuenteTitulo) al_destroy_font(v.fuenteTitulo);
        if (v.fuentePeq) al_destroy_font(v.fuentePeq);
        if (v.timer) al_destroy_timer(v.timer);
        if (v.cola) al_destroy_event_queue(v.cola);
        if (v.display) al_destroy_display(v.display);

        v.sBaja = v.sErroneo = v.sDano = v.sPulso = nullptr;
        v.fondo = nullptr;
        v.fuente = v.fuenteTitulo = v.fuentePeq = nullptr;
        v.timer = nullptr;
        v.cola = nullptr;
        v.display = nullptr;
    }

    void sonar(Vista& v, ALLEGRO_SAMPLE* s)
    {
        if (v.audioActivo && s)
            al_play_sample(s, 0.7f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, nullptr);
    }

    // ------------------------------------------------------------------
    // Particulas (efecto en cada baja)
    // ------------------------------------------------------------------
    void crearExplosion(Vista& v, double r, double theta, int color)
    {
        const float x = CX + static_cast<float>(cos(aRadianes(theta)) * r);
        const float y = CY + static_cast<float>(sin(aRadianes(theta)) * r);
        int creadas = 0;
        for (int i = 0; i < MAX_PARTICULAS && creadas < 14; ++i)
        {
            Particula& p = v.particulas[i];
            if (p.activa) continue;
            const double ang = (rand() % 360) * PI_ / 180.0;
            const float vel = 1.0f + (rand() % 300) / 100.0f;
            p.x = x; p.y = y;
            p.vx = static_cast<float>(cos(ang)) * vel;
            p.vy = static_cast<float>(sin(ang)) * vel;
            p.vidaMax = 14 + rand() % 12;
            p.vida = p.vidaMax;
            p.color = color;
            p.activa = true;
            ++creadas;
        }
    }

    void actualizarParticulas(Vista& v)
    {
        for (int i = 0; i < MAX_PARTICULAS; ++i)
        {
            Particula& p = v.particulas[i];
            if (!p.activa) continue;
            p.x += p.vx;
            p.y += p.vy;
            p.vx *= 0.94f;
            p.vy *= 0.94f;
            if (--p.vida <= 0) p.activa = false;
        }
        if (v.flashNucleo > 0) --v.flashNucleo;
        if (v.destelloPulso > 0) --v.destelloPulso;
    }

    // ------------------------------------------------------------------
    // Flujo entre pantallas
    // ------------------------------------------------------------------
    void irAlMenu(Vista& v)
    {
        v.pantalla = Pantalla::Menu;
    }

    void irAlRegistro(Vista& v)
    {
        destruirPartida(v.partida);
        inicializarPartida(v.partida);
        v.aliasEscrito.clear();
        v.mensajeRegistro.clear();
        v.pantalla = Pantalla::Registro;
    }

    void empezarPartida(Vista& v)
    {
        comenzarPartida(v.partida, v.motor, SEMILLA_DEFECTO, v.limiteOleadas);
        v.partidaJugada = true;
        v.pantalla = Pantalla::AnuncioTurno;
        v.ticksPantalla = 0;
        for (int i = 0; i < MAX_PARTICULAS; ++i)
            v.particulas[i].activa = false;
    }

    void comenzarTurno(Vista& v)
    {
        v.pantalla = Pantalla::Juego;
        v.colorPendiente = -1;
        v.pulsoPendiente = false;
        v.usarMouse = false;
        v.flashNucleo = 0;
    }

    void finalizarTurno(Vista& v, bool completada)
    {
        Jugador* j = v.motor.jugadorActual;
        v.idxTurno = indiceDeJugador(v.partida, j);
        v.turnoCompletado = completada;
        v.turnoFueBot = (j != nullptr && j->esBot);
        v.resultado = terminarTurno(v.partida, completada); // la logica de turnos vive en Partida
        v.pantalla = Pantalla::ResumenOleada;
        v.ticksPantalla = 0;
    }

    void continuarDesdeResumen(Vista& v)
    {
        if (v.resultado == ResultadoTurno::PartidaTerminada)
        {
            v.pantalla = Pantalla::FinPartida;
            return;
        }
        iniciarTurnoActual(v.partida, v.motor);
        v.pantalla = Pantalla::AnuncioTurno;
        v.ticksPantalla = 0;
    }

    // ------------------------------------------------------------------
    // Controles del jugador humano -> acciones para el Motor
    // ------------------------------------------------------------------
    void aplicarControlesHumano(Vista& v)
    {
        Motor& m = v.motor;

        ALLEGRO_KEYBOARD_STATE ks;
        al_get_keyboard_state(&ks);
        const bool izq = al_key_down(&ks, ALLEGRO_KEY_LEFT);
        const bool der = al_key_down(&ks, ALLEGRO_KEY_RIGHT);

        if (izq != der)
        {
            // Flechas: se pide girar 6 grados mas por tick (el Motor limita a 6)
            m.anguloObjetivo = normalizar(m.anguloNave + (der ? 6.0 : -6.0));
            v.usarMouse = false;
        }
        else if (v.usarMouse)
        {
            const double dx = v.mouseX - CX;
            const double dy = v.mouseY - CY;
            m.anguloObjetivo = normalizar(atan2(dy, dx) * 180.0 / PI_);
        }
    }

    void reaccionarAEventos(Vista& v)
    {
        const Motor& m = v.motor;

        for (int i = 0; i < m.nBajas; ++i)
            crearExplosion(v, m.bajas[i].r, m.bajas[i].theta, m.bajas[i].color);

        if (m.nBajas > 0) sonar(v, v.sBaja);
        if (m.evErroneo) sonar(v, v.sErroneo);
        if (m.evDanoNucleo) { v.flashNucleo = 10; sonar(v, v.sDano); }
        if (m.evPulso) { v.destelloPulso = 8; sonar(v, v.sPulso); }
    }

    // Un tick de juego: leer acciones -> Motor.procesarTickMotor -> reaccionar
    void tickJuego(Vista& v)
    {
        Motor& m = v.motor;
        Jugador* j = m.jugadorActual;
        if (j == nullptr)
            return;

        int color = -1;
        if (j->esBot)
        {
            const AccionBot a = decidirBot(m); // heuristica fija de la seccion 5.2
            m.anguloObjetivo = a.anguloObjetivo;
            m.pulsoSolicitado = a.pulso;
            color = a.color;
        }
        else
        {
            aplicarControlesHumano(v);
            m.pulsoSolicitado = v.pulsoPendiente;
            color = v.colorPendiente;
        }
        v.colorPendiente = -1;
        v.pulsoPendiente = false;

        procesarTickMotor(m, color);
        reaccionarAEventos(v);

        if (j->vidaNucleo <= 0)
            finalizarTurno(v, false);
        else if (colaVacia(m.colaSpawnOleada) && m.listaEnemigos.vacia())
            finalizarTurno(v, true);
    }

    void tickVista(Vista& v)
    {
        switch (v.pantalla)
        {
            case Pantalla::AnuncioTurno:
                if (++v.ticksPantalla >= TICKS_ANUNCIO)
                    comenzarTurno(v);
                break;
            case Pantalla::Juego:
                tickJuego(v);
                break;
            case Pantalla::ResumenOleada:
                if (v.turnoFueBot && ++v.ticksPantalla >= TICKS_RESUMEN_BOT)
                    continuarDesdeResumen(v);
                break;
            default:
                break;
        }
        actualizarParticulas(v);
    }

    // ------------------------------------------------------------------
    // Entrada de teclado por pantalla
    // ------------------------------------------------------------------
    bool aliasRepetido(const Vista& v, const string& alias)
    {
        for (int i = 0; i < v.partida.cantidad; ++i)
        {
            const string& otro = v.partida.jugadores[i].alias;
            if (otro.size() != alias.size()) continue;
            bool igual = true;
            for (size_t k = 0; k < otro.size(); ++k)
                if (tolower(static_cast<unsigned char>(otro[k])) != tolower(static_cast<unsigned char>(alias[k])))
                    igual = false;
            if (igual) return true;
        }
        return false;
    }

    void agregarJugadorDesdeRegistro(Vista& v, const string& alias)
    {
        if (v.partida.cantidad >= MAX_JUGADORES)
            v.mensajeRegistro = "Maximo 6 jugadores.";
        else if (aliasRepetido(v, alias))
            v.mensajeRegistro = "Ese alias ya esta registrado.";
        else
        {
            registrarJugador(v.partida, alias);
            v.mensajeRegistro.clear();
        }
    }

    void teclaMenu(Vista& v, int tecla)
    {
        int opcion = -1;
        if (tecla == ALLEGRO_KEY_UP) v.opcionMenu = (v.opcionMenu + 3) % 4;
        else if (tecla == ALLEGRO_KEY_DOWN) v.opcionMenu = (v.opcionMenu + 1) % 4;
        else if (tecla == ALLEGRO_KEY_ENTER) opcion = v.opcionMenu;
        else if (tecla == ALLEGRO_KEY_1) opcion = 0;
        else if (tecla == ALLEGRO_KEY_2) opcion = 1;
        else if (tecla == ALLEGRO_KEY_3) opcion = 2;
        else if (tecla == ALLEGRO_KEY_4 || tecla == ALLEGRO_KEY_ESCAPE) opcion = 3;

        switch (opcion)
        {
            case 0: irAlRegistro(v); break;
            case 1: v.pantalla = Pantalla::Estadisticas; break;
            case 2: v.pantalla = Pantalla::Ranking; break;
            case 3: v.ejecutando = false; break;
            default: break;
        }
    }

    void teclaRegistro(Vista& v, int tecla)
    {
        if (tecla == ALLEGRO_KEY_ESCAPE)
        {
            irAlMenu(v);
        }
        else if (tecla == ALLEGRO_KEY_ENTER)
        {
            string alias = v.aliasEscrito;
            if (alias.empty())
                alias = "Jugador" + to_string(v.partida.cantidad + 1);
            agregarJugadorDesdeRegistro(v, alias);
            v.aliasEscrito.clear();
        }
        else if (tecla == ALLEGRO_KEY_TAB)
        {
            int bots = 0;
            for (int i = 0; i < v.partida.cantidad; ++i)
                if (v.partida.jugadores[i].esBot) ++bots;
            agregarJugadorDesdeRegistro(v, "BOT" + to_string(bots + 1));
        }
        else if (tecla == ALLEGRO_KEY_F2)
        {
            if (v.partida.cantidad >= 1) empezarPartida(v);
            else v.mensajeRegistro = "Registra al menos un jugador.";
        }
        else if (tecla == ALLEGRO_KEY_LEFT)
        {
            if (v.limiteOleadas > 1) --v.limiteOleadas;
        }
        else if (tecla == ALLEGRO_KEY_RIGHT)
        {
            if (v.limiteOleadas < 50) ++v.limiteOleadas;
        }
    }

    void teclaJuego(Vista& v, int tecla)
    {
        if (tecla == ALLEGRO_KEY_P || tecla == ALLEGRO_KEY_ESCAPE)
        {
            v.pantalla = Pantalla::Pausa;
        }
        else if (tecla == ALLEGRO_KEY_1) v.colorPendiente = 0;
        else if (tecla == ALLEGRO_KEY_2) v.colorPendiente = 1;
        else if (tecla == ALLEGRO_KEY_3) v.colorPendiente = 2;
        else if (tecla == ALLEGRO_KEY_SPACE) v.pulsoPendiente = true;
    }

    void teclaPantalla(Vista& v, int tecla)
    {
        switch (v.pantalla)
        {
            case Pantalla::Menu:
                teclaMenu(v, tecla);
                break;
            case Pantalla::Registro:
                teclaRegistro(v, tecla);
                break;
            case Pantalla::AnuncioTurno:
                if (tecla == ALLEGRO_KEY_ENTER) comenzarTurno(v);
                break;
            case Pantalla::Juego:
                teclaJuego(v, tecla);
                break;
            case Pantalla::Pausa:
                if (tecla == ALLEGRO_KEY_P || tecla == ALLEGRO_KEY_ESCAPE) v.pantalla = Pantalla::Juego;
                else if (tecla == ALLEGRO_KEY_Q) irAlMenu(v); // abandonar la partida
                break;
            case Pantalla::ResumenOleada:
                if (tecla == ALLEGRO_KEY_ENTER) continuarDesdeResumen(v);
                break;
            case Pantalla::FinPartida:
                if (tecla == ALLEGRO_KEY_ENTER || tecla == ALLEGRO_KEY_ESCAPE) irAlMenu(v);
                break;
            case Pantalla::Estadisticas:
            case Pantalla::Ranking:
                if (tecla == ALLEGRO_KEY_ENTER || tecla == ALLEGRO_KEY_ESCAPE) irAlMenu(v);
                break;
        }
    }

    // Texto escrito en la pantalla de registro (solo letras, numeros y guion bajo)
    void caracterRegistro(Vista& v, int keycode, int unichar)
    {
        if (v.pantalla != Pantalla::Registro) return;

        if (keycode == ALLEGRO_KEY_BACKSPACE)
        {
            if (!v.aliasEscrito.empty()) v.aliasEscrito.pop_back();
        }
        else if (static_cast<int>(v.aliasEscrito.size()) < MAX_ALIAS)
        {
            const bool valido = (unichar >= '0' && unichar <= '9') ||
                                (unichar >= 'A' && unichar <= 'Z') ||
                                (unichar >= 'a' && unichar <= 'z') || unichar == '_';
            if (valido) v.aliasEscrito.push_back(static_cast<char>(unichar));
        }
    }

    // ------------------------------------------------------------------
    // Dibujo: utilidades
    // ------------------------------------------------------------------
    void dibujarFondo(Vista& v, float brillo)
    {
        al_clear_to_color(al_map_rgb(5, 8, 18));
        if (v.fondo)
        {
            al_draw_tinted_scaled_bitmap(v.fondo, al_map_rgba_f(brillo, brillo, brillo, 1.0f),
                0, 0, static_cast<float>(al_get_bitmap_width(v.fondo)), static_cast<float>(al_get_bitmap_height(v.fondo)),
                0, 0, static_cast<float>(PANTALLA_W), static_cast<float>(PANTALLA_H), 0);
        }
    }

    void panel(float x1, float y1, float x2, float y2)
    {
        al_draw_filled_rectangle(x1, y1, x2, y2, al_map_rgba(0, 0, 0, 170));
        al_draw_rectangle(x1, y1, x2, y2, al_map_rgb(70, 100, 150), 1.5f);
    }

    void barra(float x, float y, float w, float h, float frac, ALLEGRO_COLOR color)
    {
        if (frac < 0.0f) frac = 0.0f;
        if (frac > 1.0f) frac = 1.0f;
        al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgb(25, 30, 45));
        al_draw_filled_rectangle(x, y, x + w * frac, y + h, color);
        al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(150, 170, 200), 1.0f);
    }

    // Poligono regular (lados >= 3) centrado en (cx, cy), con borde blanco fino
    void poligono(float cx, float cy, float radio, int lados, double rot, ALLEGRO_COLOR color)
    {
        float verts[16];
        for (int i = 0; i < lados && i < 8; ++i)
        {
            const double a = rot + i * 2.0 * PI_ / lados;
            verts[i * 2] = cx + static_cast<float>(cos(a)) * radio;
            verts[i * 2 + 1] = cy + static_cast<float>(sin(a)) * radio;
        }
        al_draw_filled_polygon(verts, lados, color);
        al_draw_polygon(verts, lados, ALLEGRO_LINE_JOIN_ROUND, al_map_rgb(255, 255, 255), 1.2f, 1.0f);
    }

    // Estrella de 4 puntas (8 vertices alternando radio exterior e interior)
    void estrella(float cx, float cy, float radio, double rot, ALLEGRO_COLOR color)
    {
        float verts[16];
        for (int i = 0; i < 8; ++i)
        {
            const double a = rot + i * PI_ / 4.0;
            const float rr = (i % 2 == 0) ? radio * 1.25f : radio * 0.55f;
            verts[i * 2] = cx + static_cast<float>(cos(a)) * rr;
            verts[i * 2 + 1] = cy + static_cast<float>(sin(a)) * rr;
        }
        al_draw_filled_polygon(verts, 8, color);
        al_draw_polygon(verts, 8, ALLEGRO_LINE_JOIN_ROUND, al_map_rgb(255, 255, 255), 1.2f, 1.0f);
    }

    // ------------------------------------------------------------------
    // Dibujo: elementos del juego
    // ------------------------------------------------------------------
    void dibujarEnemigo(const Enemigo& e)
    {
        const double ang = aRadianes(e.theta);
        const float ex = CX + static_cast<float>(cos(ang) * e.r);
        const float ey = CY + static_cast<float>(sin(ang) * e.r);
        const float rad = static_cast<float>(e.radio);
        const double haciaNucleo = ang + PI_; // las formas apuntan al nucleo
        const ALLEGRO_COLOR c = colorFase(e.color);

        switch (e.tipo)
        {
            case TipoEnemigo::Dron: // circulo
                al_draw_filled_circle(ex, ey, rad, c);
                al_draw_circle(ex, ey, rad, al_map_rgb(255, 255, 255), 1.2f);
                break;
            case TipoEnemigo::Girador: // estrella de 4 puntas que gira
                estrella(ex, ey, rad, ang * 3.0, c);
                break;
            case TipoEnemigo::Divisor: // triangulo grande
                poligono(ex, ey, rad * 1.3f, 3, haciaNucleo, c);
                break;
            case TipoEnemigo::Mini: // rombo pequeno
                poligono(ex, ey, rad * 1.3f, 4, ang * 3.0, c);
                break;
            case TipoEnemigo::Tanque: // hexagono con puntos de vida
                poligono(ex, ey, rad, 6, haciaNucleo, c);
                for (int i = 0; i < e.vida && i < 3; ++i)
                    al_draw_filled_circle(ex - 6.0f + i * 6.0f, ey, 2.0f, al_map_rgb(255, 255, 255));
                break;
            case TipoEnemigo::Camaleon: // cuadrado con anillo de cuenta regresiva
                poligono(ex, ey, rad * 1.3f, 4, PI_ / 4.0, c);
                al_draw_arc(ex, ey, rad + 6.0f, -static_cast<float>(PI_ / 2.0),
                            static_cast<float>(2.0 * PI_ * (42 - e.ticksColor) / 42.0), al_map_rgb(255, 255, 255), 2.0f);
                break;
            default:
                al_draw_filled_circle(ex, ey, rad, c);
                break;
        }
    }

    void dibujarBalas(const ListaBalas& balas)
    {
        for (const NodoBala* b = balas.cabeza; b != nullptr; b = b->sig)
        {
            const float bx = CX + static_cast<float>(b->dato.x);
            const float by = CY + static_cast<float>(b->dato.y);
            const ALLEGRO_COLOR c = colorFase(b->dato.color);
            al_draw_line(bx, by, bx - static_cast<float>(b->dato.vx) * 1.5f, by - static_cast<float>(b->dato.vy) * 1.5f, conAlfa(c, 0.6f), 2.0f);
            al_draw_filled_circle(bx, by, 3.0f, c);
        }
    }

    void dibujarNucleo(Vista& v)
    {
        const Jugador* j = v.motor.jugadorActual;
        const float frac = (j != nullptr) ? j->vidaNucleo / 100.0f : 1.0f;

        al_draw_filled_circle(CX, CY, 20.0f, al_map_rgb(210, 190, 60));
        al_draw_circle(CX, CY, 20.0f, al_map_rgb(255, 245, 160), 2.0f);
        if (v.flashNucleo > 0)
            al_draw_filled_circle(CX, CY, 26.0f, conAlfa(al_map_rgb(255, 40, 40), v.flashNucleo / 10.0f));

        // Arco de vida alrededor del nucleo
        al_draw_circle(CX, CY, 32.0f, al_map_rgb(35, 40, 60), 5.0f);
        if (frac > 0.0f)
        {
            const ALLEGRO_COLOR c = al_map_rgb(static_cast<int>(255 * (1.0f - frac)), static_cast<int>(60 + 180 * frac), 70);
            al_draw_arc(CX, CY, 32.0f, -static_cast<float>(PI_ / 2.0), static_cast<float>(2.0 * PI_ * frac), c, 5.0f);
        }
    }

    void dibujarNave(Vista& v)
    {
        const double ang = aRadianes(v.motor.anguloNave);
        const float sx = CX + static_cast<float>(cos(ang) * RADIO_ANILLO);
        const float sy = CY + static_cast<float>(sin(ang) * RADIO_ANILLO);
        const int fase = (v.motor.jugadorActual != nullptr) ? v.motor.jugadorActual->colorActivo : 0;
        const ALLEGRO_COLOR c = colorFase(fase);

        al_draw_filled_triangle(
            sx + static_cast<float>(cos(ang)) * 22.0f, sy + static_cast<float>(sin(ang)) * 22.0f,
            sx + static_cast<float>(cos(ang + 2.4)) * 13.0f, sy + static_cast<float>(sin(ang + 2.4)) * 13.0f,
            sx + static_cast<float>(cos(ang - 2.4)) * 13.0f, sy + static_cast<float>(sin(ang - 2.4)) * 13.0f, c);
        al_draw_triangle(
            sx + static_cast<float>(cos(ang)) * 22.0f, sy + static_cast<float>(sin(ang)) * 22.0f,
            sx + static_cast<float>(cos(ang + 2.4)) * 13.0f, sy + static_cast<float>(sin(ang + 2.4)) * 13.0f,
            sx + static_cast<float>(cos(ang - 2.4)) * 13.0f, sy + static_cast<float>(sin(ang - 2.4)) * 13.0f,
            al_map_rgb(255, 255, 255), 1.5f);
    }

    void dibujarOndaPulso(Vista& v)
    {
        if (!v.motor.ondaActiva) return;
        const float r = static_cast<float>(v.motor.ondaR);
        float a = 1.0f - (r - 20.0f) / 320.0f * 0.75f;
        if (a < 0.2f) a = 0.2f;
        const ALLEGRO_COLOR c = al_map_rgb(190, 240, 255);
        al_draw_circle(CX, CY, r, conAlfa(c, a), 5.0f);
        if (r > 14.0f) al_draw_circle(CX, CY, r - 8.0f, conAlfa(c, a * 0.5f), 2.0f);
    }

    void dibujarParticulas(Vista& v)
    {
        for (int i = 0; i < MAX_PARTICULAS; ++i)
        {
            const Particula& p = v.particulas[i];
            if (!p.activa) continue;
            const float a = static_cast<float>(p.vida) / p.vidaMax;
            al_draw_filled_circle(p.x, p.y, 1.5f + 2.0f * a, conAlfa(colorFase(p.color), a));
        }
    }

    void dibujarHUD(Vista& v)
    {
        const Jugador* j = v.motor.jugadorActual;
        if (j == nullptr) return;

        // Panel izquierdo: datos del jugador en turno
        panel(10, 10, 270, 232);
        al_draw_textf(v.fuente, al_map_rgb(255, 255, 255), 22, 18, 0, "Turno: %s%s", j->alias.c_str(), j->esBot ? " [BOT]" : "");
        al_draw_textf(v.fuente, al_map_rgb(180, 200, 255), 22, 44, 0, "Oleada %d / %d", v.partida.oleada, v.partida.limiteOleadas);
        al_draw_textf(v.fuente, al_map_rgb(255, 255, 255), 22, 70, 0, "Puntaje: %d", j->puntaje);
        al_draw_textf(v.fuente, al_map_rgb(255, 230, 120), 22, 96, 0, "Combo: %d   Mult: x%d", j->combo, multiplicador(j->combo));

        al_draw_text(v.fuentePeq, al_map_rgb(200, 200, 200), 22, 126, 0, "Nucleo");
        al_draw_textf(v.fuentePeq, al_map_rgb(200, 200, 200), 258, 126, ALLEGRO_ALIGN_RIGHT, "%d", j->vidaNucleo);
        const float fv = j->vidaNucleo / 100.0f;
        barra(22, 144, 236, 12, fv, al_map_rgb(static_cast<int>(255 * (1.0f - fv)), static_cast<int>(60 + 180 * fv), 70));

        al_draw_text(v.fuentePeq, al_map_rgb(200, 200, 200), 22, 164, 0, "Pulso");
        al_draw_textf(v.fuentePeq, al_map_rgb(200, 200, 200), 258, 164, ALLEGRO_ALIGN_RIGHT, "%d%%", j->cargaPulso);
        const bool listo = (j->cargaPulso >= 100);
        const bool parpadeo = ((v.motor.tickActual / 6) % 2) == 0;
        barra(22, 182, 236, 12, j->cargaPulso / 100.0f, (listo && parpadeo) ? al_map_rgb(255, 255, 255) : al_map_rgb(120, 210, 255));
        if (listo)
            al_draw_text(v.fuentePeq, al_map_rgb(255, 255, 255), 22, 200, 0, "PULSO LISTO - ESPACIO");

        // Fase activa
        al_draw_filled_rectangle(22, 214, 38, 226, colorFase(j->colorActivo));
        al_draw_textf(v.fuentePeq, al_map_rgb(220, 220, 220), 46, 211, 0, "Fase: %s", nombreFase(j->colorActivo));

        // Marcador lateral (derecha)
        const float x0 = PANTALLA_W - 250.0f;
        panel(x0, 10, PANTALLA_W - 10.0f, 40.0f + v.partida.cantidad * 26.0f);
        al_draw_text(v.fuentePeq, al_map_rgb(150, 180, 230), x0 + 12, 16, 0, "MARCADOR");
        for (int i = 0; i < v.partida.cantidad; ++i)
        {
            const Jugador& pj = v.partida.jugadores[i];
            const bool esActual = (&pj == j);
            const bool vivo = pj.vidaNucleo > 0;
            const ALLEGRO_COLOR c = esActual ? al_map_rgb(255, 255, 120) : (vivo ? al_map_rgb(220, 220, 220) : al_map_rgb(110, 110, 110));
            al_draw_textf(v.fuentePeq, c, x0 + 12, 38.0f + i * 26.0f, 0, "%s%s", esActual ? "> " : "  ", pj.alias.c_str());
            al_draw_textf(v.fuentePeq, c, PANTALLA_W - 22.0f, 38.0f + i * 26.0f, ALLEGRO_ALIGN_RIGHT, "%d | N:%d", pj.puntaje, pj.vidaNucleo);
        }

        // Ayuda de controles
        al_draw_text(v.fuentePeq, al_map_rgb(150, 160, 190), CX, PANTALLA_H - 24.0f, ALLEGRO_ALIGN_CENTER,
            "Mouse / flechas: girar    1 2 3: fase C M A    ESPACIO: pulso    P o ESC: pausa");
    }

    void dibujarEscenaJuego(Vista& v)
    {
        dibujarFondo(v, 0.5f);

        al_draw_circle(CX, CY, static_cast<float>(RADIO_CAMPO), al_map_rgb(28, 38, 62), 1.0f);
        al_draw_circle(CX, CY, static_cast<float>(RADIO_ANILLO), al_map_rgb(90, 130, 200), 2.0f);

        if (v.destelloPulso > 0)
            al_draw_filled_circle(CX, CY, 60.0f, conAlfa(al_map_rgb(190, 240, 255), v.destelloPulso / 8.0f * 0.6f));

        dibujarNucleo(v);

        for (NodoEnemigo* e = v.motor.listaEnemigos.primero(); e != nullptr; e = e->sig)
            dibujarEnemigo(e->dato);

        dibujarBalas(v.motor.listaBalas);
        dibujarOndaPulso(v);
        dibujarNave(v);
        dibujarParticulas(v);
        dibujarHUD(v);
    }

    // ------------------------------------------------------------------
    // Dibujo: pantallas
    // ------------------------------------------------------------------
    void dibujarMenu(Vista& v)
    {
        dibujarFondo(v, 0.8f);
        al_draw_text(v.fuenteTitulo, al_map_rgb(255, 255, 255), CX, 110, ALLEGRO_ALIGN_CENTER, "FASE ORBITAL");
        al_draw_text(v.fuente, al_map_rgb(150, 190, 255), CX, 165, ALLEGRO_ALIGN_CENTER, "Defiende el nucleo con la fase de color correcta");

        const char* opciones[4] = { "1  Jugar", "2  Estadisticas", "3  Ranking top 10", "4  Salir" };
        for (int i = 0; i < 4; ++i)
        {
            const bool sel = (i == v.opcionMenu);
            if (sel)
                al_draw_filled_rectangle(CX - 170, 250.0f + i * 56.0f - 6.0f, CX + 170, 250.0f + i * 56.0f + 34.0f, al_map_rgba(40, 80, 140, 170));
            al_draw_text(v.fuente, sel ? al_map_rgb(255, 255, 255) : al_map_rgb(150, 190, 230), CX, 250.0f + i * 56.0f, ALLEGRO_ALIGN_CENTER, opciones[i]);
        }

        al_draw_text(v.fuentePeq, al_map_rgb(170, 170, 190), CX, PANTALLA_H - 70.0f, ALLEGRO_ALIGN_CENTER, "Flechas arriba/abajo + ENTER, o presiona el numero");
        al_draw_text(v.fuentePeq, al_map_rgb(255, 200, 100), CX, PANTALLA_H - 44.0f, ALLEGRO_ALIGN_CENTER,
            "En partida: mouse/flechas giran - 1/2/3 cambian de fase - ESPACIO pulso - P pausa");
    }

    void dibujarRegistro(Vista& v)
    {
        dibujarFondo(v, 0.6f);
        al_draw_text(v.fuenteTitulo, al_map_rgb(255, 255, 255), CX, 50, ALLEGRO_ALIGN_CENTER, "REGISTRO DE JUGADORES");
        al_draw_text(v.fuentePeq, al_map_rgb(180, 200, 255), CX, 105, ALLEGRO_ALIGN_CENTER,
            "Escribe un alias y ENTER. Si empieza con BOT lo controla la computadora. TAB agrega un bot.");

        // Caja de texto
        al_draw_filled_rectangle(CX - 160, 140, CX + 160, 180, al_map_rgb(15, 22, 40));
        al_draw_rectangle(CX - 160, 140, CX + 160, 180, al_map_rgb(100, 160, 255), 2.0f);
        const bool cursor = (al_get_time() * 2.0 - floor(al_get_time() * 2.0)) < 0.5;
        string textoCaja = v.aliasEscrito + (cursor ? "_" : " ");
        al_draw_text(v.fuente, al_map_rgb(255, 255, 255), CX - 150, 149, 0, textoCaja.c_str());

        // Lista de jugadores registrados
        panel(CX - 260, 210, CX + 260, 230.0f + MAX_JUGADORES * 34.0f);
        al_draw_textf(v.fuentePeq, al_map_rgb(150, 180, 230), CX - 246, 216, 0, "JUGADORES (%d/%d) - orden de turnos", v.partida.cantidad, MAX_JUGADORES);
        for (int i = 0; i < v.partida.cantidad; ++i)
        {
            const Jugador& j = v.partida.jugadores[i];
            al_draw_textf(v.fuente, al_map_rgb(255, 255, 255), CX - 246, 244.0f + i * 34.0f, 0, "%d. %s", i + 1, j.alias.c_str());
            al_draw_text(v.fuente, j.esBot ? al_map_rgb(255, 190, 80) : al_map_rgb(120, 255, 160), CX + 246, 244.0f + i * 34.0f, ALLEGRO_ALIGN_RIGHT, j.esBot ? "BOT" : "HUMANO");
        }

        if (!v.mensajeRegistro.empty())
            al_draw_text(v.fuente, al_map_rgb(255, 120, 120), CX, 470 + 0.0f, ALLEGRO_ALIGN_CENTER, v.mensajeRegistro.c_str());

        al_draw_textf(v.fuente, al_map_rgb(255, 230, 140), CX, 520, ALLEGRO_ALIGN_CENTER, "Oleadas de la partida: %d   (flechas izquierda / derecha)", v.limiteOleadas);
        al_draw_text(v.fuente, al_map_rgb(120, 230, 255), CX, 560, ALLEGRO_ALIGN_CENTER, "F2 - Iniciar partida");
        al_draw_text(v.fuentePeq, al_map_rgb(170, 170, 190), CX, 600, ALLEGRO_ALIGN_CENTER, "ESC - Volver al menu");
    }

    void dibujarAnuncioTurno(Vista& v)
    {
        dibujarFondo(v, 0.6f);
        const Jugador* j = v.motor.jugadorActual;
        if (j == nullptr) return;

        al_draw_textf(v.fuenteTitulo, al_map_rgb(255, 255, 255), CX, 170, ALLEGRO_ALIGN_CENTER, "OLEADA %d", v.partida.oleada);
        al_draw_textf(v.fuenteTitulo, al_map_rgb(255, 230, 120), CX, 240, ALLEGRO_ALIGN_CENTER, "Turno de %s", j->alias.c_str());
        al_draw_text(v.fuente, j->esBot ? al_map_rgb(255, 190, 80) : al_map_rgb(120, 255, 160), CX, 305, ALLEGRO_ALIGN_CENTER, j->esBot ? "Jugador automatico (BOT)" : "Jugador humano");
        al_draw_textf(v.fuente, al_map_rgb(190, 210, 255), CX, 350, ALLEGRO_ALIGN_CENTER, "Sentido de la ronda: %s", v.partida.horario ? "horario" : "antihorario");
        al_draw_textf(v.fuente, al_map_rgb(255, 255, 255), CX, 385, ALLEGRO_ALIGN_CENTER, "Puntaje: %d     Nucleo: %d     Pulso: %d%%", j->puntaje, j->vidaNucleo, j->cargaPulso);
        al_draw_text(v.fuente, al_map_rgb(120, 230, 255), CX, 470, ALLEGRO_ALIGN_CENTER, j->esBot ? "Empieza automaticamente..." : "ENTER - Empezar (o espera 2 segundos)");
    }

    void dibujarPausa(Vista& v)
    {
        dibujarEscenaJuego(v);
        al_draw_filled_rectangle(0, 0, PANTALLA_W, PANTALLA_H, al_map_rgba(0, 0, 0, 120));
        panel(CX - 200, CY - 110, CX + 200, CY + 110);
        al_draw_text(v.fuenteTitulo, al_map_rgb(255, 255, 255), CX, CY - 85, ALLEGRO_ALIGN_CENTER, "PAUSA");
        al_draw_text(v.fuente, al_map_rgb(255, 255, 255), CX, CY - 10, ALLEGRO_ALIGN_CENTER, "P o ESC - Reanudar");
        al_draw_text(v.fuente, al_map_rgb(255, 160, 160), CX, CY + 30, ALLEGRO_ALIGN_CENTER, "Q - Abandonar partida");
    }

    void dibujarResumenOleada(Vista& v)
    {
        dibujarEscenaJuego(v);
        al_draw_filled_rectangle(0, 0, PANTALLA_W, PANTALLA_H, al_map_rgba(0, 0, 0, 150));
        panel(CX - 280, CY - 170, CX + 280, CY + 170);

        const Jugador* j = (v.idxTurno >= 0) ? &v.partida.jugadores[v.idxTurno] : nullptr;
        al_draw_text(v.fuenteTitulo, v.turnoCompletado ? al_map_rgb(120, 255, 160) : al_map_rgb(255, 110, 110), CX, CY - 150, ALLEGRO_ALIGN_CENTER,
                     v.turnoCompletado ? "OLEADA COMPLETADA" : "NUCLEO DESTRUIDO");
        if (j != nullptr)
        {
            al_draw_textf(v.fuente, al_map_rgb(255, 255, 255), CX, CY - 80, ALLEGRO_ALIGN_CENTER, "%s", j->alias.c_str());
            al_draw_textf(v.fuente, al_map_rgb(255, 255, 255), CX, CY - 45, ALLEGRO_ALIGN_CENTER, "Puntaje: %d", j->puntaje);
            al_draw_textf(v.fuente, al_map_rgb(255, 255, 255), CX, CY - 10, ALLEGRO_ALIGN_CENTER, "Nucleo: %d%s", j->vidaNucleo,
                          v.turnoCompletado ? "   (+8 de reparacion)" : "   (eliminado de la ronda)");
        }
        const char* sig = "";
        if (v.resultado == ResultadoTurno::PartidaTerminada) sig = "Fin de la partida";
        else if (v.resultado == ResultadoTurno::NuevaRonda) sig = "Termina la ronda: cambia el sentido";
        else sig = "Sigue el siguiente jugador";
        al_draw_text(v.fuente, al_map_rgb(190, 210, 255), CX, CY + 40, ALLEGRO_ALIGN_CENTER, sig);
        al_draw_text(v.fuente, al_map_rgb(120, 230, 255), CX, CY + 100, ALLEGRO_ALIGN_CENTER,
                     v.turnoFueBot ? "Continua automaticamente... (ENTER para saltar)" : "ENTER - Continuar");
    }

    // Tabla de posiciones (se reutiliza en fin de partida, estadisticas y ranking)
    void dibujarTabla(Vista& v, const char* titulo, const char* nota, bool resaltarGanador)
    {
        dibujarFondo(v, 0.6f);
        al_draw_text(v.fuenteTitulo, al_map_rgb(255, 255, 255), CX, 50, ALLEGRO_ALIGN_CENTER, titulo);
        al_draw_text(v.fuentePeq, al_map_rgb(170, 190, 230), CX, 105, ALLEGRO_ALIGN_CENTER, nota);

        panel(CX - 380, 140, CX + 380, 190.0f + MAX_JUGADORES * 40.0f);
        const float yc = 152;
        al_draw_text(v.fuentePeq, al_map_rgb(150, 180, 230), CX - 360, yc, 0, "POS");
        al_draw_text(v.fuentePeq, al_map_rgb(150, 180, 230), CX - 300, yc, 0, "JUGADOR");
        al_draw_text(v.fuentePeq, al_map_rgb(150, 180, 230), CX + 30, yc, ALLEGRO_ALIGN_RIGHT, "PUNTAJE");
        al_draw_text(v.fuentePeq, al_map_rgb(150, 180, 230), CX + 150, yc, ALLEGRO_ALIGN_RIGHT, "OLEADA");
        al_draw_text(v.fuentePeq, al_map_rgb(150, 180, 230), CX + 270, yc, ALLEGRO_ALIGN_RIGHT, "NUCLEO");
        al_draw_text(v.fuentePeq, al_map_rgb(150, 180, 230), CX + 370, yc, ALLEGRO_ALIGN_RIGHT, "TIPO");

        if (!v.partidaJugada || v.partida.cantidad == 0)
        {
            al_draw_text(v.fuente, al_map_rgb(220, 220, 220), CX, 230, ALLEGRO_ALIGN_CENTER, "Todavia no hay partidas jugadas en esta sesion.");
        }
        else
        {
            int orden[MAX_JUGADORES];
            calcularPosiciones(v.partida, orden);
            for (int k = 0; k < v.partida.cantidad; ++k)
            {
                const int i = orden[k];
                const Jugador& j = v.partida.jugadores[i];
                const float y = 182.0f + k * 40.0f;
                const bool ganador = resaltarGanador && (k == 0);
                const ALLEGRO_COLOR c = ganador ? al_map_rgb(255, 235, 120) : al_map_rgb(235, 235, 235);
                if (ganador)
                    al_draw_filled_rectangle(CX - 372, y - 4, CX + 372, y + 30, al_map_rgba(90, 80, 20, 150));
                al_draw_textf(v.fuente, c, CX - 360, y, 0, "%d", k + 1);
                al_draw_textf(v.fuente, c, CX - 300, y, 0, "%s%s", j.alias.c_str(), ganador ? "  (GANADOR)" : "");
                al_draw_textf(v.fuente, c, CX + 30, y, ALLEGRO_ALIGN_RIGHT, "%d", j.puntaje);
                al_draw_textf(v.fuente, c, CX + 150, y, ALLEGRO_ALIGN_RIGHT, "%d", v.partida.oleadaAlcanzada[i]);
                al_draw_textf(v.fuente, c, CX + 270, y, ALLEGRO_ALIGN_RIGHT, "%d", j.vidaNucleo);
                al_draw_text(v.fuente, j.esBot ? al_map_rgb(255, 190, 80) : al_map_rgb(120, 255, 160), CX + 370, y, ALLEGRO_ALIGN_RIGHT, j.esBot ? "BOT" : "HUMANO");
            }
        }
        al_draw_text(v.fuente, al_map_rgb(120, 230, 255), CX, PANTALLA_H - 70.0f, ALLEGRO_ALIGN_CENTER, "ENTER - Volver al menu");
    }

    void dibujarPantalla(Vista& v)
    {
        switch (v.pantalla)
        {
            case Pantalla::Menu:          dibujarMenu(v); break;
            case Pantalla::Registro:      dibujarRegistro(v); break;
            case Pantalla::AnuncioTurno:  dibujarAnuncioTurno(v); break;
            case Pantalla::Juego:         dibujarEscenaJuego(v); break;
            case Pantalla::Pausa:         dibujarPausa(v); break;
            case Pantalla::ResumenOleada: dibujarResumenOleada(v); break;
            case Pantalla::FinPartida:
                dibujarTabla(v, "FIN DE PARTIDA", "Posiciones finales", true);
                break;
            case Pantalla::Estadisticas:
                // TODO (modulo Estadisticas): aqui mostrar media, mediana, desviacion e histograma leidos de historial.csv
                dibujarTabla(v, "ESTADISTICAS", "Resumen de la ultima partida de esta sesion (pendiente: conectar con historial.csv)", false);
                break;
            case Pantalla::Ranking:
                // TODO (modulo Ranking): aqui mostrar Ranking.top(10) construido con insercion ordenada
                dibujarTabla(v, "RANKING TOP 10", "Posiciones de la ultima partida de esta sesion (pendiente: conectar con historial.csv)", false);
                break;
        }
        al_flip_display();
    }
}

// ----------------------------------------------------------------------
// Bucle principal (seccion 7.4): cola de eventos + temporizador a 30 por segundo.
// Cada evento del temporizador ejecuta exactamente UN tick; se dibuja solo con la cola vacia.
// ----------------------------------------------------------------------
int ejecutarVistaAllegro()
{
    Vista v;
    inicializarMotor(v.motor);     // antes de cualquier cosa: deja la cola de spawn en estado valido
    inicializarPartida(v.partida);

    int codigo = 0;
    try
    {
        inicializarVista(v);
        al_start_timer(v.timer);

        bool redibujar = true;
        while (v.ejecutando)
        {
            ALLEGRO_EVENT ev;
            al_wait_for_event(v.cola, &ev);

            switch (ev.type)
            {
                case ALLEGRO_EVENT_DISPLAY_CLOSE:
                    v.ejecutando = false;
                    break;

                case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT: // la ventana perdio el foco: pausa automatica
                    if (v.pantalla == Pantalla::Juego)
                        v.pantalla = Pantalla::Pausa;
                    break;

                case ALLEGRO_EVENT_TIMER:
                    tickVista(v);
                    redibujar = true;
                    break;

                case ALLEGRO_EVENT_MOUSE_AXES:
                    v.mouseX = ev.mouse.x;
                    v.mouseY = ev.mouse.y;
                    v.usarMouse = true;
                    break;

                case ALLEGRO_EVENT_KEY_DOWN:
                    teclaPantalla(v, ev.keyboard.keycode);
                    break;

                case ALLEGRO_EVENT_KEY_CHAR:
                    caracterRegistro(v, ev.keyboard.keycode, ev.keyboard.unichar);
                    break;

                default:
                    break;
            }

            if (redibujar && al_is_event_queue_empty(v.cola))
            {
                dibujarPantalla(v);
                redibujar = false;
            }
        }
    }
    catch (const exception& e)
    {
        cerr << "ERROR: " << e.what() << endl;
        codigo = 1;
    }

    destruirVista(v);
    return codigo;
}
