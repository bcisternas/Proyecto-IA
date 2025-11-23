#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <random>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <sys/stat.h>

using namespace std;

// Constante de penalización para individuos inválidos
const double FITNESS_INVALIDO = 1e18;

// Generador de números aleatorios Mersenne Twister
random_device rd;
mt19937 rng(rd());

/*
 * Coordenada
 * Almacena una posición (fila, columna) en la grilla.
 * Se usa para representar posiciones de drones, bases, obstáculos y urgencias.
 */
struct Coordenada {
    int fila;
    int col;
    
    bool operator<(const Coordenada& otra) const {
        if (fila != otra.fila) {
            return fila < otra.fila;
        }
        return col < otra.col;
    }
    
    bool operator==(const Coordenada& otra) const {
        return (fila == otra.fila) && (col == otra.col);
    }
};

/*
 * Instancia
 * Almacena toda la información del problema leída desde el archivo.
 * Se usa para acceder a las dimensiones del grid, obstáculos, bases y tasas de urgencia.
 */
struct Instancia {
    int filas;
    int columnas;
    set<Coordenada> obstaculos;
    map<Coordenada, double> tasas_urgencia;
    vector<Coordenada> bases;
    
    /*
     * Constructor
     * - Recibe: ruta del archivo de instancia
     * Lee y carga todos los datos del problema desde el archivo.
     * - Retorna: objeto Instancia inicializado
     */
    Instancia(const string& filename) {
        ifstream file(filename);
        string etiqueta;
        
        file >> etiqueta >> filas;
        file >> etiqueta >> columnas;
        
        int n_obstaculos;
        file >> etiqueta >> n_obstaculos;
        for (int i = 0; i < n_obstaculos; i++) {
            int fila, col;
            file >> fila >> col;
            obstaculos.insert({fila, col});
        }
        
        int n_urgencias;
        file >> etiqueta >> n_urgencias;
        for (int i = 0; i < n_urgencias; i++) {
            int fila, col, urgencia;
            file >> fila >> col >> urgencia;
            Coordenada coord = {fila, col};
            
            if (tasas_urgencia.find(coord) == tasas_urgencia.end()) {
                tasas_urgencia[coord] = 0.0;
            }
            tasas_urgencia[coord] += static_cast<double>(urgencia);
        }
        
        int n_bases;
        file >> etiqueta >> n_bases;
        for (int i = 0; i < n_bases; i++) {
            int id, fila, col;
            file >> id >> fila >> col;
            bases.push_back({fila, col});
        }
        
        file.close();
    }
};

/*
 * Individuo
 * Representa una solución candidata (cromosoma) del algoritmo evolutivo.
 * Se usa para almacenar el plan de vuelo completo de k drones por T ticks.
 */
struct Individuo {
    vector<int> base_ids;
    vector<vector<int>> acciones;
    double fitness;
    bool es_valido;
    
    /*
     * inicializarAleatorio
     * - Recibe: número de drones (k), número de ticks (T), instancia del problema
     * Genera un plan de vuelo aleatorio para todos los drones.
     * - Retorna: void (modifica el individuo actual)
     */
    void inicializarAleatorio(int k, int T, const Instancia& inst) {
        base_ids.clear();
        acciones.clear();
        fitness = 0.0;
        es_valido = false;
        
        int num_bases = inst.bases.size();
        uniform_int_distribution<int> dist_base(0, num_bases - 1);
        
        // Asignar bases aleatorias
        for (int i = 0; i < k; i++) {
            int base_id = dist_base(rng);
            base_ids.push_back(base_id);
        }
        
        // Generar acciones aleatorias (0=Stay, 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW)
        uniform_int_distribution<int> dist_accion(0, 8);
        for (int i = 0; i < k; i++) {
            vector<int> plan_dron;
            for (int t = 0; t < T; t++) {
                int accion = dist_accion(rng);
                plan_dron.push_back(accion);
            }
            acciones.push_back(plan_dron);
        }
    }
};

/*
 * crearDirectorio
 * - Recibe: ruta del directorio
 * Crea el directorio si no existe.
 * - Retorna: void
 */
void crearDirectorio(const string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        // Directorio no existe, crearlo
        mkdir(path.c_str(), 0777);
    }
}

/*
 * extraerNombreInstancia
 * - Recibe: ruta completa del archivo de instancia
 * Extrae solo el nombre del archivo sin extensión.
 * - Retorna: nombre de la instancia
 */
string extraerNombreInstancia(const string& ruta) {
    size_t ultima_barra = ruta.find_last_of("/\\");
    string nombre_archivo = (ultima_barra == string::npos) ? ruta : ruta.substr(ultima_barra + 1);
    
    size_t punto = nombre_archivo.find_last_of(".");
    return (punto == string::npos) ? nombre_archivo : nombre_archivo.substr(0, punto);
}

/*
 * aplicarAccion
 * - Recibe: posición actual, código de acción (0-8)
 * Calcula la nueva posición tras aplicar el movimiento.
 * - Retorna: nueva coordenada
 */
Coordenada aplicarAccion(const Coordenada& pos, int accion) {
    int nueva_fila = pos.fila;
    int nueva_col = pos.col;
    
    switch (accion) {
        case 0: break;                            // Permanecer
        case 1: nueva_fila--; break;              // Arriba
        case 2: nueva_fila--; nueva_col++; break; // Arriba-Derecha
        case 3: nueva_col++; break;               // Derecha
        case 4: nueva_fila++; nueva_col++; break; // Abajo-Derecha
        case 5: nueva_fila++; break;              // Abajo
        case 6: nueva_fila++; nueva_col--; break; // Abajo-Izquierda
        case 7: nueva_col--; break;               // Izquierda
        case 8: nueva_fila--; nueva_col--; break; // Arriba-Izquierda
    }
    
    return {nueva_fila, nueva_col};
}

/*
 * calcularFitness
 * - Recibe: individuo a evaluar, instancia del problema, horizonte temporal T
 * Simula el plan de vuelo y calcula la urgencia acumulada total.
 * Aplica penalización gradual para soluciones inválidas (mejor que penalización fija).
 * - Retorna: void (modifica fitness y es_valido del individuo)
 */
void calcularFitness(Individuo& ind, const Instancia& inst, int T) {
    int k = ind.base_ids.size();
    double urgencia_acumulada_total = 0.0;
    
    // Crear estado de urgencias e inicializar todas en 0.0
    map<Coordenada, double> urgencia_actual;
    for (const auto& par : inst.tasas_urgencia) {
        urgencia_actual[par.first] = 0.0;
    }
    
    // Inicializar posiciones de drones en bases
    // NOTA: Múltiples drones pueden despegar de la misma base
    vector<Coordenada> pos_drones(k);
    for (int i = 0; i < k; i++) {
        int base_id = ind.base_ids[i];
        Coordenada pos_base = inst.bases[base_id];
        pos_drones[i] = pos_base;
    }
    
    // Simulación tick por tick
    for (int t = 0; t < T; t++) {
        // 1. Acumular urgencia ANTES de incrementar (orden corregido)
        for (const auto& par : urgencia_actual) {
            urgencia_acumulada_total += par.second;
        }
        
        // 2. Incrementar urgencias no vigiladas
        set<Coordenada> pos_visitadas_en_tick;
        for (int d = 0; d < k; d++) {
            pos_visitadas_en_tick.insert(pos_drones[d]);
        }
        
        for (auto& par : urgencia_actual) {
            const Coordenada& coord = par.first;
            if (pos_visitadas_en_tick.count(coord) == 0) {
                urgencia_actual[coord] += inst.tasas_urgencia.at(coord);
            }
        }
        
        // 3. Mover y Validar nuevas posiciones
        vector<Coordenada> nuevas_pos_drones(k);
        set<Coordenada> nuevas_posiciones_set;
        
        // Verificar si una posición es una base
        auto es_base = [&inst](const Coordenada& pos) {
            for (const auto& base : inst.bases) {
                if (base == pos) return true;
            }
            return false;
        };
        
        for (int d = 0; d < k; d++) {
            int accion = ind.acciones[d][t];
            Coordenada nueva_pos = aplicarAccion(pos_drones[d], accion);
            
            // Verificar colisión (permitida SOLO en bases)
            bool hay_colision = (nuevas_posiciones_set.count(nueva_pos) > 0);
            bool es_una_base = es_base(nueva_pos);
            bool colision_fuera_de_base = hay_colision && !es_una_base;
            
            // Validaciones con penalización gradual
            if (nueva_pos.fila < 0 || nueva_pos.fila >= inst.filas ||
                nueva_pos.col < 0 || nueva_pos.col >= inst.columnas ||
                inst.obstaculos.count(nueva_pos) > 0 ||
                colision_fuera_de_base) {
                
                // --- LÓGICA DE PENALIZACIÓN GRADUAL ---
                
                // 1. Castigo base (para ser peor que cualquier solución válida)
                double penalizacion_base = 10000000.0; // 10 Millones
                
                // 2. Castigo por "morir pronto" - cuanto más ticks faltaban, peor es
                int ticks_restantes = T - t;
                double penalizacion_tiempo = ticks_restantes * 10000.0;
                
                ind.fitness = urgencia_acumulada_total + penalizacion_base + penalizacion_tiempo;
                ind.es_valido = false;
                return; // Termina la simulación
            }
            
            nuevas_posiciones_set.insert(nueva_pos);
            nuevas_pos_drones[d] = nueva_pos;
        }
        
        pos_drones = nuevas_pos_drones;
        
        // 4. Resetear urgencias vigiladas (si no hubo error)
        for (const Coordenada& coord : pos_visitadas_en_tick) {
            if (urgencia_actual.count(coord) > 0) {
                urgencia_actual[coord] = 0.0;
            }
        }
    }
    
    // Si el bucle termina, es una solución 100% válida
    ind.fitness = urgencia_acumulada_total;
    ind.es_valido = true;
}

/*
 * AlgoritmoEvolutivo
 * Gestiona la población de individuos y ejecuta el proceso evolutivo.
 * Se usa para encontrar la mejor solución mediante selección, cruce y mutación.
 */
class AlgoritmoEvolutivo {
public:
    vector<Individuo> poblacion;
    int tam_poblacion;
    double tasa_mutacion;
    int k_drones;
    int T_ticks;
    const Instancia& inst;

    AlgoritmoEvolutivo(int pop_size, double mut_rate, int k, int T, const Instancia& inst_ref)
        : tam_poblacion(pop_size), tasa_mutacion(mut_rate), k_drones(k), T_ticks(T), inst(inst_ref) {}

    /*
     * inicializarPoblacion
     * - Recibe:
     * Crea la población inicial con individuos aleatorios y los evalúa.
     * - Retorna: modifica la población
     */
    void inicializarPoblacion() {
        poblacion.clear();
        for (int i = 0; i < tam_poblacion; ++i) {
            Individuo ind;
            ind.inicializarAleatorio(k_drones, T_ticks, inst);
            repararIndividuo(ind); // Garantizar población inicial válida
            calcularFitness(ind, inst, T_ticks);
            poblacion.push_back(ind);
        }
    }

    /*
     * seleccionarPorTorneo
     * - Recibe: tamaño del torneo
     * Selecciona un individuo mediante torneo (compara individuos aleatorios).
     * - Retorna: referencia al mejor individuo del torneo
     */
    const Individuo& seleccionarPorTorneo(int tam_torneo) {
        uniform_int_distribution<int> dist_pop(0, tam_poblacion - 1);
        const Individuo* mejor_del_torneo = &poblacion[dist_pop(rng)];

        for (int i = 1; i < tam_torneo; ++i) {
            const Individuo* retador = &poblacion[dist_pop(rng)];
            if (retador->fitness < mejor_del_torneo->fitness) {
                mejor_del_torneo = retador;
            }
        }
        return *mejor_del_torneo;
    }

    /*
     * cruzarUnPunto
     * - Recibe: dos individuos padres
     * Crea un hijo combinando acciones de ambos padres en un punto de corte temporal.
     * - Retorna: nuevo individuo hijo
     */
    Individuo cruzarUnPunto(const Individuo& p1, const Individuo& p2) {
        Individuo hijo;
        hijo.base_ids = p1.base_ids;
        hijo.acciones.resize(k_drones, vector<int>(T_ticks));

        uniform_int_distribution<int> dist_corte(1, T_ticks - 2);
        int punto_corte_t = dist_corte(rng);

        for (int d = 0; d < k_drones; ++d) {
            for (int t = 0; t < punto_corte_t; ++t) {
                hijo.acciones[d][t] = p1.acciones[d][t];
            }
            for (int t = punto_corte_t; t < T_ticks; ++t) {
                hijo.acciones[d][t] = p2.acciones[d][t];
            }
        }
        return hijo;
    }

    /*
     * generarAccionValida
     * - Recibe: posición actual del dron
     * Genera una acción aleatoria que NO saque al dron fuera de la grilla.
     * - Retorna: código de acción válida (0-8)
     */
    int generarAccionValida(const Coordenada& pos) {
        vector<int> acciones_validas;
        acciones_validas.push_back(0); // Permanecer siempre es válido
        
        // Verificar cada dirección antes de agregarla como válida
        if (pos.fila > 0) {
            acciones_validas.push_back(1); // Arriba
            if (pos.col < inst.columnas - 1) acciones_validas.push_back(2); // Arriba-Derecha
            if (pos.col > 0) acciones_validas.push_back(8); // Arriba-Izquierda
        }
        
        if (pos.col < inst.columnas - 1) {
            acciones_validas.push_back(3); // Derecha
            if (pos.fila < inst.filas - 1) acciones_validas.push_back(4); // Abajo-Derecha
        }
        
        if (pos.fila < inst.filas - 1) {
            acciones_validas.push_back(5); // Abajo
            if (pos.col > 0) acciones_validas.push_back(6); // Abajo-Izquierda
        }
        
        if (pos.col > 0) {
            acciones_validas.push_back(7); // Izquierda
        }
        
        // Retornar acción válida aleatoria
        uniform_int_distribution<int> dist(0, acciones_validas.size() - 1);
        return acciones_validas[dist(rng)];
    }

    /*
     * mutar
     * - Recibe: individuo a mutar
     * Cambia aleatoriamente algunas acciones según la tasa de mutación.
     * MEJORADO: Ahora solo genera acciones que mantienen al dron dentro de la grilla.
     * - Retorna: void (modifica el individuo recibido)
     */
    void mutar(Individuo& ind) {
        uniform_real_distribution<double> dist_muta(0.0, 1.0);
        
        for (int d = 0; d < k_drones; ++d) {
            // Simular trayectoria para conocer posición en cada tick
            Coordenada pos_actual = inst.bases[ind.base_ids[d]];
            
            for (int t = 0; t < T_ticks; ++t) {
                if (dist_muta(rng) < tasa_mutacion) {
                    // Generar acción VÁLIDA (que no saque de la grilla)
                    ind.acciones[d][t] = generarAccionValida(pos_actual);
                }
                
                // Actualizar posición para siguiente tick
                pos_actual = aplicarAccion(pos_actual, ind.acciones[d][t]);
            }
        }
    }

    /*
     * repararIndividuo
     * - Recibe: individuo a reparar
     * Corrige acciones que sacarían al dron fuera de la grilla (forzando "permanecer").
     * - Retorna: void (modifica el individuo recibido)
     */
    void repararIndividuo(Individuo& ind) {
        for (int d = 0; d < k_drones; ++d) {
            Coordenada pos = inst.bases[ind.base_ids[d]];
            
            for (int t = 0; t < T_ticks; ++t) {
                int accion = ind.acciones[d][t];
                Coordenada nueva_pos = aplicarAccion(pos, accion);
                
                // Si la acción saca de la grilla, forzar "permanecer"
                if (nueva_pos.fila < 0 || nueva_pos.fila >= inst.filas ||
                    nueva_pos.col < 0 || nueva_pos.col >= inst.columnas) {
                    ind.acciones[d][t] = 0; // Permanecer
                } else {
                    pos = nueva_pos; // Actualizar posición
                }
            }
        }
    }

    /*
     * ejecutarGeneracion
     * - Recibe: nada (usa la población actual)
     * Aplica elitismo, selección, cruce y mutación para crear nueva generación.
     * - Retorna: void (reemplaza la población actual)
     */
    void ejecutarGeneracion() {
        vector<Individuo> nueva_poblacion;

        // Elitismo: preservar el mejor
        sort(poblacion.begin(), poblacion.end(), 
            [](const Individuo& a, const Individuo& b) {
                return a.fitness < b.fitness;
            });
        
        nueva_poblacion.push_back(poblacion[0]);

        // Crear nuevos individuos
        while (nueva_poblacion.size() < static_cast<size_t>(tam_poblacion)) {
            const Individuo& p1 = seleccionarPorTorneo(5);
            const Individuo& p2 = seleccionarPorTorneo(5);

            Individuo hijo = cruzarUnPunto(p1, p2);
            mutar(hijo);
            repararIndividuo(hijo); // Garantizar que el hijo sea válido espacialmente
            calcularFitness(hijo, inst, T_ticks);
            
            nueva_poblacion.push_back(hijo);
        }

        poblacion = nueva_poblacion;
    }

    /*
     * getMejorIndividuo
     * - Recibe: nada
     * Encuentra el individuo con menor fitness (mejor solución).
     * - Retorna: copia del mejor individuo
     */
    Individuo getMejorIndividuo() {
        sort(poblacion.begin(), poblacion.end(), 
            [](const Individuo& a, const Individuo& b) {
                return a.fitness < b.fitness;
            });
        return poblacion[0];
    }
};

/*
 * guardarResultadosCSV
 * - Recibe: nombre instancia, parámetros, mejor individuo, tiempo ejecución
 * Guarda estadísticas de ejecución en archivo CSV.
 * - Retorna: void
 */
void guardarResultadosCSV(const string& nombre_instancia, int num_drones, int K_iter, 
                          int T_ticks, const Individuo& mejor_ind, double tiempo_s) {
    crearDirectorio("resultados");
    
    stringstream ss;
    ss << "resultados/" << nombre_instancia << "_estadisticas.csv";
    
    // Verificar si el archivo ya existe para saber si escribir encabezado
    bool existe = ifstream(ss.str()).good();
    
    ofstream archivo(ss.str(), ios::app);  // Modo append
    
    // Escribir encabezado solo si es archivo nuevo
    if (!existe) {
        archivo << "num_drones,iteraciones,ticks_operacion,urgencia_acumulada,solucion_valida,tiempo_s\n";
    }
    
    // Agregar fila de datos
    archivo << num_drones << ","
            << K_iter << ","
            << T_ticks << ","
            << fixed << setprecision(2) << mejor_ind.fitness << ","
            << (mejor_ind.es_valido ? "Si" : "No") << ","
            << setprecision(3) << tiempo_s << "\n";
    
    archivo.close();
    cout << "\nEstadísticas guardadas en: " << ss.str() << endl;
}

/*
 * guardarRutasCSV
 * - Recibe: nombre instancia, parámetros, mejor individuo, instancia problema, horizonte T
 * Guarda las rutas de cada dron en formato CSV para visualización.
 * - Retorna: void
 */
void guardarRutasCSV(const string& nombre_instancia, int num_drones, int K_iter,
                     int T_ticks, const Individuo& mejor_ind, 
                     const Instancia& inst, int T) {
    crearDirectorio("resultados");
    
    stringstream ss;
    ss << "resultados/" << nombre_instancia << "_rutas.csv";
    
    // Verificar si el archivo ya existe para saber si escribir encabezado
    bool existe = ifstream(ss.str()).good();
    
    ofstream archivo(ss.str(), ios::app);  // Modo append
    
    // Escribir encabezado solo si es archivo nuevo
    if (!existe) {
        archivo << "num_drones,dron,tick,fila,columna,accion,base_origen\n";
    }
    
    int k = mejor_ind.base_ids.size();
    vector<Coordenada> pos_actuales(k);
    
    // Posiciones iniciales
    for (int d = 0; d < k; ++d) {
        int id_base = mejor_ind.base_ids[d];
        Coordenada pos_base = inst.bases[id_base];
        pos_actuales[d] = pos_base;
        
        // Tick 0 (posición inicial)
        archivo << num_drones << "," << d << ",0," << pos_base.fila << "," << pos_base.col 
                << ",0," << id_base << "\n";
    }
    
    // Simular cada tick
    for (int t = 0; t < T; ++t) {
        for (int d = 0; d < k; ++d) {
            int accion = mejor_ind.acciones[d][t];
            Coordenada nueva_pos = aplicarAccion(pos_actuales[d], accion);
            
            archivo << num_drones << "," << d << "," << (t + 1) << "," 
                    << nueva_pos.fila << "," << nueva_pos.col << "," 
                    << accion << "," << mejor_ind.base_ids[d] << "\n";
            
            pos_actuales[d] = nueva_pos;
        }
    }
    
    archivo.close();
    cout << "Rutas guardadas en: " << ss.str() << endl;
}

/*
 * imprimirMejorRuta
 * - Recibe: mejor individuo, instancia del problema, horizonte temporal T
 * Re-simula el plan de vuelo y muestra las rutas de cada dron.
 * - Retorna: void (imprime en consola)
 */
void imprimirMejorRuta(const Individuo& mejor_ind, const Instancia& inst, int T) {
    int k = mejor_ind.base_ids.size();
    
    vector<vector<Coordenada>> historial_rutas(k);
    vector<Coordenada> pos_actuales(k);

    // Posiciones iniciales en bases
    for (int d = 0; d < k; ++d) {
        int id_base = mejor_ind.base_ids[d];
        Coordenada pos_base = inst.bases[id_base];
        historial_rutas[d].push_back(pos_base);
        pos_actuales[d] = pos_base;
    }

    // Re-simular cada tick
    for (int t = 0; t < T; ++t) {
        for (int d = 0; d < k; ++d) {
            int accion = mejor_ind.acciones[d][t];
            Coordenada nueva_pos = aplicarAccion(pos_actuales[d], accion);
            historial_rutas[d].push_back(nueva_pos);
            pos_actuales[d] = nueva_pos;
        }
    }

    // Imprimir formato: D1: B0 - (f,c) - ...
    cout << "Rutas:" << endl;
    for (int d = 0; d < k; ++d) {
        cout << "D" << (d + 1) << ": B" << mejor_ind.base_ids[d];
        for (size_t t = 1; t < historial_rutas[d].size(); ++t) {
            const auto& pos = historial_rutas[d][t];
            cout << " - (" << pos.fila << "," << pos.col << ")";
        }
        cout << endl;
    }
}

int main(int argc, char* argv[]) {
    // Validar argumentos
    if (argc != 5) {
        cerr << "Error: Argumentos incorrectos." << endl;
        cerr << "Uso: ./PSP-UAV <ruta_instancia> <num_drones> <K_iteraciones> <T_ticks>" << endl;
        cerr << "Ejemplo: ./PSP-UAV instancias/PSP-UAV_01_a.txt 5 1000 50" << endl;
        return 1;
    }

    string ruta_instancia = argv[1];
    int num_drones = stoi(argv[2]);
    int K_iteraciones = stoi(argv[3]);
    int T_ticks_operacion = stoi(argv[4]);

    auto t_start = chrono::high_resolution_clock::now();

    // Cargar instancia del problema
    Instancia inst(ruta_instancia);

    // Parámetros del algoritmo evolutivo (ajustados para mejor convergencia)
    const int pop_size = 150;
    const double mut_rate = 0.05;

    cout << "--- Iniciando Búsqueda Evolutiva (PSP-UAV) ---" << endl;
    cout << "Instancia: " << ruta_instancia << endl;
    cout << "Número de drones: " << num_drones << endl;
    cout << "Iteraciones: " << K_iteraciones << endl;
    cout << "Ticks de operación (T): " << T_ticks_operacion << endl;
    cout << "------------------------------------------------" << endl;

    // Ejecutar algoritmo evolutivo con cantidad exacta de drones
    AlgoritmoEvolutivo ae(pop_size, mut_rate, num_drones, T_ticks_operacion, inst);
    ae.inicializarPoblacion();

    // Evolucionar durante K generaciones
    for (int g = 0; g < K_iteraciones; ++g) {
        ae.ejecutarGeneracion();
        
        // Mostrar progreso cada 10% de iteraciones
        if ((g + 1) % (K_iteraciones / 10) == 0 || g == 0) {
            Individuo mejor_actual = ae.getMejorIndividuo();
            cout << "Iteración " << (g + 1) << "/" << K_iteraciones 
                 << " - Mejor fitness: " << mejor_actual.fitness << endl;
        }
    }

    Individuo mejor_solucion_global = ae.getMejorIndividuo();

    auto t_end = chrono::high_resolution_clock::now();
    double tiempo_total_s = chrono::duration<double>(t_end - t_start).count();

    // Imprimir resultados finales
    cout << "\n--- FIN DE LA EJECUCIÓN ---" << endl;
    cout << fixed << setprecision(1);
    
    cout << "Urgencia acumulada: " << mejor_solucion_global.fitness << endl;
    cout << "Ventana de operación T: " << T_ticks_operacion << endl;
    cout << "Drones utilizados: " << num_drones << endl;
    cout << "Solución válida: " << (mejor_solucion_global.es_valido ? "Sí" : "No") << endl;
    cout << "Tiempo de ejecución: " << tiempo_total_s << "s" << endl;
    
    imprimirMejorRuta(mejor_solucion_global, inst, T_ticks_operacion);
    
    // Guardar resultados en archivos CSV
    string nombre_inst = extraerNombreInstancia(ruta_instancia);
    guardarResultadosCSV(nombre_inst, num_drones, K_iteraciones, T_ticks_operacion, 
                         mejor_solucion_global, tiempo_total_s);
    guardarRutasCSV(nombre_inst, num_drones, K_iteraciones, T_ticks_operacion,
                    mejor_solucion_global, inst, T_ticks_operacion);

    return 0;
}