# Formato de Salida CSV - PSP-UAV

## Descripción General

Los resultados del algoritmo evolutivo se guardan en formato CSV acumulativo, donde **cada instancia genera solo 2 archivos**:
- `{instancia}_estadisticas.csv`: Resumen de resultados por configuración
- `{instancia}_rutas.csv`: Trayectorias detalladas de todos los drones

## Estructura de Archivos

### Archivos de Estadísticas

**Nombre**: `PSP-UAV_{id}_estadisticas.csv`

**Formato**:
```csv
num_drones,iteraciones,ticks_operacion,urgencia_acumulada,solucion_valida,tiempo_s
3,1000,50,152029.00,Si,2.324
5,1000,50,138942.00,Si,2.058
10,1000,50,10500000.00,No,0.626
```

**Columnas**:
- `num_drones`: Número de UAVs utilizados (k)
- `iteraciones`: Iteraciones del algoritmo evolutivo
- `ticks_operacion`: Horizonte temporal (T)
- `urgencia_acumulada`: Fitness final (urgencia no atendida)
- `solucion_valida`: "Si" o "No" (sin colisiones ni salidas de límites)
- `tiempo_s`: Tiempo de ejecución en segundos

### Archivos de Rutas

**Nombre**: `PSP-UAV_{id}_rutas.csv`

**Formato**:
```csv
num_drones,dron,tick,fila,columna,accion,base_origen
3,0,0,4,5,0,0
3,0,1,5,5,2,0
3,0,2,5,6,4,0
...
```

**Columnas**:
- `num_drones`: Configuración de drones (permite filtrar por k)
- `dron`: ID del dron (0 a k-1)
- `tick`: Instante de tiempo (0 a T)
- `fila`: Coordenada de fila
- `columna`: Coordenada de columna
- `accion`: Acción tomada (0=quieto, 1=arriba, 2=abajo, 3=izq, 4=der)
- `base_origen`: ID de la base asignada


## Uso en Python

### Cargar Estadísticas

```python
import pandas as pd

# Cargar todas las estadísticas de una instancia
df = pd.read_csv('resultados/PSP-UAV_01_a_estadisticas.csv')

# Filtrar solo resultados con 3 drones
df_k3 = df[df['num_drones'] == 3]

# Obtener solo soluciones válidas
df_validos = df[df['solucion_valida'] == 'Si']
```

### Cargar Rutas

```python
import pandas as pd

# Cargar rutas
df_rutas = pd.read_csv('resultados/PSP-UAV_01_a_rutas.csv')

# Filtrar rutas de configuración con 5 drones
rutas_k5 = df_rutas[df_rutas['num_drones'] == 5]

# Obtener ruta específica de un dron
ruta_dron_0 = rutas_k5[rutas_k5['dron'] == 0]

# Coordenadas como arrays
filas = ruta_dron_0['fila'].values
columnas = ruta_dron_0['columna'].values
```

### Visualización

```python
from visualizar_rutas import visualizar_rutas, comparar_configuraciones

# Visualizar una configuración específica
visualizar_rutas('PSP-UAV_01_a', num_drones=3)

# Comparar todas las configuraciones
comparar_configuraciones('PSP-UAV_02_a')
```

## Análisis de Resultados

### Script de Análisis Automático

```bash
python3 analizar_resultados.py
```

Este script genera:
1. Tabla resumen en consola
2. Análisis por configuración de drones
3. Tabla LaTeX para el informe
4. `resumen_completo.csv` consolidado

### Resumen Consolidado

El archivo `resumen_completo.csv` contiene todos los experimentos en un solo CSV para análisis en Excel/Python:

```csv
instancia,num_drones,iteraciones,ticks,urgencia_acumulada,es_valida,tiempo_s
PSP-UAV_01_a,3,1000,50,152029.00,Si,2.324
PSP-UAV_01_a,5,1000,50,138942.00,Si,2.058
PSP-UAV_01_a,10,1000,50,10500000.00,No,0.626
...
```

## Estructura del Proyecto

```
Proyecto_UAVs_IA/
├── main.cpp                    # Código fuente del algoritmo
├── PSP-UAV                     # Ejecutable compilado
├── Makefile                    # Compilación
├── analizar_resultados.py      # Script de análisis
├── visualizar_rutas.py         # Script de visualización
├── resumen_completo.csv        # Resumen consolidado
├── instancias/                 # Archivos de entrada
│   ├── PSP-UAV_01_a.txt
│   └── ...
└── resultados/                 # Archivos CSV de salida
    ├── PSP-UAV_01_a_estadisticas.csv
    ├── PSP-UAV_01_a_rutas.csv
    └── ...
```

## Ejecución de Experimentos

Para ejecutar un experimento:

```bash
./PSP-UAV instancias/PSP-UAV_01_a.txt <num_drones> <iteraciones> <ticks>
```

Ejemplo con múltiples configuraciones:

```bash
# Ejecutar con 3, 5 y 10 drones sobre la misma instancia
for k in 3 5 10; do
    ./PSP-UAV instancias/PSP-UAV_01_a.txt $k 1000 50
done

# Los resultados se acumulan en:
# - resultados/PSP-UAV_01_a_estadisticas.csv (3 filas)
# - resultados/PSP-UAV_01_a_rutas.csv (todas las trayectorias)
```

## Notas Importantes

1. **Modo Append**: Los archivos CSV se abren en modo `append`, por lo que:
   - La primera ejecución crea el archivo con encabezado
   - Ejecuciones posteriores agregan filas
   - Para reiniciar: eliminar archivos de `resultados/`

2. **Identificación de Configuraciones**: La columna `num_drones` permite:
   - Filtrar resultados por configuración
   - Comparar diferentes valores de k
   - Analizar escalabilidad del algoritmo

3. **Validación de Soluciones**: 
   - `solucion_valida=Si`: Sin colisiones ni salidas de grilla
   - `solucion_valida=No`: Fitness penalizado (≥ 10^7)

4. **Fitness**:
   - Valores < 1M: Solución válida (urgencia real acumulada)
   - Valores ≥ 10M: Solución inválida (penalización aplicada)
