#!/usr/bin/env python3
"""
Script de visualización de rutas PSP-UAV
Permite visualizar las rutas de diferentes configuraciones de drones
desde un solo archivo CSV acumulativo
"""

import csv
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

def cargar_rutas(archivo, num_drones_filtro=None):
    """
    Carga rutas desde archivo CSV
    Si num_drones_filtro es None, carga todas las configuraciones
    """
    rutas = {}
    
    with open(archivo, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            k = int(row['num_drones'])
            
            # Filtrar por número de drones si se especifica
            if num_drones_filtro is not None and k != num_drones_filtro:
                continue
            
            if k not in rutas:
                rutas[k] = {}
            
            dron = int(row['dron'])
            if dron not in rutas[k]:
                rutas[k][dron] = []
            
            rutas[k][dron].append({
                'tick': int(row['tick']),
                'fila': int(row['fila']),
                'col': int(row['columna']),
                'accion': int(row['accion']),
                'base': int(row['base_origen'])
            })
    
    return rutas

def visualizar_rutas(archivo_instancia, num_drones, titulo=None):
    """Visualiza las rutas de una configuración específica"""
    archivo_rutas = f"resultados/{archivo_instancia}_rutas.csv"
    
    if not Path(archivo_rutas).exists():
        print(f"❌ Archivo no encontrado: {archivo_rutas}")
        return
    
    rutas = cargar_rutas(archivo_rutas, num_drones)
    
    if num_drones not in rutas:
        print(f"❌ No hay datos para {num_drones} drones")
        return
    
    # Crear figura
    plt.figure(figsize=(12, 10))
    
    colores = plt.cm.tab10(np.linspace(0, 1, num_drones))
    
    # Plotear cada dron
    for dron_id, trayectoria in sorted(rutas[num_drones].items()):
        filas = [p['fila'] for p in trayectoria]
        cols = [p['col'] for p in trayectoria]
        
        # Línea de trayectoria
        plt.plot(cols, filas, '-', color=colores[dron_id], 
                alpha=0.6, linewidth=2, label=f'Dron {dron_id}')
        
        # Punto inicial (base)
        plt.plot(cols[0], filas[0], 'o', color=colores[dron_id], 
                markersize=12, markeredgecolor='black', markeredgewidth=2)
        
        # Punto final
        plt.plot(cols[-1], filas[-1], 's', color=colores[dron_id], 
                markersize=10, markeredgecolor='black', markeredgewidth=1.5)
    
    plt.xlabel('Columna', fontsize=12)
    plt.ylabel('Fila', fontsize=12)
    plt.title(titulo or f'{archivo_instancia} - {num_drones} drones', fontsize=14, fontweight='bold')
    plt.legend(loc='best', fontsize=10)
    plt.grid(True, alpha=0.3)
    plt.gca().invert_yaxis()  # Invertir eje Y para que (0,0) esté arriba-izquierda
    
    # Guardar figura
    archivo_salida = f"resultados/{archivo_instancia}_k{num_drones}_mapa.png"
    plt.savefig(archivo_salida, dpi=150, bbox_inches='tight')
    print(f"✅ Gráfico guardado: {archivo_salida}")
    
    plt.close()

def comparar_configuraciones(archivo_instancia):
    """Genera subplots comparando diferentes configuraciones de drones"""
    archivo_rutas = f"resultados/{archivo_instancia}_rutas.csv"
    
    if not Path(archivo_rutas).exists():
        print(f"❌ Archivo no encontrado: {archivo_rutas}")
        return
    
    rutas = cargar_rutas(archivo_rutas)
    configuraciones = sorted(rutas.keys())
    
    n_configs = len(configuraciones)
    fig, axes = plt.subplots(1, n_configs, figsize=(6*n_configs, 5))
    
    if n_configs == 1:
        axes = [axes]
    
    for idx, k in enumerate(configuraciones):
        ax = axes[idx]
        colores = plt.cm.tab10(np.linspace(0, 1, k))
        
        for dron_id, trayectoria in sorted(rutas[k].items()):
            filas = [p['fila'] for p in trayectoria]
            cols = [p['col'] for p in trayectoria]
            
            ax.plot(cols, filas, '-', color=colores[dron_id], 
                   alpha=0.6, linewidth=1.5, label=f'D{dron_id}')
            ax.plot(cols[0], filas[0], 'o', color=colores[dron_id], 
                   markersize=10, markeredgecolor='black', markeredgewidth=2)
        
        ax.set_xlabel('Columna')
        ax.set_ylabel('Fila')
        ax.set_title(f'k={k} drones', fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.invert_yaxis()
        if k <= 5:
            ax.legend(fontsize=8)
    
    plt.suptitle(f'{archivo_instancia} - Comparación de configuraciones', 
                fontsize=14, fontweight='bold')
    
    archivo_salida = f"resultados/{archivo_instancia}_comparacion.png"
    plt.savefig(archivo_salida, dpi=150, bbox_inches='tight')
    print(f"✅ Gráfico comparativo guardado: {archivo_salida}")
    
    plt.close()

def main():
    """Ejemplo de uso"""
    print("=" * 70)
    print("VISUALIZACIÓN DE RUTAS PSP-UAV")
    print("=" * 70)
    
    # Ejemplo 1: Visualizar PSP-UAV_01_a con 3 drones
    print("\n📊 Generando visualización para PSP-UAV_01_a (k=3)...")
    visualizar_rutas("PSP-UAV_01_a", 3)
    
    # Ejemplo 2: Comparar todas las configuraciones de PSP-UAV_02_a
    print("\n📊 Generando comparación para PSP-UAV_02_a...")
    comparar_configuraciones("PSP-UAV_02_a")
    
    print("\n✨ Visualizaciones completadas!")
    print("\n💡 Uso personalizado:")
    print("   from visualizar_rutas import visualizar_rutas, comparar_configuraciones")
    print("   visualizar_rutas('PSP-UAV_01_a', 5)")
    print("   comparar_configuraciones('PSP-UAV_03_b')")

if __name__ == "__main__":
    # Verificar si matplotlib está disponible
    try:
        import matplotlib.pyplot as plt
        main()
    except ImportError:
        print("❌ Error: matplotlib no está instalado")
        print("   Instalar con: pip install matplotlib")
