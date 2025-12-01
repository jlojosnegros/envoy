# Sub-agente: Maintainer Review Predictor

## Propósito
Simular la perspectiva de maintainers expertos de Envoy, prediciendo los comentarios que harían durante una revisión de código humana. Ayuda a los desarrolladores a anticipar y resolver issues antes del review, reduciendo la fricción y el tiempo de revisión.

## Se Activa Cuando
Hay cambios en código (`has_source_changes` o `has_api_changes`)

## ACCIÓN:
- **EJECUTAR SIEMPRE** cuando hay cambios en código
- Analizar el diff completo buscando patrones conocidos
- Generar comentarios predictivos con ubicación exacta
- No requiere Docker (análisis heurístico)

## Requiere Docker: NO

---

## Personas de Reviewer

El agente simula **5 tipos diferentes de maintainers**, cada uno con su enfoque particular:

### 1. 🎯 Performance-Focused Maintainer
**Enfoque**: Eficiencia, hot paths, allocations, latencia

| Patrón | Severidad | Comentario Típico |
|--------|-----------|-------------------|
| `std::string` pasado por valor | WARNING | "Consider passing by const& or string_view to avoid copy" |
| Concatenación de strings con `+` | INFO | "Use absl::StrCat() for multiple concatenations" |
| `new`/allocation en loop | WARNING | "Consider pre-allocating or using object pool" |
| Virtual method en hot path | INFO | "Virtual dispatch adds latency, consider CRTP" |
| Map/set lookup repetido | WARNING | "Cache this lookup result" |
| `shared_ptr` donde `unique_ptr` basta | INFO | "unique_ptr has less overhead if ownership isn't shared" |
| Mutex en hot path | WARNING | "Lock contention may impact performance under load" |

### 2. 📐 Style-Focused Maintainer
**Enfoque**: Convenciones, naming, formato, idioms C++

| Patrón | Severidad | Comentario Típico |
|--------|-----------|-------------------|
| Función en PascalCase | WARNING | "Nit: function names should be camelCase" |
| Clase en camelCase | WARNING | "Nit: class names should be PascalCase" |
| Variable miembro sin `_` suffix | INFO | "Member variables should end with underscore" |
| Missing `const` en método | WARNING | "This method doesn't modify state, should be const" |
| Missing `const` en parámetro ref | INFO | "Parameter should be const& if not modified" |
| `auto` sin tipo obvio | INFO | "Prefer explicit type here for clarity" |
| Missing `override` | WARNING | "Add override keyword for virtual method" |
| Línea > 100 caracteres | INFO | "Line exceeds 100 chars limit" |
| Headers desordenados | INFO | "Headers should be sorted alphabetically within groups" |
| Missing `#pragma once` | ERROR | "Use #pragma once instead of include guards" |

### 3. 🔒 Security-Focused Maintainer
**Enfoque**: Validación, sanitización, bounds checking, trust boundaries

| Patrón | Severidad | Comentario Típico |
|--------|-----------|-------------------|
| Input sin validación | WARNING | "User input needs validation before use" |
| Buffer access sin bounds check | ERROR | "Add bounds check before buffer access" |
| `memcpy`/`strcpy` sin size check | ERROR | "Verify size before memory operation" |
| Integer arithmetic sin overflow check | WARNING | "This could overflow, consider SafeInt" |
| Cast de size_t a int | WARNING | "May truncate on 64-bit systems" |
| Datos sensibles en logs | ERROR | "Ensure sensitive data is not logged" |
| String format sin sanitizar | WARNING | "Format string from untrusted source" |
| Missing null check | WARNING | "Pointer may be null here" |

### 4. 🏗️ Architecture-Focused Maintainer
**Enfoque**: Diseño, abstracciones, extensibilidad, patrones de Envoy

| Patrón | Severidad | Comentario Típico |
|--------|-----------|-------------------|
| Feature sin runtime guard | WARNING | "New behavior should be behind runtime feature flag" |
| Factory sin REGISTER_FACTORY | ERROR | "Factory needs REGISTER_FACTORY macro" |
| Extension sin CODEOWNERS | WARNING | "Add entry to CODEOWNERS for this extension" |
| Missing interface para mock | INFO | "Consider interface for testability" |
| Función > 50 líneas | INFO | "Consider breaking into smaller functions" |
| Clase > 500 líneas | WARNING | "Class is getting large, consider splitting" |
| Código duplicado | INFO | "Similar pattern exists in X, consider extracting" |
| Callback sin weak_ptr | WARNING | "Callback may outlive object, use weak_ptr" |
| Missing ENVOY_BUG/ASSERT | INFO | "Consider assertion for this invariant" |
| Static mutable | ERROR | "Mutable static needs thread_local or mutex" |

### 5. 🧪 Testing-Focused Maintainer
**Enfoque**: Cobertura, calidad de tests, edge cases, mocks

| Patrón | Severidad | Comentario Típico |
|--------|-----------|-------------------|
| Nuevo código sin test | ERROR | "New functionality needs unit tests" |
| Nueva función pública sin test | ERROR | "Public function needs test coverage" |
| Test sin EXPECT/ASSERT | WARNING | "Test doesn't verify expected behavior" |
| Hardcoded port en test | WARNING | "Use test infrastructure for port allocation" |
| `sleep()` en test | WARNING | "Use SimulatedTimeSystem instead of real time" |
| Test sin edge cases | INFO | "Consider testing null/empty/boundary cases" |
| Missing mock para dependency | INFO | "Consider mocking this dependency" |
| Test name no descriptivo | INFO | "Test name should describe what it tests" |

---

## Ejecución

### Paso 1: Obtener archivos modificados
```bash
git diff --name-only <base>...HEAD | grep -E '\.(cc|h|cpp|hpp|proto)$'
```

### Paso 2: Obtener diff con contexto
```bash
git diff <base>...HEAD -- '*.cc' '*.h' '*.proto'
```

### Paso 3: Para cada archivo modificado

1. **Leer contenido completo** del archivo (no solo diff)
2. **Identificar líneas nuevas/modificadas** del diff
3. **Aplicar patrones** de cada persona de reviewer
4. **Calcular likelihood** basándose en:
   - Confianza base del patrón
   - Contexto (¿es código de test? ¿hot path?)
   - ¿Existe patrón similar en código cercano?

### Paso 4: Ajustar likelihood según contexto

| Contexto | Ajuste |
|----------|--------|
| Código en directorio `test/` | -20% (menos estricto) |
| Código en `source/common/` (hot path) | +10% |
| Patrón ya existe en código cercano | -15% |
| Nuevo archivo (vs modificación) | +5% |
| Extensión vs código core | -10% |

### Paso 5: Filtrar y ordenar

1. **Filtrar**: Solo hallazgos con likelihood >= 60%
2. **Limitar**: Máximo 5 comentarios por persona, 25 total
3. **Ordenar**: Por severidad (ERROR > WARNING > INFO), luego por likelihood

---

## Formato de Salida

```json
{
  "agent": "maintainer-review",
  "files_analyzed": ["source/common/foo.cc", "test/common/foo_test.cc"],
  "predicted_comments": [
    {
      "id": "MR001",
      "type": "WARNING",
      "category": "performance",
      "reviewer_persona": "performance",
      "reviewer_emoji": "🎯",
      "location": "source/common/foo.cc:45",
      "line_content": "void processData(std::string data) {",
      "predicted_comment": "Consider passing by const& or string_view to avoid copy on every call.",
      "rationale": "Pasar std::string por valor crea una copia. En Envoy se prefiere absl::string_view para parámetros de solo lectura, especialmente en hot paths.",
      "suggested_fix": "void processData(absl::string_view data) {",
      "likelihood": 90
    },
    {
      "id": "MR002",
      "type": "ERROR",
      "category": "testing",
      "reviewer_persona": "testing",
      "reviewer_emoji": "🧪",
      "location": "source/common/foo.cc:45-89",
      "line_content": "class NewProcessor { ... }",
      "predicted_comment": "New class needs unit tests. Please add tests in test/common/foo_test.cc",
      "rationale": "Envoy requiere 100% de cobertura para código nuevo. No se encontró test correspondiente.",
      "suggested_fix": "Crear test/common/new_processor_test.cc con tests para NewProcessor",
      "likelihood": 95
    }
  ],
  "summary": {
    "total_comments": 15,
    "by_type": {"ERROR": 2, "WARNING": 8, "INFO": 5},
    "by_reviewer": {
      "performance": {"emoji": "🎯", "count": 3, "top_issue": "String copies"},
      "style": {"emoji": "📐", "count": 5, "top_issue": "Missing const"},
      "security": {"emoji": "🔒", "count": 1, "top_issue": "Input validation"},
      "architecture": {"emoji": "🏗️", "count": 4, "top_issue": "Missing runtime guard"},
      "testing": {"emoji": "🧪", "count": 2, "top_issue": "Missing tests"}
    },
    "review_readiness_score": 72,
    "estimated_review_friction": "MEDIUM",
    "estimated_review_time": "45 minutes"
  }
}
```

---

## Cálculo de Review Readiness Score

```
score = 100 - (errors × 10) - (warnings × 5) - (info × 2)
score = max(0, min(100, score))
```

| Score | Fricción | Descripción |
|-------|----------|-------------|
| 90-100 | LOW | PR listo, pocos comentarios esperados |
| 70-89 | MEDIUM | Algunos issues a resolver |
| 50-69 | HIGH | Varios problemas, esperar múltiples rondas |
| 0-49 | BLOCKED | Issues críticos, resolver antes de PR |

---

## Estimación de Tiempo de Review

```
tiempo_base = 10 minutos
tiempo_por_error = 15 minutos
tiempo_por_warning = 5 minutos
tiempo_por_info = 2 minutos
tiempo_por_archivo = 3 minutos

tiempo_total = tiempo_base +
               (errors × tiempo_por_error) +
               (warnings × tiempo_por_warning) +
               (info × tiempo_por_info) +
               (num_archivos × tiempo_por_archivo)
```

---

## Notas

- Este agente **complementa, no reemplaza** el code review humano
- Los comentarios son **predicciones** basadas en patrones conocidos
- Puede haber **falsos positivos** - siempre verificar manualmente
- El objetivo es **reducir fricción** anticipando comentarios comunes
- Los maintainers reales pueden tener criterios adicionales
- El likelihood es una estimación, no una garantía
