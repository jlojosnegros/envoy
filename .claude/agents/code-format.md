# Sub-agente: Code Format Check

## Propósito
Verificar que el código cumple con los estándares de formateo de Envoy usando clang-format y otras herramientas.

## ACCIÓN: EJECUTAR SIEMPRE (tarda 2-5 minutos)
Este comando es rápido. NO usar heurísticos - ejecutar el comando directamente.

## Requiere Docker: SI

## Comando CI Principal
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-format.log
```

## Verificaciones

### 1. C++ Format (clang-format) - ERROR
Verifica que todo el código C++ está formateado según `.clang-format`.

**Archivos afectados:** `*.cc`, `*.h`, `*.cpp`, `*.hpp`

**Si hay errores:**
```
ERROR: Código C++ no formateado correctamente
Archivos afectados: [lista]
Sugerencia: Ejecutar fix automático:
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'
```

### 2. Proto Format - ERROR
Verifica formato de archivos protobuf.

**Archivos afectados:** `*.proto` en `api/`

**Comando específico:**
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh check_proto_format'
```

**Fix:**
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh fix_proto_format'
```

### 3. Python Format - WARNING
Verifica formato de scripts Python.

**Archivos afectados:** `*.py`

### 4. Bash Format - WARNING
Verifica formato de scripts Bash.

**Archivos afectados:** `*.sh`

### 5. BUILD Files Format - WARNING
Verifica formato de archivos Bazel.

**Archivos afectados:** `BUILD`, `*.bzl`, `BUILD.bazel`

## Parsing del Output

El comando `format` produce output indicando archivos con problemas.

**Patrones a buscar:**
- `ERROR:` - Errores de formato
- `would reformat` - Archivos que necesitan reformateo
- `--- a/` y `+++ b/` - Diff de cambios necesarios

## Pre-check Sin Docker

Antes de ejecutar Docker, se puede hacer un check rápido:

```bash
# Verificar si hay cambios de formato pendientes localmente
git diff --name-only | grep -E '\.(cc|h|cpp|hpp)$'
```

## Formato de Salida

```json
{
  "agent": "code-format",
  "requires_docker": true,
  "docker_executed": true|false,
  "findings": [
    {
      "type": "ERROR",
      "check": "clang_format",
      "message": "Archivo no formateado correctamente",
      "location": "source/common/foo.cc",
      "suggestion": "Ejecutar: ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "fix_command": "ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'"
}
```

## Ejecución

1. **Verificar ENVOY_DOCKER_BUILD_DIR** - Si no está definida, no ejecutar y reportar

2. **Crear directorio de logs:**
```bash
mkdir -p ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs
```

3. **Generar timestamp:**
```bash
TIMESTAMP=$(date +%Y%m%d%H%M)
```

4. **Mostrar comando al usuario antes de ejecutar:**
```
Ejecutando verificación de formato...
Comando: ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'
Log: ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-format.log
```

5. **Ejecutar comando**

6. **Parsear output y generar reporte**

## Notas

- El comando `format` puede tardar varios minutos
- Si hay errores, el exit code será != 0
- El log completo estará disponible para debugging
