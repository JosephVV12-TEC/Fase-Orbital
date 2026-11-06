#include "Partida.h"
#include "Oleada.h"
#include <cctype>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <cstdlib>

using namespace std;

bool esAliasBot(const string& alias)
{
    return alias.size() >= 3 &&
           toupper(static_cast<unsigned char>(alias[0])) == 'B' &&
           toupper(static_cast<unsigned char>(alias[1])) == 'O' &&
           toupper(static_cast<unsigned char>(alias[2])) == 'T';
}

void inicializarPartida(Partida& p)
{
    p.cantidad = 0;
    inicializarRonda(p.ronda);
    vaciarBitacora(p.bitacora);
    p.semilla = 42;
    p.oleada = 0;
    p.limiteOleadas = 10;
    p.horario = true;
    p.turnosPendientes = 0;
    p.enCurso = false;
    p.ganador = -1;
    for (int i = 0; i < MAX_JUGADORES; ++i)
        p.oleadaAlcanzada[i] = 0;
}

void destruirPartida(Partida& p)
{
    vaciarRonda(p.ronda);   // libera los nodos, NO los jugadores (viven en el arreglo)
    vaciarBitacora(p.bitacora);
    p.cantidad = 0;
    p.enCurso = false;
}

bool registrarJugador(Partida& p, const string& alias)
{
    if (p.cantidad >= MAX_JUGADORES)
        return false;

    inicializarJugador(p.jugadores[p.cantidad], alias, esAliasBot(alias));
    p.oleadaAlcanzada[p.cantidad] = 0;
    agregarRonda(p.ronda, &p.jugadores[p.cantidad]); // queda al final de la ronda
    p.cantidad++;
    return true;
}

int indiceDeJugador(const Partida& p, const Jugador* j)
{
    for (int i = 0; i < p.cantidad; ++i)
        if (&p.jugadores[i] == j)
            return i;
    return -1;
}

void comenzarPartida(Partida& p, Motor& m, unsigned int semilla, int limiteOleadas)
{
    p.semilla = semilla;
    p.limiteOleadas = limiteOleadas;
    p.oleada = 1;
    p.horario = true;
    p.turnosPendientes = p.ronda.tam;
    p.enCurso = true;
    p.ganador = -1;

    generarOleada(p.oleada, p.semilla, p.colaOleada); // la cola de la oleada se genera UNA vez
    iniciarTurnoActual(p, m);
}

void iniciarTurnoActual(Partida& p, Motor& m)
{
    Jugador* j = verActualRonda(p.ronda);
    int i = indiceDeJugador(p, j);
    if (i >= 0)
        p.oleadaAlcanzada[i] = p.oleada;

    // El Motor hace una copia profunda de la cola, la original queda intacta
    iniciarTurnoMotor(m, j, p.colaOleada, p.oleada, i, &p.bitacora);
}

// Orden por puntaje; empate: mayor oleada alcanzada.TODO (Estadisticas): como tercer criterio usar la mayor precision de color.
void calcularPosiciones(const Partida& p, int orden[MAX_JUGADORES])
{
    for (int i = 0; i < p.cantidad; ++i)
        orden[i] = i;

    // Insercion sobre un arreglo de 6 enteros (no se usa std::sort)
    for (int i = 1; i < p.cantidad; ++i)
    {
        int actual = orden[i];
        int k = i - 1;
        while (k >= 0)
        {
            const Jugador& a = p.jugadores[orden[k]];
            const Jugador& b = p.jugadores[actual];
            const int oleadaB = p.oleadaAlcanzada[actual];
            const int oleadaA = p.oleadaAlcanzada[orden[k]];
            bool bMejor = (b.puntaje > a.puntaje) ||
                (b.puntaje == a.puntaje && oleadaB > oleadaA) ||
                (b.puntaje == a.puntaje && oleadaB == oleadaA && precisionColor(b) > precisionColor(a));
            if (!bMejor)
                break;
            orden[k + 1] = orden[k];
            --k;
        }
        orden[k + 1] = actual;
    }
}

ResultadoTurno terminarTurno(Partida& p, bool oleadaCompletada)
{
    Jugador* j = verActualRonda(p.ronda);

    // Reparacion del nucleo al completar la oleada: +8 (maximo 100)
    if (j != NULL && oleadaCompletada && j->vidaNucleo > 0)
    {
        j->vidaNucleo += 8;
        if (j->vidaNucleo > 100)
            j->vidaNucleo = 100;
    }

    p.turnosPendientes--;
    const bool ultimoDeLaRonda = (p.turnosPendientes <= 0);
    const bool murio = (j != NULL && j->vidaNucleo <= 0);

    // Orden serpenteante: la ronda siguiente va en sentido contrario y empieza con
    // el ultimo que jugo. Si el ultimo en jugar es el eliminado, el puntero debe
    // quedar en su vecino segun el sentido de la ronda siguiente.
    const bool sentidoDelPuntero = ultimoDeLaRonda ? !p.horario : p.horario;

    if (murio)
        eliminarActualRonda(p.ronda, sentidoDelPuntero);
    else if (!ultimoDeLaRonda)
        avanzarRonda(p.ronda, p.horario);

    if (!ultimoDeLaRonda)
        return ResultadoTurno::SigueRonda;

    // --- La ronda se completo ---
    const bool quedaUnoSolo = (p.ronda.tam == 1 && p.cantidad > 1);
    const bool sinJugadores = (p.ronda.tam == 0);
    const bool limite = (p.oleada >= p.limiteOleadas);

    if (sinJugadores || quedaUnoSolo || limite)
    {
        p.enCurso = false;
        int orden[MAX_JUGADORES];
        calcularPosiciones(p, orden);
        p.ganador = (p.cantidad > 0) ? orden[0] : -1;
        return ResultadoTurno::PartidaTerminada;
    }

    p.oleada++;
    p.horario = !p.horario;
    p.turnosPendientes = p.ronda.tam;
    generarOleada(p.oleada, p.semilla, p.colaOleada);
    return ResultadoTurno::NuevaRonda;
}

namespace
{
    const char* NOMBRES_TIPO[6] = { "Dron", "Girador", "Divisor", "Mini", "Tanque", "Camaleon" };
    const char* NOMBRES_COLOR[3] = { "C", "M", "A" };

    double pct(double fraccion)
    {
        return fraccion * 100.0;
    }
}

//El reporte de la partida se imprime en un flujo de salida
void imprimirReportePartida(const Partida& p, ostream& os)
{
    const ios::fmtflags flagsGuardados = os.flags();
    const streamsize precisionGuardada = os.precision();

    int orden[MAX_JUGADORES];
    calcularPosiciones(p, orden);

    os << fixed << setprecision(1);
    os << "=== REPORTE DE LA PARTIDA ===\n";
    os << "Semilla: " << p.semilla << "   Oleada final: " << p.oleada << "   Limite: " << p.limiteOleadas << "\n";
    if (p.ganador >= 0)
        os << "Ganador: " << p.jugadores[p.ganador].alias << "\n";

    // ---- Posiciones ----
    os << "\nPOSICIONES\n";
    os << left << setw(5) << "Pos" << setw(14) << "Jugador" << setw(8) << "Tipo"
        << right << setw(9) << "Puntaje" << setw(8) << "Oleada" << setw(8) << "Nucleo" << "\n";
    for (int k = 0; k < p.cantidad; ++k)
    {
        const int i = orden[k];
        const Jugador& j = p.jugadores[i];
        os << left << setw(5) << (k + 1) << setw(14) << j.alias << setw(8) << (j.esBot ? "BOT" : "HUMANO")
            << right << setw(9) << j.puntaje << setw(8) << p.oleadaAlcanzada[i] << setw(8) << j.vidaNucleo << "\n";
    }

    // ---- Metricas por jugador (6.1) ----
    os << "\nMETRICAS POR JUGADOR\n";
    for (int k = 0; k < p.cantidad; ++k)
    {
        const int i = orden[k];
        const Jugador& j = p.jugadores[i];
        const EstadisticasJugador& e = j.est;

        os << (k + 1) << ". " << j.alias << (j.esBot ? " (BOT)" : "") << "\n";
        os << "  Puntaje: " << j.puntaje << "   Oleada alcanzada: " << p.oleadaAlcanzada[i] << "\n";
        os << "  Disparos: " << e.disparos << "   Impactos correctos: " << e.correctos
            << "   Impactos erroneos: " << e.erroneos << "\n";
        os << "  Precision de color: " << pct(precisionColor(j)) << "%   Eficiencia de disparo: "
            << pct(eficienciaDisparo(j)) << "%\n";
        os << "  Precision por color:";
        for (int c = 0; c < 3; ++c)
            os << " " << NOMBRES_COLOR[c] << " " << pct(precisionDeFase(j, c)) << "%" << (c < 2 ? " |" : "");
        os << "\n";
        os << "  Bajas por tipo:";
        for (int t = 0; t < 6; ++t)
            os << " " << NOMBRES_TIPO[t] << " " << e.bajasTipo[t] << (t < 5 ? " |" : "");
        os << "\n";
        os << "  Bajas por color:";
        for (int c = 0; c < 3; ++c)
            os << " " << NOMBRES_COLOR[c] << " " << e.bajasColor[c] << (c < 2 ? " |" : "");
        os << "\n";
        os << "  Combo maximo: " << e.comboMax << "   Pulsos usados: " << e.pulsos
            << "   Ticks sobrevividos: " << e.ticks << "\n";
        os << "  Dano recibido: " << e.danoTotal << " (";
        for (int t = 0; t < 6; ++t)
            os << NOMBRES_TIPO[t] << " " << e.danoTipo[t] << (t < 5 ? " | " : "");
        os << ")\n";
    }

    // ---- Linea de tiempo por oleada, calculada recorriendo la bitacora (6.2) ----
    os << "\nLINEA DE TIEMPO POR OLEADA\n";
    for (int k = 0; k < p.cantidad; ++k)
    {
        const int i = orden[k];
        os << p.jugadores[i].alias << "\n";
        os << "  " << left << setw(8) << "Oleada" << right << setw(8) << "Bajas" << setw(8) << "Dano"
            << setw(12) << "Puntaje" << "  Estado\n";

        int acumulado = 0;
        for (int o = 1; o <= p.oleadaAlcanzada[i]; ++o)
        {
            const int bajas = contarEventos(p.bitacora, i, o, TipoEvento::BAJA);
            const int dano = sumarValorEventos(p.bitacora, i, o, TipoEvento::DANO_NUCLEO)
                + sumarValorEventos(p.bitacora, i, o, TipoEvento::CHOQUE_NAVE);
            acumulado += sumarValorEventos(p.bitacora, i, o, TipoEvento::BAJA);

            const char* estado = "";
            if (contarEventos(p.bitacora, i, o, TipoEvento::ELIMINACION) > 0)
                estado = "eliminado";
            else if (contarEventos(p.bitacora, i, o, TipoEvento::FIN_OLEADA) > 0)
                estado = "completada";

            os << "  " << left << setw(8) << o << right << setw(8) << bajas << setw(8) << dano
                << setw(12) << acumulado << "  " << estado << "\n";
        }
    }

    os.flags(flagsGuardados);
    os.precision(precisionGuardada);
}


// ---------------------------------------------------------------------------
// Historial de partidas (CSV)
// ---------------------------------------------------------------------------
namespace
{
    // Nombres de las 33 columnas, en orden. Las primeras 21 son las del PDF.
    const char* NOMBRES_COLUMNAS[NUM_COLUMNAS] =
    {
        "id_partida", "semilla", "modo", "jugador", "es_bot", "posicion", "puntaje", "oleada",
        "disparos", "correctos", "erroneos", "combo_max", "pulsos", "dano_total", "ticks",
        "bajas_dron", "bajas_girador", "bajas_divisor", "bajas_mini", "bajas_tanque", "bajas_camaleon",
        "impactos_c", "impactos_m", "impactos_a", "correctos_c", "correctos_m", "correctos_a",
        "dano_dron", "dano_girador", "dano_divisor", "dano_mini", "dano_tanque", "dano_camaleon"
    };

    // Parte la linea en campos separados por comas. Devuelve cuantos campos tiene la linea.
    int dividirLinea(const string& linea, string campos[], int max)
    {
        int cantidad = 0;
        string actual;
        for (size_t i = 0; i <= linea.size(); ++i)
        {
            if (i == linea.size() || linea[i] == ',')
            {
                if (cantidad < max)
                    campos[cantidad] = actual;
                cantidad += 1;   // sigue contando aunque no quepa, para detectar lineas con columnas de mas
                actual.clear();
            }
            else
            {
                actual += linea[i];
            }
        }
        return cantidad;
    }

    // true si el texto es un numero entero entre 0 y 2147483647
    bool aEntero(const string& texto, long& valor)
    {
        if (texto.empty())
            return false;
        char* fin = nullptr;
        valor = strtol(texto.c_str(), &fin, 10);
        return *fin == '\0' && valor >= 0 && valor <= 2147483647L;
    }

    // Convierte los campos de una linea en una fila. Si algo esta mal devuelve false y explica por que.
    bool leerFila(const string campos[], int cantidad, FilaHistorial& f, string& motivo)
    {
        if (cantidad != 21 && cantidad != NUM_COLUMNAS)
        {
            motivo = "se esperaban 21 o 33 columnas y hay " + to_string(cantidad);
            return false;
        }

        for (int i = 0; i < NUM_COLUMNAS; ++i)
            f.num[i] = 0;
        f.modo = campos[2];
        f.jugador = campos[3];
        if (f.jugador.empty())
        {
            motivo = "la columna jugador esta vacia";
            return false;
        }

        for (int i = 0; i < cantidad; ++i)
        {
            if (i == 2 || i == 3)
                continue;   // columnas de texto
            long valor = 0;
            if (!aEntero(campos[i], valor))
            {
                motivo = string("valor invalido en la columna ") + NOMBRES_COLUMNAS[i];
                return false;
            }
            f.num[i] = static_cast<int>(valor);
        }

        if (f.num[COL_ES_BOT] > 1)
        {
            motivo = "es_bot debe ser 0 o 1";
            return false;
        }
        if (f.num[COL_POSICION] < 1)
        {
            motivo = "posicion debe ser 1 o mayor";
            return false;
        }
        return true;
    }

    // Escribe una fila en el CSV (columnas separadas por comas)
    void escribirFila(ostream& out, const FilaHistorial& f)
    {
        for (int i = 0; i < NUM_COLUMNAS; ++i)
        {
            if (i > 0)
                out << ",";
            if (i == 2)
                out << f.modo;
            else if (i == 3)
                out << f.jugador;
            else
                out << f.num[i];
        }
        out << "\n";
    }

    int siguienteIdPartida(const HistorialPartidas& h)
    {
        int mayor = 0;
        for (NodoFila* n = h.cabeza; n != nullptr; n = n->sig)
            if (n->dato.num[COL_ID] > mayor)
                mayor = n->dato.num[COL_ID];
        return mayor + 1;
    }

    // Arma la fila del jugador i de la partida (posicion 1 = ganador)
    FilaHistorial crearFila(const Partida& p, int i, int posicion, int id, const string& modo)
    {
        const Jugador& j = p.jugadores[i];
        const EstadisticasJugador& e = j.est;

        FilaHistorial f;
        for (int k = 0; k < NUM_COLUMNAS; ++k)
            f.num[k] = 0;
        f.modo = modo;
        f.jugador = j.alias;
        f.num[COL_ID] = id;
        f.num[COL_SEMILLA] = static_cast<int>(p.semilla);
        f.num[COL_ES_BOT] = j.esBot ? 1 : 0;
        f.num[COL_POSICION] = posicion;
        f.num[COL_PUNTAJE] = j.puntaje;
        f.num[COL_OLEADA] = p.oleadaAlcanzada[i];
        f.num[COL_DISPAROS] = e.disparos;
        f.num[COL_CORRECTOS] = e.correctos;
        f.num[COL_ERRONEOS] = e.erroneos;
        f.num[COL_COMBO_MAX] = e.comboMax;
        f.num[COL_PULSOS] = e.pulsos;
        f.num[COL_DANO_TOTAL] = e.danoTotal;
        f.num[COL_TICKS] = e.ticks;
        for (int t = 0; t < 6; ++t)
        {
            f.num[COL_BAJAS + t] = e.bajasTipo[t];
            f.num[COL_DANO_TIPO + t] = e.danoTipo[t];
        }
        for (int c = 0; c < 3; ++c)
        {
            f.num[COL_IMPACTOS_FASE + c] = e.impactosFase[c];
            f.num[COL_CORRECTOS_FASE + c] = e.correctosFase[c];
        }
        return f;
    }
}

HistorialPartidas::HistorialPartidas() : cabeza(nullptr), cola(nullptr), tam(0)
{
}

HistorialPartidas::~HistorialPartidas()
{
    vaciarHistorial(*this);
}

void agregarFila(HistorialPartidas& h, const FilaHistorial& f)
{
    NodoFila* nuevo = new NodoFila;
    nuevo->dato = f;
    nuevo->sig = nullptr;

    if (h.cola == nullptr)
        h.cabeza = nuevo;
    else
        h.cola->sig = nuevo;

    h.cola = nuevo;
    h.tam += 1;
}

void vaciarHistorial(HistorialPartidas& h)
{
    while (h.cabeza != nullptr)
    {
        NodoFila* aux = h.cabeza;
        h.cabeza = h.cabeza->sig;
        delete aux;
    }
    h.cola = nullptr;
    h.tam = 0;
}

int cargarHistorial(HistorialPartidas& h, const string& archivo, ostream& errores)
{
    ifstream entrada(archivo.c_str());
    if (!entrada)
        return 0;   // todavia no existe: historial vacio

    int invalidas = 0;
    int numeroLinea = 0;
    string linea;
    string campos[NUM_COLUMNAS + 1];

    while (getline(entrada, linea))
    {
        numeroLinea += 1;
        if (!linea.empty() && linea[linea.size() - 1] == '\r')
            linea.erase(linea.size() - 1);
        if (linea.empty())
            continue;
        if (numeroLinea == 1 && linea.compare(0, 10, "id_partida") == 0)
            continue;   // encabezado

        const int cantidad = dividirLinea(linea, campos, NUM_COLUMNAS + 1);
        FilaHistorial fila;
        string motivo;
        if (leerFila(campos, cantidad, fila, motivo))
        {
            agregarFila(h, fila);
        }
        else
        {
            errores << archivo << ", linea " << numeroLinea << ": " << motivo << "\n";
            invalidas += 1;
        }
    }
    return invalidas;
}

bool guardarPartidaEnHistorial(HistorialPartidas& h, const Partida& p, const string& modo, const string& archivo)
{
    // Si el archivo no existe o esta vacio, primero va el encabezado
    bool escribirEncabezado = true;
    {
        ifstream existente(archivo.c_str(), ios::binary | ios::ate);
        if (existente && existente.tellg() > streampos(0))
            escribirEncabezado = false;
    }

    ofstream salida(archivo.c_str(), ios::app);
    const bool archivoOk = static_cast<bool>(salida);
    if (archivoOk && escribirEncabezado)
    {
        for (int i = 0; i < NUM_COLUMNAS; ++i)
            salida << (i > 0 ? "," : "") << NOMBRES_COLUMNAS[i];
        salida << "\n";
    }

    int orden[MAX_JUGADORES];
    calcularPosiciones(p, orden);
    const int id = siguienteIdPartida(h);

    for (int k = 0; k < p.cantidad; ++k)
    {
        const FilaHistorial fila = crearFila(p, orden[k], k + 1, id, modo);
        agregarFila(h, fila);
        if (archivoOk)
            escribirFila(salida, fila);
    }
    return archivoOk;
}

// ---------------------------------------------------------------------------
// Merge sort sobre lista enlazada
// ---------------------------------------------------------------------------
namespace
{
    // Mezcla dos listas ya ordenadas en una sola (usa un nodo auxiliar 'base' en la pila)
    NodoNum* mezclar(NodoNum* a, NodoNum* b)
    {
        NodoNum base;
        base.sig = nullptr;
        NodoNum* ultimo = &base;

        while (a != nullptr && b != nullptr)
        {
            if (a->valor <= b->valor)
            {
                ultimo->sig = a;
                a = a->sig;
            }
            else
            {
                ultimo->sig = b;
                b = b->sig;
            }
            ultimo = ultimo->sig;
        }
        ultimo->sig = (a != nullptr) ? a : b;
        return base.sig;
    }
}

NodoNum* mergeSortLista(NodoNum* cabeza)
{
    if (cabeza == nullptr || cabeza->sig == nullptr)
        return cabeza;   // vacia o de un solo nodo: ya esta ordenada

    // Se busca la mitad: 'rapido' avanza de 2 en 2 y 'lento' de 1 en 1
    NodoNum* lento = cabeza;
    NodoNum* rapido = cabeza->sig;
    while (rapido != nullptr && rapido->sig != nullptr)
    {
        lento = lento->sig;
        rapido = rapido->sig->sig;
    }

    NodoNum* segundaMitad = lento->sig;
    lento->sig = nullptr;   // se corta la lista en dos

    return mezclar(mergeSortLista(cabeza), mergeSortLista(segundaMitad));
}

void liberarNumeros(NodoNum* cabeza)
{
    while (cabeza != nullptr)
    {
        NodoNum* aux = cabeza;
        cabeza = cabeza->sig;
        delete aux;
    }
}

// ---------------------------------------------------------------------------
// Estadisticas historicas por jugador
// ---------------------------------------------------------------------------
void resumenDeJugador(const HistorialPartidas& h, const string& alias, ResumenJugador& r)
{
    r.alias = alias;
    r.partidas = 0;
    r.victorias = 0;
    r.mejor = 0;
    r.peor = 0;
    r.media = 0.0;
    r.mediana = 0.0;
    r.desviacion = 0.0;
    r.precisionProm = 0.0;
    r.mejorColor = -1;
    r.tipoMasDano = -1;
    for (int i = 0; i < 10; ++i)
        r.histograma[i] = 0;

    NodoNum* puntajes = nullptr;   // lista auxiliar con los puntajes del jugador
    double suma = 0.0;
    double sumaPrecision = 0.0;
    int partidasConImpactos = 0;
    int impactos[3] = { 0, 0, 0 };
    int correctos[3] = { 0, 0, 0 };
    int dano[6] = { 0, 0, 0, 0, 0, 0 };

    for (NodoFila* n = h.cabeza; n != nullptr; n = n->sig)
    {
        const FilaHistorial& f = n->dato;
        if (f.jugador != alias)
            continue;

        const int pts = f.num[COL_PUNTAJE];
        if (r.partidas == 0 || pts > r.mejor)
            r.mejor = pts;
        if (r.partidas == 0 || pts < r.peor)
            r.peor = pts;
        r.partidas += 1;
        if (f.num[COL_POSICION] == 1)
            r.victorias += 1;
        suma += pts;

        NodoNum* nuevo = new NodoNum;
        nuevo->valor = pts;
        nuevo->sig = puntajes;
        puntajes = nuevo;

        const int total = f.num[COL_CORRECTOS] + f.num[COL_ERRONEOS];
        if (total > 0)
        {
            sumaPrecision += static_cast<double>(f.num[COL_CORRECTOS]) / total;
            partidasConImpactos += 1;
        }
        for (int c = 0; c < 3; ++c)
        {
            impactos[c] += f.num[COL_IMPACTOS_FASE + c];
            correctos[c] += f.num[COL_CORRECTOS_FASE + c];
        }
        for (int t = 0; t < 6; ++t)
            dano[t] += f.num[COL_DANO_TIPO + t];
    }

    if (r.partidas == 0)
        return;

    // Media y desviacion estandar poblacional
    r.media = suma / r.partidas;
    double sumaCuadrados = 0.0;
    for (NodoNum* x = puntajes; x != nullptr; x = x->sig)
        sumaCuadrados += (x->valor - r.media) * (x->valor - r.media);
    r.desviacion = sqrt(sumaCuadrados / r.partidas);

    // Mediana: se ordena la lista auxiliar con merge sort
    puntajes = mergeSortLista(puntajes);
    const int mitad = r.partidas / 2;
    NodoNum* centro = puntajes;
    for (int i = 0; i < mitad; ++i)
        centro = centro->sig;
    if (r.partidas % 2 == 1)
    {
        r.mediana = centro->valor;
    }
    else
    {
        NodoNum* anterior = puntajes;
        for (int i = 0; i < mitad - 1; ++i)
            anterior = anterior->sig;
        r.mediana = (anterior->valor + centro->valor) / 2.0;
    }

    // Histograma: 10 intervalos iguales entre el minimo y el maximo
    for (NodoNum* x = puntajes; x != nullptr; x = x->sig)
    {
        int intervalo = 0;
        if (r.mejor > r.peor)
            intervalo = static_cast<int>((x->valor - r.peor) * 10.0 / (r.mejor - r.peor));
        if (intervalo > 9)
            intervalo = 9;   // el maximo cae en el ultimo intervalo
        r.histograma[intervalo] += 1;
    }
    liberarNumeros(puntajes);

    if (partidasConImpactos > 0)
        r.precisionProm = sumaPrecision / partidasConImpactos;

    // Color con mejor precision (correctos / impactos con esa fase)
    double mejorPrecision = -1.0;
    for (int c = 0; c < 3; ++c)
    {
        if (impactos[c] == 0)
            continue;
        const double precision = static_cast<double>(correctos[c]) / impactos[c];
        if (precision > mejorPrecision)
        {
            mejorPrecision = precision;
            r.mejorColor = c;
        }
    }

    // Tipo de enemigo que mas dano le ha causado
    int mayorDano = 0;
    for (int t = 0; t < 6; ++t)
    {
        if (dano[t] > mayorDano)
        {
            mayorDano = dano[t];
            r.tipoMasDano = t;
        }
    }
}

int calcularResumenes(const HistorialPartidas& h, ResumenJugador salida[], int max)
{
    int total = 0;
    for (NodoFila* n = h.cabeza; n != nullptr && total < max; n = n->sig)
    {
        bool yaEsta = false;
        for (int i = 0; i < total; ++i)
            if (salida[i].alias == n->dato.jugador)
                yaEsta = true;

        if (!yaEsta)
        {
            resumenDeJugador(h, n->dato.jugador, salida[total]);
            total += 1;
        }
    }
    return total;
}

void imprimirEstadisticas(const HistorialPartidas& h, ostream& os)
{
    const ios::fmtflags flagsGuardados = os.flags();
    const streamsize precisionGuardada = os.precision();

    ResumenJugador resumenes[MAX_ALIAS_HIST];
    const int cantidad = calcularResumenes(h, resumenes, MAX_ALIAS_HIST);

    os << fixed << setprecision(1);
    os << "=== ESTADISTICAS HISTORICAS (" << h.tam << " filas) ===\n";
    if (cantidad == 0)
        os << "No hay partidas en el historial.\n";

    for (int k = 0; k < cantidad; ++k)
    {
        const ResumenJugador& r = resumenes[k];
        os << "\n" << r.alias << "\n";
        os << "  Partidas: " << r.partidas << "   Victorias: " << r.victorias << "\n";
        os << "  Puntaje mejor: " << r.mejor << "   peor: " << r.peor << "\n";
        os << "  Media: " << r.media << "   Mediana: " << r.mediana << "   Desviacion estandar: " << r.desviacion << "\n";
        os << "  Precision de color promedio: " << pct(r.precisionProm) << "%\n";
        os << "  Color con mejor precision: ";
        if (r.mejorColor >= 0) os << NOMBRES_COLOR[r.mejorColor]; else os << "sin datos";
        os << "\n  Enemigo que mas dano le ha causado: ";
        if (r.tipoMasDano >= 0) os << NOMBRES_TIPO[r.tipoMasDano]; else os << "sin datos";
        os << "\n  Histograma de puntajes:\n";

        for (int i = 0; i < 10; ++i)
        {
            const double desde = r.peor + (r.mejor - r.peor) * i / 10.0;
            const double hasta = r.peor + (r.mejor - r.peor) * (i + 1) / 10.0;
            os << "    " << right << setw(8) << static_cast<int>(desde) << " - " << left << setw(8) << static_cast<int>(hasta)
                << " | " << string(r.histograma[i], '#') << " " << r.histograma[i] << "\n";
        }
    }

    os.flags(flagsGuardados);
    os.precision(precisionGuardada);
}

//Para el ranking
Ranking::Ranking() : cabeza(nullptr), tam(0)
{
}

Ranking::~Ranking()
{
    while (cabeza != nullptr)
    {
        NodoRanking* aux = cabeza;
        cabeza = cabeza->sig;
        delete aux;
    }
}

namespace
{
    // true si 'a' va antes que 'b': mayor puntaje; si empatan, mayor precision; luego alias alfabetico
    bool iraAntes(const EntradaRanking& a, const EntradaRanking& b)
    {
        if (a.puntaje != b.puntaje)
            return a.puntaje > b.puntaje;
        if (a.precision != b.precision)
            return a.precision > b.precision;
        return a.alias < b.alias;
    }
}

void insertarOrdenado(Ranking& r, const EntradaRanking& e)
{
    NodoRanking* nuevo = new NodoRanking;
    nuevo->dato = e;

    // 'actual' apunta al enlace (cabeza o sig) donde va el nodo nuevo: no hace falta caso especial para la cabeza
    NodoRanking** actual = &r.cabeza;
    while (*actual != nullptr && !iraAntes(e, (*actual)->dato))
        actual = &(*actual)->sig;

    nuevo->sig = *actual;
    *actual = nuevo;
    r.tam += 1;
}

void construirRanking(const ResumenJugador res[], int cantidad, Ranking& r)
{
    // Se vacia el ranking anterior
    while (r.cabeza != nullptr)
    {
        NodoRanking* aux = r.cabeza;
        r.cabeza = r.cabeza->sig;
        delete aux;
    }
    r.tam = 0;

    for (int i = 0; i < cantidad; ++i)
    {
        EntradaRanking e;
        e.alias = res[i].alias;
        e.puntaje = res[i].mejor;
        e.precision = res[i].precisionProm;
        insertarOrdenado(r, e);
    }
}

void imprimirTop(const Ranking& r, int k, ostream& os)
{
    const ios::fmtflags flagsGuardados = os.flags();
    const streamsize precisionGuardada = os.precision();

    os << fixed << setprecision(1);
    os << left << setw(5) << "Pos" << setw(16) << "Jugador" << right << setw(10) << "Puntaje" << setw(12) << "Precision" << "\n";
    int posicion = 1;
    for (NodoRanking* n = r.cabeza; n != nullptr && posicion <= k; n = n->sig)
    {
        os << left << setw(5) << posicion << setw(16) << n->dato.alias << right << setw(10) << n->dato.puntaje
            << setw(11) << pct(n->dato.precision) << "%\n";
        posicion += 1;
    }

    os.flags(flagsGuardados);
    os.precision(precisionGuardada);
}

void imprimirRanking(const HistorialPartidas& h, ostream& os)
{
    ResumenJugador resumenes[MAX_ALIAS_HIST];
    const int cantidad = calcularResumenes(h, resumenes, MAX_ALIAS_HIST);

    Ranking ranking;
    construirRanking(resumenes, cantidad, ranking);

    os << "=== RANKING TOP 10 ===\n";
    imprimirTop(ranking, 10, os);
}