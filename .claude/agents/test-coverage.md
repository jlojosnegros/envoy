# Sub-agente: Test Coverage Analysis

## Propósito
Verificar que el código nuevo tiene cobertura de tests adecuada.

## Modos de Operación

### Modo SEMI (por defecto) - Sin Docker
Análisis heurístico para estimar cobertura sin ejecutar build completo.

### Modo FULL (--coverage-full) - Con Docker
Ejecuta build de coverage real para verificación precisa.

---

## MODO SEMI (Heurístico)

### Objetivo
Estimar con alta probabilidad si el código nuevo está cubierto por tests.

### Verificaciones

#### 1. Matching de Archivos Test
Para cada archivo `.cc` modificado, buscar archivo de test correspondiente:

```bash
# Para source/common/foo/bar.cc buscar:
# - test/common/foo/bar_test.cc
# - test/common/foo_test.cc

FILE="source/common/foo/bar.cc"
TEST_DIR=$(echo $FILE | sed 's/^source/test/')
TEST_FILE=$(echo $TEST_DIR | sed 's/\.cc$/_test.cc/')
```

**Si no existe test file:**
```
WARNING: No se encontró archivo de test para source/common/foo/bar.cc
Esperado: test/common/foo/bar_test.cc
```

#### 2. Análisis de Funciones Nuevas

Identificar funciones/métodos nuevos en el diff:

```bash
git diff HEAD~1..HEAD -- '*.cc' '*.h' | grep -E '^\+.*\b(void|bool|int|string|Status)\s+\w+\s*\('
```

Para cada función nueva, verificar si existe test que la referencie:

```bash
grep -r "functionName" test/
```

#### 3. Verificación de Tests Añadidos

Si hay archivos nuevos en `source/`, debe haber archivos nuevos en `test/`:

```bash
git diff --name-only --diff-filter=A HEAD~1..HEAD | grep '^source/'
git diff --name-only --diff-filter=A HEAD~1..HEAD | grep '^test/'
```

#### 4. Análisis de Branches de Código

Buscar nuevos `if`/`else`/`switch` y verificar cobertura:
- ¿Hay tests para el caso positivo?
- ¿Hay tests para el caso negativo?
- ¿Hay tests para edge cases?

### Cálculo de Confianza

```
Confianza Base: 50%
+ 15% si existe archivo de test correspondiente
+ 15% si se modificó/añadió el archivo de test
+ 10% si se encuentran referencias a funciones nuevas en tests
+ 10% si ratio nuevos_tests/nuevo_codigo > 0.5
= Confianza Final (max 100%)
```

### Formato de Salida Modo SEMI

```json
{
  "agent": "test-coverage",
  "mode": "semi",
  "confidence_percentage": 75,
  "findings": [
    {
      "type": "WARNING",
      "check": "missing_test_file",
      "message": "No se encontró archivo de test",
      "location": "source/common/foo.cc",
      "suggestion": "Crear test/common/foo_test.cc"
    },
    {
      "type": "INFO",
      "check": "uncovered_function",
      "message": "Función potencialmente no testeada",
      "location": "source/common/foo.cc:123 - processRequest()",
      "suggestion": "Añadir test para processRequest()"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "coverage_estimate": {
    "new_source_files": 3,
    "matching_test_files": 2,
    "new_functions": 5,
    "tested_functions": 3
  }
}
```

---

## MODO FULL (Build de Coverage)

### Requiere Docker: SI

### Advertencia Previa
```
ADVERTENCIA: El build de coverage es un proceso MUY lento (puede tardar horas).
¿Estás seguro de que deseas ejecutarlo? (s/n)

Alternativa: Puedes usar el comando /coverage en tu PR de GitHub
para obtener un reporte de coverage sin ejecutar localmente.
```

### Comando CI
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh coverage' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-coverage.log
```

### Análisis del Reporte

Después del build, analizar el reporte de coverage para los archivos modificados:

1. Ubicar reporte de coverage generado
2. Filtrar por archivos en el diff
3. Verificar % de cobertura en código nuevo
4. Identificar líneas no cubiertas

### Criterio de Envoy
- Se espera 100% de cobertura en código nuevo
- Excepciones deben estar justificadas en el PR

### Formato de Salida Modo FULL

```json
{
  "agent": "test-coverage",
  "mode": "full",
  "docker_executed": true,
  "findings": [
    {
      "type": "ERROR",
      "check": "coverage_below_100",
      "message": "Cobertura de código nuevo: 85%",
      "location": "source/common/foo.cc",
      "suggestion": "Añadir tests para líneas: 45, 67, 89-92",
      "uncovered_lines": [45, 67, 89, 90, 91, 92]
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "coverage_data": {
    "new_lines": 150,
    "covered_lines": 127,
    "percentage": 84.67
  }
}
```

---

## Ejecución

### Para Modo SEMI:

1. Obtener lista de archivos modificados en `source/`:
```bash
git diff --name-only HEAD~1..HEAD | grep '^source/.*\.cc$'
```

2. Para cada archivo, buscar test correspondiente

3. Analizar funciones nuevas en diff

4. Calcular confianza

5. Generar reporte

### Para Modo FULL:

1. Mostrar advertencia y confirmar con usuario

2. Verificar ENVOY_DOCKER_BUILD_DIR

3. Ejecutar build de coverage

4. Analizar reporte

5. Generar reporte con líneas no cubiertas

## Notas

- El modo SEMI es suficiente para la mayoría de PRs
- El modo FULL debería usarse solo cuando hay dudas o para PRs críticos
- La CI de GitHub ejecutará coverage automáticamente
