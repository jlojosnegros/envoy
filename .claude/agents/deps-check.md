# Sub-agente: Dependencies Check

## Propósito
Verificar que los cambios en dependencias cumplen con la política de dependencias de Envoy.

## Se Activa Cuando
Hay cambios en archivos de dependencias:
- `BUILD`
- `*.bzl`
- `bazel/repositories.bzl`
- `bazel/repository_locations.bzl`
- Archivos en `bazel/`

## Requiere Docker: SI

## Comando CI Principal
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh deps' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-deps.log
```

## Verificaciones

### 1. Nueva Dependencia Documentada - ERROR
Si se añade una nueva dependencia, debe estar documentada:

```bash
git diff HEAD~1..HEAD -- 'bazel/repository_locations.bzl' | grep -E '^\+'
```

**Verificar:**
- Justificación de la dependencia
- Licencia compatible
- Mantenimiento activo del proyecto

### 2. Política de Dependencias - ERROR
Según DEPENDENCY_POLICY.md:

- Dependencias deben tener licencia compatible (Apache 2.0, MIT, BSD)
- No dependencias con licencias restrictivas (GPL, LGPL sin excepción)
- Dependencias deben estar activamente mantenidas

### 3. CVE/Vulnerabilidades - WARNING
El comando deps verifica vulnerabilidades conocidas:

**En output buscar:**
- `CVE-` seguido de número
- `vulnerability`
- `security`

### 4. Version Pinning - WARNING
Las dependencias deben estar pinned a versiones específicas:

```bash
git diff HEAD~1..HEAD | grep -E 'version|sha256|commit'
```

### 5. Dependencias Transitivas - INFO
Advertir sobre nuevas dependencias transitivas que se añaden.

## Verificación Rápida Sin Docker

Antes de ejecutar Docker, verificar cambios básicos:

```bash
# Ver qué archivos de dependencias cambiaron
git diff --name-only HEAD~1..HEAD | grep -E '(BUILD|\.bzl|bazel/)'

# Ver nuevas dependencias añadidas
git diff HEAD~1..HEAD -- 'bazel/repository_locations.bzl' | grep -E '^\+.*name.*='
```

## Formato de Salida

```json
{
  "agent": "deps-check",
  "requires_docker": true,
  "docker_executed": true|false,
  "dependency_files_changed": ["bazel/repository_locations.bzl"],
  "findings": [
    {
      "type": "ERROR",
      "check": "new_dependency",
      "message": "Nueva dependencia sin justificación documentada",
      "location": "bazel/repository_locations.bzl:123",
      "suggestion": "Documentar justificación de 'new_lib' en PR description"
    },
    {
      "type": "WARNING",
      "check": "cve_detected",
      "message": "Dependencia con CVE conocida",
      "location": "dependency_name:version",
      "suggestion": "Actualizar a versión sin vulnerabilidad"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "new_dependencies": [
    {
      "name": "new_lib",
      "version": "1.2.3",
      "license": "Apache-2.0"
    }
  ]
}
```

## Ejecución

1. Verificar si hay cambios en archivos de dependencias:
```bash
git diff --name-only HEAD~1..HEAD | grep -E '(BUILD|\.bzl|bazel/)'
```

2. Si no hay cambios, saltar este agente

3. Si hay cambios:
   - Hacer análisis rápido sin Docker
   - Ejecutar comando deps con Docker
   - Parsear output

4. Generar reporte

## Política de Dependencias (Resumen)

Según DEPENDENCY_POLICY.md:

### Licencias Permitidas
- Apache 2.0
- MIT
- BSD (2-clause, 3-clause)
- ISC
- Zlib

### Licencias NO Permitidas
- GPL (cualquier versión)
- LGPL (sin linking exception)
- AGPL
- Propietarias

### Criterios de Calidad
- Proyecto activamente mantenido
- Comunidad razonable
- Sin vulnerabilidades conocidas sin parche
- Builds reproducibles

## Notas

- El comando deps puede tardar varios minutos
- Verifica tanto dependencias directas como transitivas
- Las advertencias de CVE deben investigarse antes del merge
