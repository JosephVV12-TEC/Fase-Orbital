#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <stdexcept>

#include <allegro5/allegro5.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#include "Juego.h"

using namespace std;

namespace
{
    const int SCREEN_W = 1024;
    const int SCREEN_H = 768;
    const int CENTER_X = SCREEN_W / 2;
    const int CENTER_Y = SCREEN_H / 2;

    enum class ScreenState
    {
        MainMenu,
        Register,
        Gameplay,
        Pause,
        EndWave,
        EndMatch,
        Statistics,
        Ranking
    };

    struct AppState
    {
        Motor motor;
        Jugador jugador;
        ColaSpawn oleada;
        ScreenState screen = ScreenState::MainMenu;
        ALLEGRO_DISPLAY* display = nullptr;
        ALLEGRO_EVENT_QUEUE* queue = nullptr;
        ALLEGRO_TIMER* timer = nullptr;
        ALLEGRO_FONT* font = nullptr;
        ALLEGRO_FONT* titleFont = nullptr;
        bool running = true;
        bool paused = false;
        int pendingColor = -1;
        int mouseX = CENTER_X;
        int mouseY = CENTER_Y;
    };

    double normalizarAngulo(double angulo)
    {
        while (angulo < 0.0)
            angulo += 360.0;
        while (angulo >= 360.0)
            angulo -= 360.0;
        return angulo;
    }

    void actualizarRotacionDesdeCursor(AppState& app)
    {
        if (app.screen != ScreenState::Gameplay)
            return;

        double dx = static_cast<double>(app.mouseX - CENTER_X);
        double dy = static_cast<double>(app.mouseY - CENTER_Y);
        double objetivo = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
        objetivo = normalizarAngulo(objetivo);

        double delta = objetivo - app.motor.anguloNave;
        if (delta > 180.0)
            delta -= 360.0;
        else if (delta < -180.0)
            delta += 360.0;

        if (delta > 6.0)
            delta = 6.0;
        else if (delta < -6.0)
            delta = -6.0;

        app.motor.anguloNave = normalizarAngulo(app.motor.anguloNave + delta);
    }

    ALLEGRO_COLOR colorParaCodigo(int codigo)
    {
        switch (codigo)
        {
            case 0: return al_map_rgb(0, 255, 255);
            case 1: return al_map_rgb(255, 0, 255);
            case 2: return al_map_rgb(255, 180, 0);
            default: return al_map_rgb(200, 200, 200);
        }
    }

    double radianes(double grados)
    {
        return grados * 3.14159265358979323846 / 180.0;
    }

    void prepararOleadaPrueba(ColaSpawn& cola)
    {
        inicializarCola(cola);
        for (int i = 0; i < 12; ++i)
        {
            Spawn s;
            s.tipoEnemigo = i % 6;
            s.tickSpawn = i + 1;
            s.color = i % 3;
            s.angulo = (i * 33) % 360;
            s.sentido = (i % 2 == 0) ? 1 : -1;
            encolar(cola, s);
        }
    }

    void inicializarAplicacion(AppState& app)
    {
        if (!al_init())
            throw runtime_error("No se pudo inicializar Allegro.");
        if (!al_init_primitives_addon())
            throw runtime_error("No se pudo inicializar primitives.");
        if (!al_init_font_addon())
            throw runtime_error("No se pudo inicializar font.");
        if (!al_init_ttf_addon())
            throw runtime_error("No se pudo inicializar ttf.");
        if (!al_init_image_addon())
            throw runtime_error("No se pudo inicializar image.");
        if (!al_install_keyboard())
            throw runtime_error("No se pudo instalar teclado.");
        if (!al_install_mouse())
            throw runtime_error("No se pudo instalar mouse.");

        app.display = al_create_display(SCREEN_W, SCREEN_H);
        if (!app.display)
            throw runtime_error("No se pudo crear la ventana.");

        app.queue = al_create_event_queue();
        if (!app.queue)
            throw runtime_error("No se pudo crear la cola de eventos.");

        app.timer = al_create_timer(1.0 / 30.0);
        if (!app.timer)
            throw runtime_error("No se pudo crear el temporizador.");

        app.font = al_load_font("C:\\Windows\\Fonts\\arial.ttf", 18, 0);
        if (!app.font)
            app.font = al_create_builtin_font();

        app.titleFont = al_load_font("C:\\Windows\\Fonts\\arial.ttf", 32, 0);
        if (!app.titleFont)
            app.titleFont = al_create_builtin_font();

        if (!app.font || !app.titleFont)
            throw runtime_error("No se pudo cargar la fuente del juego.");

        al_register_event_source(app.queue, al_get_display_event_source(app.display));
        al_register_event_source(app.queue, al_get_timer_event_source(app.timer));
        al_register_event_source(app.queue, al_get_keyboard_event_source());
        al_register_event_source(app.queue, al_get_mouse_event_source());

        inicializarJugador(app.jugador, "Jugador 1", false);
        app.jugador.vidaNucleo = 100;
        app.jugador.colorActivo = 0;
        prepararOleadaPrueba(app.oleada);

        inicializarMotor(app.motor);
        iniciarTurnoMotor(app.motor, &app.jugador, app.oleada);
    }

    void destruirAplicacion(AppState& app)
    {
        if (app.font)
            al_destroy_font(app.font);
        if (app.titleFont)
            al_destroy_font(app.titleFont);
        if (app.timer)
            al_destroy_timer(app.timer);
        if (app.queue)
            al_destroy_event_queue(app.queue);
        if (app.display)
            al_destroy_display(app.display);
    }

    void dibujarMenu(AppState& app)
    {
        al_clear_to_color(al_map_rgb(8, 10, 18));
        al_draw_text(app.titleFont, al_map_rgb(255, 255, 255), CENTER_X, 100, ALLEGRO_ALIGN_CENTER, "FASE ORBITAL");
        al_draw_text(app.font, al_map_rgb(180, 180, 180), CENTER_X, 180, ALLEGRO_ALIGN_CENTER, "Menu principal");
        al_draw_text(app.font, al_map_rgb(100, 220, 255), CENTER_X, 260, ALLEGRO_ALIGN_CENTER, "ENTER - Iniciar partida");
        al_draw_text(app.font, al_map_rgb(100, 220, 255), CENTER_X, 300, ALLEGRO_ALIGN_CENTER, "P - Pausa");
        al_draw_text(app.font, al_map_rgb(100, 220, 255), CENTER_X, 340, ALLEGRO_ALIGN_CENTER, "ESC - Salir");
        al_draw_text(app.font, al_map_rgb(255, 200, 100), CENTER_X, 420, ALLEGRO_ALIGN_CENTER, "Controles: flechas para girar, 1/2/3 para color, ESPACIO para pulso");
    }

    void dibujarRegistro(AppState& app)
    {
        al_clear_to_color(al_map_rgb(12, 15, 30));
        al_draw_text(app.titleFont, al_map_rgb(255, 255, 255), CENTER_X, 90, ALLEGRO_ALIGN_CENTER, "REGISTRO DEL JUGADOR");
        al_draw_text(app.font, al_map_rgb(255, 255, 255), CENTER_X, 180, ALLEGRO_ALIGN_CENTER, app.jugador.alias.c_str());
        al_draw_text(app.font, al_map_rgb(90, 220, 255), CENTER_X, 270, ALLEGRO_ALIGN_CENTER, "ENTER para continuar");
    }

    void dibujarHUD(AppState& app)
    {
        stringstream score;
        score << "Puntaje: " << app.jugador.puntaje;
        stringstream combo;
        combo << "Combo: x" << app.jugador.combo;
        stringstream carga;
        carga << "Pulso: " << app.jugador.cargaPulso << "%";
        stringstream salud;
        salud << "Nucleo: " << app.jugador.vidaNucleo;

        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 20, 20, 0, "Jugador: %s", app.jugador.alias.c_str());
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 20, 50, 0, score.str().c_str());
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 20, 80, 0, combo.str().c_str());
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 20, 110, 0, carga.str().c_str());
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 20, 140, 0, salud.str().c_str());
        al_draw_textf(app.font, colorParaCodigo(app.jugador.colorActivo), 20, 170, 0, "Color activo: %d", app.jugador.colorActivo);
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 20, 200, 0, "Tick: %d", app.motor.tickActual);
    }

    void dibujarEnemigo(const Enemigo& enemigo)
    {
        const double angle = radianes(enemigo.theta);
        const double ex = CENTER_X + std::cos(angle) * enemigo.r;
        const double ey = CENTER_Y + std::sin(angle) * enemigo.r;
        const ALLEGRO_COLOR color = colorParaCodigo(enemigo.color);

        switch (enemigo.tipo)
        {
            case TipoEnemigo::Dron:
                al_draw_filled_circle(ex, ey, enemigo.radio, color);
                break;
            case TipoEnemigo::Girador:
                al_draw_circle(ex, ey, enemigo.radio + 2, color, 2);
                al_draw_filled_circle(ex, ey, enemigo.radio - 2, color);
                break;
            case TipoEnemigo::Divisor:
                al_draw_filled_triangle(ex, ey - enemigo.radio, ex - enemigo.radio, ey + enemigo.radio, ex + enemigo.radio, ey + enemigo.radio, color);
                break;
            case TipoEnemigo::Mini:
                al_draw_filled_circle(ex, ey, enemigo.radio, color);
                break;
            case TipoEnemigo::Tanque:
                al_draw_filled_circle(ex, ey, enemigo.radio + 4, color);
                al_draw_circle(ex, ey, enemigo.radio - 3, al_map_rgb(255, 255, 255), 1);
                break;
            case TipoEnemigo::Camaleon:
                al_draw_filled_rectangle(ex - enemigo.radio, ey - enemigo.radio, ex + enemigo.radio, ey + enemigo.radio, color);
                break;
            default:
                al_draw_filled_circle(ex, ey, enemigo.radio, color);
                break;
        }
    }

    void dibujarBalas(const ListaBalas& balas)
    {
        for (NodoBala* bala = balas.cabeza; bala != nullptr; bala = bala->sig)
        {
            const double bx = CENTER_X + bala->dato.x;
            const double by = CENTER_Y + bala->dato.y;
            al_draw_filled_circle(bx, by, 3.0, colorParaCodigo(bala->dato.color));
        }
    }

    void dibujarNave(AppState& app)
    {
        const double angle = radianes(app.motor.anguloNave);
        const double noseX = CENTER_X + std::cos(angle) * 34.0;
        const double noseY = CENTER_Y + std::sin(angle) * 34.0;
        const double wingFront = 12.0;

        al_draw_filled_circle(CENTER_X, CENTER_Y, 22.0, al_map_rgb(120, 120, 120));
        al_draw_circle(CENTER_X, CENTER_Y, 60.0, al_map_rgb(100, 140, 180), 2);
        al_draw_filled_triangle(
            noseX, noseY,
            CENTER_X + std::cos(angle + 2.2) * wingFront, CENTER_Y + std::sin(angle + 2.2) * wingFront,
            CENTER_X + std::cos(angle - 2.2) * wingFront, CENTER_Y + std::sin(angle - 2.2) * wingFront,
            colorParaCodigo(app.jugador.colorActivo));
    }

    void dibujarPantallaJuego(AppState& app)
    {
        al_clear_to_color(al_map_rgb(3, 6, 15));
        al_draw_circle(CENTER_X, CENTER_Y, 160.0, al_map_rgb(80, 120, 170), 2);
        al_draw_filled_circle(CENTER_X, CENTER_Y, 22.0, al_map_rgb(200, 180, 40));

        for (NodoEnemigo* enemigo = app.motor.listaEnemigos.primero(); enemigo != nullptr; enemigo = enemigo->sig)
            dibujarEnemigo(enemigo->dato);

        dibujarBalas(app.motor.listaBalas);
        dibujarNave(app);

        if (app.jugador.cargaPulso >= 100)
            al_draw_circle(CENTER_X, CENTER_Y, 80.0, al_map_rgb(120, 220, 255), 3);

        dibujarHUD(app);
    }

    void dibujarPausa(AppState& app)
    {
        dibujarPantallaJuego(app);
        al_draw_filled_rectangle(260, 280, 764, 480, al_map_rgba(0, 0, 0, 170));
        al_draw_text(app.titleFont, al_map_rgb(255, 255, 255), CENTER_X, 330, ALLEGRO_ALIGN_CENTER, "PAUSA");
        al_draw_text(app.font, al_map_rgb(255, 255, 255), CENTER_X, 390, ALLEGRO_ALIGN_CENTER, "P - Reanudar");
        al_draw_text(app.font, al_map_rgb(255, 255, 255), CENTER_X, 430, ALLEGRO_ALIGN_CENTER, "ESC - Salir");
    }

    void dibujarFinOleada(AppState& app)
    {
        dibujarPantallaJuego(app);
        al_draw_filled_rectangle(220, 220, 804, 540, al_map_rgba(0, 0, 0, 180));
        al_draw_text(app.titleFont, al_map_rgb(255, 255, 255), CENTER_X, 270, ALLEGRO_ALIGN_CENTER, "FIN DE OLEADA");
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), CENTER_X, 340, ALLEGRO_ALIGN_CENTER, "Puntaje: %d", app.jugador.puntaje);
        al_draw_text(app.font, al_map_rgb(255, 255, 255), CENTER_X, 390, ALLEGRO_ALIGN_CENTER, "ENTER - Continuar");
    }

    void dibujarFinPartida(AppState& app)
    {
        al_clear_to_color(al_map_rgb(10, 8, 20));
        al_draw_text(app.titleFont, al_map_rgb(255, 255, 255), CENTER_X, 180, ALLEGRO_ALIGN_CENTER, "FIN DE PARTIDA");
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), CENTER_X, 260, ALLEGRO_ALIGN_CENTER, "Jugador: %s", app.jugador.alias.c_str());
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), CENTER_X, 300, ALLEGRO_ALIGN_CENTER, "Puntaje final: %d", app.jugador.puntaje);
        al_draw_text(app.font, al_map_rgb(255, 255, 255), CENTER_X, 360, ALLEGRO_ALIGN_CENTER, "ENTER - Volver al menu");
    }

    void dibujarEstadisticas(AppState& app)
    {
        al_clear_to_color(al_map_rgb(7, 12, 20));
        al_draw_text(app.titleFont, al_map_rgb(255, 255, 255), CENTER_X, 80, ALLEGRO_ALIGN_CENTER, "ESTADISTICAS");
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 220, 180, 0, "Puntaje: %d", app.jugador.puntaje);
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 220, 220, 0, "Combo: %d", app.jugador.combo);
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 220, 260, 0, "Carga de pulso: %d%%", app.jugador.cargaPulso);
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 220, 300, 0, "Vida del nucleo: %d", app.jugador.vidaNucleo);
        al_draw_text(app.font, al_map_rgb(255, 255, 255), CENTER_X, 420, ALLEGRO_ALIGN_CENTER, "ENTER - Volver");
    }

    void dibujarRanking(AppState& app)
    {
        al_clear_to_color(al_map_rgb(10, 10, 15));
        al_draw_text(app.titleFont, al_map_rgb(255, 255, 255), CENTER_X, 80, ALLEGRO_ALIGN_CENTER, "RANKING");
        al_draw_textf(app.font, al_map_rgb(255, 255, 255), 220, 180, 0, "1. %s - %d", app.jugador.alias.c_str(), app.jugador.puntaje);
        al_draw_text(app.font, al_map_rgb(255, 255, 255), CENTER_X, 420, ALLEGRO_ALIGN_CENTER, "ENTER - Volver");
    }

    void dibujarPantalla(AppState& app)
    {
        switch (app.screen)
        {
            case ScreenState::MainMenu:
                dibujarMenu(app);
                break;
            case ScreenState::Register:
                dibujarRegistro(app);
                break;
            case ScreenState::Gameplay:
                dibujarPantallaJuego(app);
                break;
            case ScreenState::Pause:
                dibujarPausa(app);
                break;
            case ScreenState::EndWave:
                dibujarFinOleada(app);
                break;
            case ScreenState::EndMatch:
                dibujarFinPartida(app);
                break;
            case ScreenState::Statistics:
                dibujarEstadisticas(app);
                break;
            case ScreenState::Ranking:
                dibujarRanking(app);
                break;
        }

        al_flip_display();
    }

    void actualizarJuego(AppState& app)
    {
        if (app.screen == ScreenState::Gameplay && !app.paused)
        {
            procesarTickMotor(app.motor, app.pendingColor);
            app.pendingColor = -1;

            if (app.jugador.vidaNucleo <= 0)
            {
                app.screen = ScreenState::EndMatch;
            }
            else if (colaVacia(app.motor.colaSpawnOleada) && app.motor.listaEnemigos.vacia())
            {
                app.screen = ScreenState::EndWave;
            }
        }
    }
}

int main()
{
    AppState app;

    try
    {
        inicializarAplicacion(app);
        al_start_timer(app.timer);

        while (app.running)
        {
            ALLEGRO_EVENT ev;
            al_wait_for_event(app.queue, &ev);

            if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            {
                app.running = false;
            }
            else if (ev.type == ALLEGRO_EVENT_TIMER)
            {
                if (app.screen == ScreenState::Gameplay && !app.paused)
                {
                    actualizarRotacionDesdeCursor(app);
                    actualizarJuego(app);
                }
                dibujarPantalla(app);
            }
            else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES)
            {
                app.mouseX = ev.mouse.x;
                app.mouseY = ev.mouse.y;
                if (app.screen == ScreenState::Gameplay)
                    actualizarRotacionDesdeCursor(app);
            }
            else if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
            {
                const int key = ev.keyboard.keycode;

                if (key == ALLEGRO_KEY_ESCAPE)
                {
                    if (app.screen == ScreenState::Gameplay)
                    {
                        app.screen = ScreenState::Pause;
                        app.paused = true;
                    }
                    else
                    {
                        app.running = false;
                    }
                }
                else if (key == ALLEGRO_KEY_P)
                {
                    if (app.screen == ScreenState::Gameplay)
                    {
                        app.screen = ScreenState::Pause;
                        app.paused = true;
                    }
                    else if (app.screen == ScreenState::Pause)
                    {
                        app.screen = ScreenState::Gameplay;
                        app.paused = false;
                    }
                }
                else if (key == ALLEGRO_KEY_ENTER)
                {
                    if (app.screen == ScreenState::MainMenu)
                    {
                        app.screen = ScreenState::Register;
                    }
                    else if (app.screen == ScreenState::Register)
                    {
                        app.screen = ScreenState::Gameplay;
                        app.paused = false;
                    }
                    else if (app.screen == ScreenState::EndWave)
                    {
                        app.screen = ScreenState::Gameplay;
                        app.paused = false;
                        prepararOleadaPrueba(app.oleada);
                        iniciarTurnoMotor(app.motor, &app.jugador, app.oleada);
                    }
                    else if (app.screen == ScreenState::EndMatch)
                    {
                        app.screen = ScreenState::MainMenu;
                    }
                    else if (app.screen == ScreenState::Statistics || app.screen == ScreenState::Ranking)
                    {
                        app.screen = ScreenState::MainMenu;
                    }
                }
                else if (key == ALLEGRO_KEY_1)
                {
                    app.pendingColor = 0;
                    if (app.screen == ScreenState::Gameplay)
                        procesarTickMotor(app.motor, 0);
                }
                else if (key == ALLEGRO_KEY_2)
                {
                    app.pendingColor = 1;
                    if (app.screen == ScreenState::Gameplay)
                        procesarTickMotor(app.motor, 1);
                }
                else if (key == ALLEGRO_KEY_3)
                {
                    app.pendingColor = 2;
                    if (app.screen == ScreenState::Gameplay)
                        procesarTickMotor(app.motor, 2);
                }
                else if (key == ALLEGRO_KEY_LEFT)
                {
                    if (app.screen == ScreenState::Gameplay)
                        app.motor.anguloNave = app.motor.anguloNave - 16.0;
                }
                else if (key == ALLEGRO_KEY_RIGHT)
                {
                    if (app.screen == ScreenState::Gameplay)
                        app.motor.anguloNave = app.motor.anguloNave + 16.0;
                }
                else if (key == ALLEGRO_KEY_SPACE)
                {
                    if (app.screen == ScreenState::Gameplay && app.motor.jugadorActual != nullptr)
                        app.motor.jugadorActual->cargaPulso = 100;
                }
                else if (key == ALLEGRO_KEY_S)
                {
                    app.screen = ScreenState::Statistics;
                }
                else if (key == ALLEGRO_KEY_R)
                {
                    app.screen = ScreenState::Ranking;
                }
            }
        }
    }
    catch (const exception& e)
    {
        cerr << "Error de Allegro: " << e.what() << endl;
        app.running = false;
    }

    destruirAplicacion(app);
    return 0;
}
