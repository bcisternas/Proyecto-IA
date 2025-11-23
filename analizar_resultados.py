#!/usr/bin/env python3
"""
Script de análisis de resultados del algoritmo evolutivo PSP-UAV
Procesa los archivos CSV acumulativos y genera resúmenes y tablas LaTeX
"""

import csv
import os
from pathlib import Path
from collections import defaultdict

def cargar_resultados():
    """Carga todos los resultados de las ejecuciones"""
    resultados = []
    
    for archivo in sorted(Path("resultados").glob("*_estadisticas.csv")):
        instancia = archivo.stem.replace("_estadisticas", "")
        
        with open(archivo, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                resultados.append({
                    'instancia': instancia,
                    'num_drones': int(row['num_drones']),
                    'iteraciones': int(row['iteraciones']),
                    'ticks': int(row['ticks_operacion']),
                    'fitness': float(row['urgencia_acumulada']),
                    'valido': row['solucion_valida'] == 'Si',
                    'tiempo': float(row['tiempo_s'])
                })
    
    return resultados

def analizar_por_configuracion(resultados):
    """Analiza resultados agrupados por número de drones"""
    por_k = defaultdict(list)
    
    for r in resultados:
        por_k[r['num_drones']].append(r)
    
    print("\n" + "=" * 80)
    print("ANÁLISIS POR CONFIGURACIÓN DE DRONES")
    print("=" * 80)
    
    for k in sorted(por_k.keys()):
        datos = por_k[k]
        validos = [r for r in datos if r['valido']]
        invalidos = [r for r in datos if not r['valido']]
        
        print(f"\n🔹 k={k} drones:")
        print(f"   ✅ Soluciones válidas: {len(validos)}/{len(datos)}")
        print(f"   ❌ Soluciones inválidas: {len(invalidos)}/{len(datos)}")
        
        if validos:
            fitness_validos = [r['fitness'] for r in validos]
            print(f"   📊 Fitness promedio (válidos): {sum(fitness_validos)/len(fitness_validos):,.0f}")
            print(f"   📈 Rango fitness: {min(fitness_validos):,.0f} - {max(fitness_validos):,.0f}")
        
        tiempos = [r['tiempo'] for r in datos]
        print(f"   ⏱️  Tiempo promedio: {sum(tiempos)/len(tiempos):.2f}s")

def generar_tabla_latex(resultados):
    """Genera tabla LaTeX con los resultados"""
    print("\n" + "=" * 80)
    print("TABLA LaTeX PARA EL INFORME")
    print("=" * 80)
    
    print(r"""
\begin{table}[htbp]
\centering
\caption{Resultados experimentales del algoritmo evolutivo para PSP-UAV}
\label{tab:resultados_experimentos}
\small
\begin{tabular}{lrrrr}
\hline
\textbf{Instancia} & \textbf{$k$} & \textbf{Urgencia} & \textbf{Válida} & \textbf{Tiempo (s)} \\
\hline""")
    
    # Agrupar por instancia
    por_instancia = defaultdict(list)
    for r in resultados:
        por_instancia[r['instancia']].append(r)
    
    for instancia in sorted(por_instancia.keys()):
        datos = sorted(por_instancia[instancia], key=lambda x: x['num_drones'])
        
        for i, r in enumerate(datos):
            inst_tex = instancia.replace('_', r'\_')
            valido_tex = r'\checkmark' if r['valido'] else r'\texttimes'
            
            # Formatear fitness
            if r['fitness'] > 1000000:
                fitness_tex = r'\textit{' + f"{r['fitness']:,.0f}".replace(',', '.') + '}'
            else:
                fitness_tex = f"{r['fitness']:,.0f}".replace(',', '.')
            
            print(f"{inst_tex} & {r['num_drones']} & {fitness_tex} & {valido_tex} & {r['tiempo']:.2f} " + r"\\")
        
        print(r"\hline")
    
    print(r"""\end{tabular}
\begin{tablenotes}
\small
\item \textit{Nota:} Valores en cursiva representan soluciones inválidas ($\geq 10^7$).
\item \checkmark: Válida; \texttimes: Inválida (colisiones/fuera de límites).
\end{tablenotes}
\end{table}
""")

def generar_resumen_consolidado(resultados):
    """Genera un CSV consolidado para análisis en Python/Excel"""
    archivo_salida = "resumen_completo.csv"
    
    with open(archivo_salida, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=[
            'instancia', 'num_drones', 'iteraciones', 'ticks',
            'urgencia_acumulada', 'es_valida', 'tiempo_s'
        ])
        writer.writeheader()
        
        for r in sorted(resultados, key=lambda x: (x['instancia'], x['num_drones'])):
            writer.writerow({
                'instancia': r['instancia'],
                'num_drones': r['num_drones'],
                'iteraciones': r['iteraciones'],
                'ticks': r['ticks'],
                'urgencia_acumulada': r['fitness'],
                'es_valida': 'Si' if r['valido'] else 'No',
                'tiempo_s': r['tiempo']
            })
    
    print(f"\n✅ Archivo consolidado guardado: {archivo_salida}")

def main():
    print("=" * 80)
    print("ANÁLISIS DE RESULTADOS - PSP-UAV")
    print("=" * 80)
    
    resultados = cargar_resultados()
    
    print(f"\n📁 Archivos procesados: {len(set(r['instancia'] for r in resultados))}")
    print(f"📊 Total de experimentos: {len(resultados)}")
    print(f"🔧 Configuraciones de drones: {sorted(set(r['num_drones'] for r in resultados))}")
    
    # Tabla general
    print("\n" + "=" * 80)
    print(f"{'Instancia':<18} {'k':<5} {'Fitness':<15} {'Válida':<8} {'Tiempo (s)':<12}")
    print("=" * 80)
    
    for r in sorted(resultados, key=lambda x: (x['instancia'], x['num_drones'])):
        valido_str = "✓" if r['valido'] else "✗"
        print(f"{r['instancia']:<18} {r['num_drones']:<5} {r['fitness']:<15,.0f} {valido_str:<8} {r['tiempo']:<12.2f}")
    
    # Análisis por configuración
    analizar_por_configuracion(resultados)
    
    # Tabla LaTeX
    generar_tabla_latex(resultados)
    
    # Archivo consolidado
    generar_resumen_consolidado(resultados)

if __name__ == "__main__":
    main()
