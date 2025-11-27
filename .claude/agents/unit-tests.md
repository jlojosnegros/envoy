# Sub-agente: Unit Tests Runner

## Propósito
Ejecutar las test suites unitarias impactadas por el código nuevo/modificado y generar un informe detallado de resultados, incluyendo explicaciones para los tests que fallen.

## ACCIÓN:
- **Por defecto**: EJECUTAR si hay cambios en source/ (requiere Docker)
- **Omitir**: Con flag `--skip-tests`

## Requiere Docker: SÍ

## Timeout: 30 minutos máximo

---

## Flujo de Ejecución

### Paso 1: Identificar archivos modificados

```bash
# Obtener archivos source modificados
git diff --name-only <base>...HEAD | grep '^source/.*\.(cc|h)$'
```

### Paso 2: Determinar tests impactados (Enfoque Híbrido)

#### 2.1 Directorio correspondiente (Rápido)

Para cada archivo `source/path/to/file.cc`:
1. Buscar test directo: `test/path/to/file_test.cc`
2. Buscar tests en directorio: `test/path/to/...`

```bash
# Ejemplo: si se modificó source/common/http/codec.cc
# Buscar:
#   - test/common/http/codec_test.cc
#   - test/common/http/*_test.cc
```

#### 2.2 Bazel query (Completo)

Encontrar todos los tests que dependen de los archivos modificados:

```bash
# Para cada archivo modificado, ejecutar dentro de Docker:
bazel query "rdeps(//test/..., //source/path/to/file.cc)" --output=label 2>/dev/null
```

**Combinar resultados de 2.1 y 2.2, eliminar duplicados.**

### Paso 3: Ejecutar tests

```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh \
  'bazel test --test_timeout=1800 --test_output=errors <lista_de_tests>' \
  2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-unit-tests.log
```

**Opciones de bazel test:**
- `--test_timeout=1800` - Timeout de 30 minutos
- `--test_output=errors` - Solo mostrar output de tests fallidos
- `--test_summary=detailed` - Resumen detallado

### Paso 4: Parsear resultados

#### Patrones a buscar en output:

**Tests pasados:**
```
//test/common/http:utility_test                                          PASSED in X.Xs
```

**Tests fallidos:**
```
//test/common/http:codec_test                                            FAILED in X.Xs
```

**Tests con timeout:**
```
//test/common/http:slow_test                                             TIMEOUT in X.Xs
```

**Resumen final:**
```
Executed N out of M tests: X tests pass, Y fail
```

#### Extraer logs de tests fallidos:

```bash
# Los logs están en bazel-testlogs/<test_path>/test.log
cat bazel-testlogs/test/common/http/codec_test/test.log
```

### Paso 5: Analizar fallos y generar explicaciones

Para cada test fallido:

1. **Leer el log del test** para obtener:
   - Mensaje de error exacto
   - Stack trace
   - Assertion que falló

2. **Leer el diff del código modificado**:
   ```bash
   git diff <base>...HEAD -- <archivo_modificado>
   ```

3. **Leer el código del test que falló**:
   ```bash
   cat test/path/to/failing_test.cc
   ```

4. **Generar explicación** correlacionando:
   - Qué cambió en el código
   - Qué esperaba el test
   - Por qué el cambio puede haber causado el fallo

5. **Sugerir solución** basándose en:
   - El error específico
   - El cambio realizado
   - Las expectativas del test

---

## Formato de Salida

```json
{
  "agent": "unit-tests",
  "execution_time_seconds": 450,
  "timeout_seconds": 1800,
  "tests_discovered": 25,
  "tests_executed": 25,
  "results": {
    "passed": 22,
    "failed": 2,
    "timeout": 1,
    "skipped": 0
  },
  "failures": [
    {
      "test_name": "//test/common/http:codec_test",
      "test_case": "Http2CodecImplTest.BasicRequest",
      "status": "FAILED",
      "error_message": "Value of: response.status()\n  Actual: 500\nExpected: 200",
      "assertion": "EXPECT_EQ(response.status(), 200)",
      "file": "test/common/http/codec_test.cc",
      "line": 234,
      "stack_trace": "test/common/http/codec_test.cc:234\nsource/common/http/codec.cc:89",
      "log_file": "bazel-testlogs/test/common/http/codec_test/test.log",
      "related_changes": [
        "source/common/http/codec.cc:45-67"
      ],
      "possible_explanation": "El cambio en codec.cc línea 52 modificó el handling del header ':status'. La nueva lógica retorna 500 cuando el header está vacío, pero el test espera que se use el valor por defecto 200.",
      "suggestion": "Revisar la condición en línea 52. El test espera comportamiento backward-compatible. Considerar: if (status.empty()) { status = '200'; }"
    }
  ],
  "passed_tests": [
    "//test/common/http:utility_test",
    "//test/common/http:header_map_test",
    "//test/common/http:parser_test"
  ],
  "summary": {
    "status": "FAILED",
    "pass_rate": 88.0,
    "duration": "7m 30s",
    "message": "2 tests fallaron, 1 timeout. Ver detalles arriba."
  }
}
```

---

## Integración con Reporte Final

### Sección en reporte:

```markdown
## Unit Tests

**Ejecutados**: 25 tests
**Duración**: 7m 30s
**Resultado**: ❌ 2 FALLIDOS, 1 TIMEOUT

### Tests Fallidos

#### [UT001] //test/common/http:codec_test - Http2CodecImplTest.BasicRequest

- **Error**: `Expected: 200, Actual: 500`
- **Ubicación**: test/common/http/codec_test.cc:234
- **Cambio relacionado**: source/common/http/codec.cc:45-67
- **Explicación**: El cambio en codec.cc línea 52 modificó el handling del header ':status'. La nueva lógica retorna 500 cuando el header está vacío, pero el test espera que se use el valor por defecto 200.
- **Sugerencia**: Revisar la condición en línea 52. Considerar mantener comportamiento backward-compatible.

### Tests Pasados (22)

<details>
<summary>Ver lista completa</summary>

- //test/common/http:utility_test
- //test/common/http:header_map_test
- //test/common/http:parser_test
...
</details>
```

---

## Comandos Docker

### Ejecución de tests:
```bash
bash -c 'TIMESTAMP=$(date +%Y%m%d%H%M) && ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh "bazel test --test_timeout=1800 --test_output=errors <tests>" 2>&1 | tee <dir>/review-agent-logs/${TIMESTAMP}-unit-tests.log'
```

### Bazel query para tests impactados:
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel query "rdeps(//test/..., //source/path/to/file.cc)" --output=label'
```

---

## Manejo de Casos Especiales

### Sin tests impactados
Si no se encuentran tests relacionados:
```json
{
  "agent": "unit-tests",
  "tests_discovered": 0,
  "message": "No se encontraron tests relacionados con los archivos modificados",
  "suggestion": "Considerar añadir tests para el nuevo código en test/path/to/"
}
```

### Timeout global
Si se excede el timeout de 30 minutos:
```json
{
  "status": "TIMEOUT",
  "message": "La ejecución de tests excedió el límite de 30 minutos",
  "tests_completed": 15,
  "tests_pending": 10
}
```

### Error de compilación
Si los tests no compilan:
```json
{
  "status": "BUILD_FAILED",
  "error": "Error de compilación en //test/common/http:codec_test",
  "build_log": "...",
  "suggestion": "Corregir errores de compilación antes de ejecutar tests"
}
```

---

## Notas

- Los tests se ejecutan dentro del contenedor Docker de Envoy
- El timeout de 30 minutos es para la ejecución total, no por test individual
- bazel test cachea resultados, solo re-ejecuta tests afectados
- Las explicaciones de fallos son generadas por el agente analizando código, no son infalibles
- Si hay muchos tests impactados (>50), considerar ejecutar solo los más relevantes
- Los logs completos siempre se guardan en `review-agent-logs/`
