# PSP-UAV - Algoritmo Evolutivo

Algoritmo Evolutivo para el problema de Planificación de Patrullaje con UAVs.

## Compilación

```bash
make        # Compilar
make clean  # Limpiar
```

O manualmente:
```bash
g++ -std=c++17 -O2 main.cpp -o PSP-UAV
```

## Uso

```bash
./PSP-UAV <archivo_instancia> <max_drones> <iteraciones> <ticks>
```

Ejemplo:
```bash
./PSP-UAV instancias/PSP-UAV_01_a.txt 5 1000 50
```

## Comandos Makefile

- `make` - Compila el programa
- `make clean` - Elimina el ejecutable
- `make test` - Ejecuta todas las instancias

## Parámetros

- **archivo_instancia**: Archivo de entrada
- **max_drones**: Máximo número de drones a probar
- **iteraciones**: Generaciones del algoritmo
- **ticks**: Horizonte temporal

## Salida

El programa muestra la urgencia acumulada, drones utilizados, tiempo de ejecución y las rutas de cada dron.

