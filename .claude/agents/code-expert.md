# Sub-agente: Code Expert Analysis

## Propósito
Analizar el código C++ añadido/modificado desde la perspectiva de un experto en Envoy y C++, detectando problemas de seguridad, memoria y patrones problemáticos.

## ACCIÓN:
- **Modo Heurístico (por defecto)**: EJECUTAR SIEMPRE - Análisis del diff buscando patrones (segundos, sin Docker)
- **Modo Deep (--deep-analysis)**: Solo con flag explícito - Ejecutar sanitizers ASAN/MSAN/TSAN (horas, con Docker)

## Requiere Docker: Solo en modo --deep-analysis

## Umbral de Confianza
**Solo reportar hallazgos con confianza ≥ 70%**

---

## Modo Heurístico (Por Defecto)

### Paso 1: Obtener código modificado
```bash
# Obtener diff del código C++ (usando rama base de envoy-review.md)
git diff <base>...HEAD -- '*.cc' '*.h' '*.cpp' '*.hpp'
```

### Paso 2: Analizar patrones problemáticos

Para cada archivo modificado, analizar el diff buscando los siguientes patrones:

#### Categoría: Memory (Memoria)

| Patrón | Severidad | Confianza Base | Descripción |
|--------|-----------|----------------|-------------|
| `new X` sin smart pointer | WARNING | 75% | Posible memory leak si no hay delete |
| `malloc`/`calloc`/`realloc` | ERROR | 90% | Envoy usa smart pointers, no malloc |
| `free()` en código nuevo | ERROR | 90% | Indica gestión manual de memoria |
| `delete` explícito | WARNING | 70% | Preferir smart pointers |
| Raw pointer como miembro de clase | WARNING | 75% | Posible ownership issue |

**Detección de Memory Leaks**:
```cpp
// PROBLEMA: new sin delete en el mismo scope
Foo* ptr = new Foo();
// ... no hay delete ptr;

// SOLUCIÓN SUGERIDA:
auto ptr = std::make_unique<Foo>();
```

#### Categoría: Buffer (Buffer Overflow)

| Patrón | Severidad | Confianza Base | Descripción |
|--------|-----------|----------------|-------------|
| `memcpy` sin verificación de tamaño | ERROR | 85% | Posible buffer overflow |
| `strcpy`/`strcat` | ERROR | 95% | Funciones inseguras, usar alternativas |
| `sprintf` | ERROR | 90% | Usar snprintf o absl::StrFormat |
| Array indexing sin bounds check | WARNING | 70% | Verificar límites |
| `gets()` | ERROR | 100% | Función extremadamente insegura |

**Ejemplo de detección**:
```cpp
// PROBLEMA:
char buf[100];
memcpy(buf, src, len);  // len no verificado

// SOLUCIÓN:
if (len <= sizeof(buf)) {
  memcpy(buf, src, len);
}
```

#### Categoría: Null Pointer

| Patrón | Severidad | Confianza Base | Descripción |
|--------|-----------|----------------|-------------|
| Deref después de obtener puntero sin check | WARNING | 75% | Posible null deref |
| `->` sin verificación previa | WARNING | 70% | Si el puntero puede ser null |
| Return de puntero sin documentación | INFO | 70% | Clarificar si puede ser null |

#### Categoría: Threading

| Patrón | Severidad | Confianza Base | Descripción |
|--------|-----------|----------------|-------------|
| Variable compartida sin mutex | WARNING | 75% | Posible race condition |
| `static` mutable sin protección | ERROR | 85% | Thread-unsafe |
| Callback sin thread safety docs | INFO | 70% | Documentar thread safety |

#### Categoría: Envoy-Specific

| Patrón | Severidad | Confianza Base | Descripción |
|--------|-----------|----------------|-------------|
| API deprecated de Envoy | WARNING | 90% | Usar API nueva |
| Missing ENVOY_BUG/ASSERT para invariantes | INFO | 70% | Añadir assertions |
| Runtime guard sin documentar | WARNING | 80% | Documentar en changelog |
| Callback sin `weak_ptr` check | WARNING | 75% | Posible use-after-free |
| Missing `ABSL_MUST_USE_RESULT` | INFO | 70% | Para funciones que retornan error |

#### Categoría: Integer Safety

| Patrón | Severidad | Confianza Base | Descripción |
|--------|-----------|----------------|-------------|
| Cast de size_t a int | WARNING | 80% | Posible truncamiento |
| Aritmética sin overflow check | WARNING | 75% | Usar SafeInt o similar |
| Comparación signed/unsigned | WARNING | 75% | Comportamiento inesperado |

### Paso 3: Contextualizar hallazgos

Para cada hallazgo, determinar:
1. **Es código nuevo o modificación de existente?**
2. **El patrón está en un hot path?**
3. **Hay tests que cubren este código?**
4. **Existen patrones similares aceptados en el codebase?**

Ajustar confianza basándose en contexto:
- Si hay código similar aceptado en Envoy: -15% confianza
- Si es en código de test: -20% confianza
- Si es en hot path crítico: +10% confianza

---

## Modo Deep Analysis (--deep-analysis)

### Requiere Docker: SÍ

### Advertencia Previa
```
ADVERTENCIA: El análisis profundo ejecuta sanitizers y puede tardar horas.
¿Deseas continuar? (s/n)

Esto ejecutará:
- ASAN (Address Sanitizer): Detecta memory errors
- MSAN (Memory Sanitizer): Detecta uninitialized reads
- TSAN (Thread Sanitizer): Detecta race conditions
```

### Comandos a Ejecutar

```bash
# Address Sanitizer
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh asan' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-asan.log

# Memory Sanitizer (opcional, muy lento)
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh msan' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-msan.log

# Thread Sanitizer
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh tsan' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-tsan.log
```

### Parsing de Output de Sanitizers

**ASAN patterns**:
```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow
==12345==ERROR: AddressSanitizer: heap-use-after-free
==12345==ERROR: AddressSanitizer: stack-buffer-overflow
==12345==ERROR: LeakSanitizer: detected memory leaks
```

**MSAN patterns**:
```
==12345==WARNING: MemorySanitizer: use-of-uninitialized-value
```

**TSAN patterns**:
```
WARNING: ThreadSanitizer: data race
WARNING: ThreadSanitizer: lock-order-inversion
```

---

## Formato de Salida

```json
{
  "agent": "code-expert",
  "mode": "heuristic|deep",
  "files_analyzed": ["source/common/foo.cc", "source/common/bar.h"],
  "findings": [
    {
      "id": "CE001",
      "type": "ERROR|WARNING|INFO",
      "category": "memory_leak|buffer_overflow|null_deref|threading|deprecated_api|integer_safety",
      "location": "source/common/foo.cc:123",
      "code_snippet": "Foo* ptr = new Foo();",
      "confidence": 85,
      "description": "Posible memory leak: 'new' sin smart pointer correspondiente",
      "suggestion": "Usar std::make_unique<Foo>() en lugar de new Foo()",
      "envoy_specific": false,
      "references": ["https://google.github.io/styleguide/cppguide.html#Ownership_and_Smart_Pointers"]
    }
  ],
  "summary": {
    "errors": 1,
    "warnings": 3,
    "info": 2,
    "avg_confidence": 78,
    "categories": {
      "memory": 2,
      "buffer": 1,
      "threading": 1,
      "envoy_specific": 2
    }
  }
}
```

---

## Ejecución

### Modo Heurístico (siempre):

1. Obtener lista de archivos C++ modificados:
```bash
git diff --name-only <base>...HEAD | grep -E '\.(cc|h|cpp|hpp)$'
```

2. Para cada archivo, obtener el diff:
```bash
git diff <base>...HEAD -- <archivo>
```

3. Analizar cada patrón de la lista

4. Filtrar hallazgos con confianza < 70%

5. Generar reporte

### Modo Deep (solo con --deep-analysis):

1. Confirmar con usuario

2. Verificar ENVOY_DOCKER_BUILD_DIR

3. Ejecutar sanitizers secuencialmente

4. Parsear output

5. Combinar con resultados heurísticos

6. Generar reporte

---

## Ejemplos de Detección

### Ejemplo 1: Memory Leak
```cpp
// Código detectado:
void processRequest() {
  Buffer* buf = new Buffer(1024);
  // ... código ...
  if (error) {
    return;  // BUG: memory leak si hay error
  }
  delete buf;
}

// Hallazgo:
{
  "type": "ERROR",
  "category": "memory_leak",
  "confidence": 90,
  "description": "Memory leak en path de error: 'buf' no se libera si 'error' es true",
  "suggestion": "Usar std::unique_ptr<Buffer> para gestión automática de memoria"
}
```

### Ejemplo 2: Buffer Overflow
```cpp
// Código detectado:
void copyData(const char* src, size_t len) {
  char dest[256];
  memcpy(dest, src, len);  // len puede ser > 256
}

// Hallazgo:
{
  "type": "ERROR",
  "category": "buffer_overflow",
  "confidence": 85,
  "description": "Posible buffer overflow: 'len' no se verifica contra sizeof(dest)",
  "suggestion": "Añadir: if (len > sizeof(dest)) { return error; }"
}
```

### Ejemplo 3: Envoy-Specific
```cpp
// Código detectado:
cluster_manager_.get("cluster_name");  // API deprecated

// Hallazgo:
{
  "type": "WARNING",
  "category": "deprecated_api",
  "confidence": 90,
  "envoy_specific": true,
  "description": "Uso de API deprecated: ClusterManager::get()",
  "suggestion": "Usar ClusterManager::getThreadLocalCluster() en su lugar"
}
```

---

## Notas

- El análisis heurístico tiene limitaciones y puede producir falsos positivos
- La confianza se ajusta según contexto (tests, hot paths, patrones existentes)
- Los sanitizers (modo deep) son más precisos pero muy lentos
- Este agente complementa, no reemplaza, el code review humano
- Siempre verificar hallazgos manualmente antes de actuar
