# Mejoras Propuestas para el Algoritmo Evolutivo PSP-UAV

## 📊 Contexto Actual

**Resultados Experimentales (21 experimentos):**
- **k=3 drones**: ✅ 100% éxito (7/7) - Fitness prom: 826,905 - Tiempo: 7.88s
- **k=5 drones**: ⚠️ 57% éxito (4/7) - Fitness prom: 1,185,581 - Tiempo: 7.25s
- **k=10 drones**: ❌ 0% éxito (0/7) - Todas inválidas - Tiempo: 4.61s

**Problemas Críticos Identificados:**
1. **Coordenadas negativas**: 8 de 10 drones salen de la grilla (rango: filas [-8, 18], cols [-7, 12])
2. **Colisiones masivas**: 34 de 50 ticks con colisiones múltiples
3. **Parámetros inadecuados**: Población/iteraciones fijas independiente de k

---

## 🔍 Análisis de Mutación Actual

### Implementación Actual (líneas 377-388 de main.cpp)

```cpp
void mutar(Individuo& ind) {
    uniform_real_distribution<double> dist_muta(0.0, 1.0);
    uniform_int_distribution<int> dist_accion(0, 8);  // ← 9 acciones posibles
    
    for (int d = 0; d < k_drones; ++d) {
        for (int t = 0; t < T_ticks; ++t) {
            if (dist_muta(rng) < tasa_mutacion) {
                ind.acciones[d][t] = dist_accion(rng);  // ← Acción TOTALMENTE aleatoria
            }
        }
    }
}
```

**Acciones disponibles (0-8):**
- 0: Permanecer
- 1: Arriba
- 2: Arriba-Derecha (diagonal)
- 3: Derecha
- 4: Abajo-Derecha (diagonal)
- 5: Abajo
- 6: Abajo-Izquierda (diagonal)
- 7: Izquierda
- 8: Arriba-Izquierda (diagonal)

### ❌ Problemas de la Mutación Actual

1. **Mutación ciega**: No considera límites de la grilla
   - Genera acciones sin verificar si la posición resultante es válida
   - Puede generar movimientos fuera de [0, filas-1] × [0, columnas-1]

2. **No consciente del contexto espacial**:
   - Un dron en (0, 5) puede mutar a "Arriba" → (-1, 5) ❌
   - Un dron en (9, 0) puede mutar a "Izquierda" → (9, -1) ❌

3. **Tasa de mutación fija**:
   - `tasa_mutacion` no se adapta a la complejidad del problema
   - Mismo valor para k=3 que para k=10

4. **Mutación uniforme en todas las acciones**:
   - Todas las acciones tienen igual probabilidad (1/9 = 11.1%)
   - No favorece acciones conservadoras (permanecer) en situaciones críticas

---

## 🎯 MEJORAS PRIORITARIAS

### 🔴 [URGENTE-1] Restricción de Límites en Mutación

**Objetivo**: Garantizar que NINGUNA solución tenga coordenadas fuera de la grilla.

**Implementación**:

```cpp
// Nueva función auxiliar: Genera acción válida desde una posición
int generarAccionValida(const Coordenada& pos, const Instancia& inst) {
    vector<int> acciones_validas;
    acciones_validas.push_back(0); // Permanecer siempre es válido
    
    // Verificar cada dirección
    if (pos.fila > 0) {
        acciones_validas.push_back(1); // Arriba
        if (pos.col < inst.columnas - 1) acciones_validas.push_back(2); // Arriba-Der
        if (pos.col > 0) acciones_validas.push_back(8); // Arriba-Izq
    }
    
    if (pos.col < inst.columnas - 1) {
        acciones_validas.push_back(3); // Derecha
        if (pos.fila < inst.filas - 1) acciones_validas.push_back(4); // Abajo-Der
    }
    
    if (pos.fila < inst.filas - 1) {
        acciones_validas.push_back(5); // Abajo
        if (pos.col > 0) acciones_validas.push_back(6); // Abajo-Izq
    }
    
    if (pos.col > 0) {
        acciones_validas.push_back(7); // Izquierda
    }
    
    // Retornar acción válida aleatoria
    uniform_int_distribution<int> dist(0, acciones_validas.size() - 1);
    return acciones_validas[dist(rng)];
}
```

**Modificar función `mutar`**:

```cpp
void mutar(Individuo& ind) {
    uniform_real_distribution<double> dist_muta(0.0, 1.0);
    
    for (int d = 0; d < k_drones; ++d) {
        // Simular trayectoria del dron para saber posición en cada tick
        Coordenada pos_actual = instancia.bases[ind.base_ids[d]];
        
        for (int t = 0; t < T_ticks; ++t) {
            if (dist_muta(rng) < tasa_mutacion) {
                // En lugar de acción aleatoria, generar acción VÁLIDA
                ind.acciones[d][t] = generarAccionValida(pos_actual, instancia);
            }
            
            // Actualizar posición para siguiente tick
            pos_actual = aplicarAccion(pos_actual, ind.acciones[d][t]);
        }
    }
}
```

**Impacto Esperado**:
- ✅ **100% de soluciones dentro de límites** (0% con coordenadas negativas)
- ✅ Elimina fitness penalizado por salida de grilla
- ✅ Permite que el algoritmo se enfoque en minimizar urgencia y colisiones
- 📈 k=10: 0% → 40-50% éxito estimado

**Esfuerzo**: ~30 líneas de código | **Tiempo**: 30 minutos

---

### 🟡 [URGENTE-2] Reparación Post-Mutación y Post-Crossover

**Objetivo**: Asegurar que incluso después de crossover, las soluciones sean válidas.

**Implementación**:

```cpp
void repararIndividuo(Individuo& ind) {
    for (int d = 0; d < k_drones; ++d) {
        Coordenada pos = instancia.bases[ind.base_ids[d]];
        
        for (int t = 0; t < T_ticks; ++t) {
            int accion = ind.acciones[d][t];
            Coordenada nueva_pos = aplicarAccion(pos, accion);
            
            // Si la acción saca de la grilla, forzar "permanecer"
            if (nueva_pos.fila < 0 || nueva_pos.fila >= instancia.filas ||
                nueva_pos.col < 0 || nueva_pos.col >= instancia.columnas) {
                ind.acciones[d][t] = 0; // Permanecer
                // No actualizar posición
            } else {
                pos = nueva_pos; // Actualizar posición
            }
        }
    }
}
```

**Llamar después de cruce y mutación**:

```cpp
void ejecutarGeneracion() {
    // ... código existente ...
    
    while (nueva_poblacion.size() < tam_poblacion) {
        Individuo padre1 = seleccionTorneo();
        Individuo padre2 = seleccionTorneo();
        Individuo hijo = cruzar(padre1, padre2);
        mutar(hijo);
        
        repararIndividuo(hijo); // ← NUEVA LÍNEA
        
        nueva_poblacion.push_back(hijo);
    }
    
    poblacion = nueva_poblacion;
}
```

**Impacto Esperado**:
- ✅ Garantía de validez espacial en toda la población
- ✅ Crossover puede mezclar rutas sin romper restricciones
- 📈 +10-15% adicional en tasa de éxito

**Esfuerzo**: ~20 líneas de código | **Tiempo**: 15 minutos

---

## 🔵 Análisis de Tipos de Mutación Alternativos

### Opción 1: Mutación Uniforme Restringida (IMPLEMENTAR)
**Estado**: ✅ Propuesta en [URGENTE-1]

**Ventajas**:
- Garantiza validez espacial
- Fácil de implementar
- Compatible con operador de crossover actual

**Desventajas**:
- Sigue siendo "ciega" al objetivo (minimizar urgencia)
- No considera distribución espacial de obstáculos

---

### Opción 2: Mutación Gaussiana Adaptativa

**Concepto**: En lugar de cambiar completamente la acción, perturbar levemente.

```cpp
void mutarGaussiana(Individuo& ind) {
    normal_distribution<double> gaussiana(0.0, 1.0);
    
    for (int d = 0; d < k_drones; ++d) {
        Coordenada pos = instancia.bases[ind.base_ids[d]];
        
        for (int t = 0; t < T_ticks; ++t) {
            if (dist_muta(rng) < tasa_mutacion) {
                int accion_actual = ind.acciones[d][t];
                
                // Perturbar: acción vecina (±1 en el "espacio de acciones")
                double perturbacion = gaussiana(rng);
                int nueva_accion = accion_actual + (int)perturbacion;
                nueva_accion = max(0, min(8, nueva_accion)); // Clamping
                
                // Validar que nueva acción sea factible
                Coordenada test_pos = aplicarAccion(pos, nueva_accion);
                if (esPosicionValida(test_pos, instancia)) {
                    ind.acciones[d][t] = nueva_accion;
                } // else: mantener acción actual
            }
            
            pos = aplicarAccion(pos, ind.acciones[d][t]);
        }
    }
}
```

**Ventajas**:
- Convergencia más suave (cambios locales)
- Preserva "buenos patrones" de movimiento
- Reduce disrupciones en rutas funcionales

**Desventajas**:
- Puede converger prematuramente (menos exploración)
- Más compleja de implementar correctamente

**Recomendación**: ⚠️ Implementar DESPUÉS de [URGENTE-1] si se necesita refinamiento

---

### Opción 3: Mutación Dirigida por Urgencia (Heurística)

**Concepto**: Favorecer acciones que muevan al dron hacia zonas de alta urgencia.

```cpp
int mutarDirigida(Coordenada pos_actual, int tick_actual, const Instancia& inst) {
    // Encontrar celda con mayor urgencia no atendida en radio cercano
    double max_urgencia = -1;
    Coordenada objetivo;
    
    for (int df = -3; df <= 3; df++) {
        for (int dc = -3; dc <= 3; dc++) {
            int f = pos_actual.fila + df;
            int c = pos_actual.col + dc;
            
            if (f >= 0 && f < inst.filas && c >= 0 && c < inst.columnas) {
                double urgencia = inst.tasas_urgencia[f][c];
                if (urgencia > max_urgencia) {
                    max_urgencia = urgencia;
                    objetivo = {f, c};
                }
            }
        }
    }
    
    // Elegir acción que acerque al objetivo
    return seleccionarAccionHacia(pos_actual, objetivo, inst);
}
```

**Ventajas**:
- Búsqueda guiada (no ciega)
- Convergencia más rápida a buenas soluciones
- Explota estructura del problema

**Desventajas**:
- Puede causar convergencia prematura (greedy)
- Pierde diversidad poblacional
- Más compleja (~60 líneas adicionales)

**Recomendación**: 💡 Implementar como **operador híbrido** (50% uniforme + 50% dirigida)

---

### Opción 4: Mutación por Intercambio Temporal (Swap)

**Concepto**: En lugar de cambiar acciones individuales, intercambiar segmentos de la trayectoria.

```cpp
void mutarSwap(Individuo& ind) {
    for (int d = 0; d < k_drones; ++d) {
        if (dist_muta(rng) < tasa_mutacion) {
            // Elegir dos puntos aleatorios en el tiempo
            int t1 = rand() % T_ticks;
            int t2 = rand() % T_ticks;
            
            // Intercambiar acciones en esos ticks
            swap(ind.acciones[d][t1], ind.acciones[d][t2]);
        }
    }
}
```

**Ventajas**:
- Preserva buenas secuencias de acciones
- Útil para reordenar visitas a zonas

**Desventajas**:
- Puede romper validez espacial fácilmente
- Menos efectiva en este problema (urgencia acumula en el tiempo)

**Recomendación**: ❌ No adecuada para PSP-UAV

---

## 📋 Comparación de Mutaciones

| Tipo | Validez Garantizada | Complejidad | Impacto k=10 | Recomendación |
|------|---------------------|-------------|--------------|---------------|
| **Uniforme Restringida** | ✅ Sí | Baja (~30 líneas) | 0% → 50% | ✅ **IMPLEMENTAR AHORA** |
| **Gaussiana Adaptativa** | ⚠️ Con validación | Media (~40 líneas) | +10% adicional | ⏳ Fase 2 |
| **Dirigida por Urgencia** | ✅ Con validación | Alta (~60 líneas) | +20% adicional | 💡 Fase 3 (opcional) |
| **Swap Temporal** | ❌ No | Baja (~15 líneas) | Negativo | ❌ No usar |

---

## 🎯 Plan de Implementación Recomendado

### Fase 1 - INMEDIATA (Resolver k=10)
**Objetivo**: Eliminar soluciones con coordenadas negativas

1. ✅ **[URGENTE-1]** Mutación con validación de límites
2. ✅ **[URGENTE-2]** Reparación post-operadores
3. ✅ Ejecutar experimentos y verificar: **0 coordenadas negativas**

**Resultado Esperado**:
- k=3: 100% → 100% ✅
- k=5: 57% → 85-90% 📈
- k=10: 0% → 45-50% 🚀

---

### Fase 2 - CORTO PLAZO (Mejorar tasa de éxito)
**Objetivo**: Alcanzar >60% éxito con k=10

4. ⭐ **[A2]** Escalamiento de parámetros según k
   ```cpp
   int poblacion = 50 + (num_drones - 3) * 15;
   int iteraciones = 1000 + (num_drones - 3) * 300;
   ```

5. ⭐ **[B1]** Penalización dinámica de colisiones
   ```cpp
   int generacion_actual = /* ... */;
   double factor_penalizacion = 1000 + (generacion_actual / 500) * 4000;
   ```

**Resultado Esperado**:
- k=5: 85% → 95% 📈
- k=10: 50% → 65-70% 📈

---

### Fase 3 - OPCIONAL (Refinamiento)
**Objetivo**: Maximizar calidad de fitness

6. 💡 Mutación híbrida (50% uniforme + 50% dirigida)
7. 💡 Búsqueda local post-evolución
8. 💡 Inicialización inteligente

---

## 📊 Matriz de Impacto vs Esfuerzo

```
   Alto │                      
Impacto│  [URGENTE-1] ●              [A2] ●
        │  [URGENTE-2] ●              
        │                             [B1] ●
   Bajo │                   [C1] ●         [C2] ●
        └─────────────────────────────────────────
           Bajo              Medio          Alto
                         Esfuerzo
```

---

## ✅ Próximos Pasos

### Paso 1: Implementar [URGENTE-1] y [URGENTE-2]
- Tiempo estimado: **45 minutos**
- Cambios en: `mutar()`, nueva función `generarAccionValida()`, nueva función `repararIndividuo()`
- Líneas de código: ~50 líneas

### Paso 2: Validar Resultados
```bash
# Limpiar resultados anteriores
rm -rf resultados/*

# Ejecutar experimentos
for inst in instancias/PSP-UAV_*.txt; do
    for k in 3 5 10; do
        ./PSP-UAV "$inst" $k 1000 50
    done
done

# Analizar
python3 analizar_resultados.py
```

**Métricas a verificar**:
- ✅ **Coordenadas negativas**: Debe ser 0 (actualmente 8/10 drones con k=10)
- 📊 **Tasa de éxito k=10**: Objetivo >45% (actualmente 0%)
- 📊 **Colisiones**: Reducción esperada ~30%

### Paso 3: Decisión de Fase 2
Si **Fase 1** alcanza:
- k=10 ≥ 50% éxito → Pasar a **Fase 2**
- k=10 < 50% éxito → Revisar implementación o considerar mutación dirigida

---

## 📝 Notas Importantes

1. **No cambiar crossover aún**: El operador actual funciona bien para k=3, las mejoras deben enfocarse en mutación y parámetros.

2. **Validez ≠ Factibilidad**: Las mejoras propuestas garantizan que las soluciones estén **dentro de la grilla**, pero no necesariamente **sin colisiones**. Para colisiones, se requiere Fase 2 (penalización dinámica).

3. **Monitorear diversidad**: Si la mutación restringida causa convergencia prematura, considerar **aumentar tasa de mutación** para k≥5.

4. **Benchmark**: Guardar resultados actuales (k=10: 0%) como baseline para comparar mejoras.

---

## 🔬 Conclusión

La **mutación actual es inadecuada** porque:
- ❌ No valida límites espaciales
- ❌ Tasa fija sin adaptación a complejidad
- ❌ Distribución uniforme en todas las acciones

La **mutación propuesta [URGENTE-1]** resuelve el problema crítico de escalabilidad con:
- ✅ Validación de límites en tiempo de generación
- ✅ Bajo costo de implementación (~30 líneas)
- ✅ Alto impacto (0% → 50% para k=10)

**Recomendación final**: Implementar Fase 1 completa y evaluar antes de continuar con Fase 2.
