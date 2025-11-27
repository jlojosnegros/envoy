# Sub-agente: Security Audit

## Propósito
Auditar la seguridad del código y dependencias, detectando:
- Dependencias con versiones deprecated
- Dependencias con CVEs abiertos
- Código con vulnerabilidades de seguridad conocidas

## ACCIÓN:
- **Fase 1 (Sin Docker)**: EJECUTAR SIEMPRE - Consultar APIs externas de CVE
- **Fase 2 (Con Docker)**: EJECUTAR si hay cambios en dependencias - `//tools/dependency:validate`

## Requiere Docker: Solo Fase 2

## Umbral de Confianza
**Solo reportar hallazgos con confianza ≥ 70%**

---

## Fase 1: Análisis de Dependencias (Sin Docker)

### Paso 1: Identificar dependencias del proyecto

```bash
# Obtener lista de dependencias de Envoy
cat bazel/repository_locations.bzl | grep -E '(name|version|sha256)' | head -100
```

### Paso 2: Identificar dependencias modificadas

```bash
# Ver si hay cambios en archivos de dependencias
git diff --name-only <base>...HEAD | grep -E '(repository_locations|repositories)\.bzl'
```

### Paso 3: Consultar APIs de CVE

Para cada dependencia (especialmente las modificadas), consultar:

#### 3.1 OSV (Open Source Vulnerabilities) - Preferido
```
URL: https://api.osv.dev/v1/query
Método: POST
Body: {"package": {"name": "<nombre>", "ecosystem": "<ecosystem>"}, "version": "<version>"}
```

#### 3.2 GitHub Advisory Database
```
URL: https://api.github.com/advisories
Query: ?ecosystem=<ecosystem>&package=<nombre>
```

#### 3.3 NVD (National Vulnerability Database)
```
URL: https://services.nvd.nist.gov/rest/json/cves/2.0
Query: ?keywordSearch=<nombre>
```

### Paso 4: Fallback a herramientas locales

Si las APIs externas no están disponibles:
```bash
# Usar herramientas de Envoy
cat bazel/repository_locations.bzl | grep -A5 "cve_"
```

---

## Fase 2: Verificación con Docker

### Requiere Docker: SÍ

### Se Activa Cuando
- Hay cambios en `bazel/repository_locations.bzl`
- Hay cambios en `bazel/repositories.bzl`
- Hay cambios en archivos BUILD que añaden dependencias

### Comando CI
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel run //tools/dependency:validate' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-security-deps.log
```

### Parsing de Output

Buscar en output:
- `CVE-` seguido de número
- `vulnerability`
- `deprecated`
- `outdated`
- `FAIL` o `ERROR`

---

## Categorías de Hallazgos

### CVE (Common Vulnerabilities and Exposures)

| Severidad CVSS | Tipo en Reporte | Descripción |
|----------------|-----------------|-------------|
| 9.0 - 10.0 | ERROR (Critical) | Vulnerabilidad crítica, acción inmediata |
| 7.0 - 8.9 | ERROR (High) | Vulnerabilidad alta, acción prioritaria |
| 4.0 - 6.9 | WARNING (Medium) | Vulnerabilidad media, planificar fix |
| 0.1 - 3.9 | INFO (Low) | Vulnerabilidad baja, evaluar |

### Deprecated Dependencies

| Situación | Tipo | Confianza |
|-----------|------|-----------|
| Versión EOL (End of Life) | ERROR | 95% |
| Versión sin soporte activo | WARNING | 85% |
| Nueva versión major disponible | INFO | 75% |

### Código Inseguro

| Patrón | Tipo | Confianza |
|--------|------|-----------|
| Uso de crypto deprecated (MD5, SHA1 para seguridad) | ERROR | 90% |
| Hardcoded secrets/credentials | ERROR | 95% |
| Insecure random (rand() para crypto) | ERROR | 90% |
| HTTP en lugar de HTTPS para recursos | WARNING | 80% |

---

## Dependencias Principales de Envoy a Monitorear

| Dependencia | Ecosystem | Criticidad |
|-------------|-----------|------------|
| boringssl | C++ | CRÍTICA |
| nghttp2 | C++ | ALTA |
| libevent | C++ | ALTA |
| protobuf | C++ | ALTA |
| abseil-cpp | C++ | MEDIA |
| grpc | C++ | ALTA |
| yaml-cpp | C++ | MEDIA |
| zlib | C++ | MEDIA |
| curl | C++ | ALTA |

---

## Formato de Salida

```json
{
  "agent": "security-audit",
  "phase1_executed": true,
  "phase2_executed": true|false,
  "api_sources_used": ["osv", "github", "nvd"],
  "dependencies_checked": 45,
  "findings": [
    {
      "id": "SA001",
      "type": "ERROR|WARNING|INFO",
      "severity": "critical|high|medium|low",
      "category": "cve|deprecated|insecure_code|outdated",
      "location": "boringssl:1.0.0 | source/common/crypto.cc:45",
      "confidence": 95,
      "description": "CVE-2024-1234: Buffer overflow en BoringSSL < 1.1.0",
      "source": "https://nvd.nist.gov/vuln/detail/CVE-2024-1234",
      "cvss_score": 8.5,
      "affected_versions": "< 1.1.0",
      "current_version": "1.0.0",
      "fixed_version": "1.1.0",
      "suggestion": "Actualizar boringssl a versión 1.1.0 o superior",
      "exploitability": "Requires network access",
      "patch_available": true
    }
  ],
  "summary": {
    "critical": 0,
    "high": 1,
    "medium": 2,
    "low": 3,
    "total_cves": 6,
    "deprecated_deps": 1,
    "outdated_deps": 4
  }
}
```

---

## Ejecución

### Fase 1 (Sin Docker - Siempre):

1. Leer `bazel/repository_locations.bzl` para obtener dependencias:
```bash
cat bazel/repository_locations.bzl
```

2. Para cada dependencia crítica, consultar OSV:
```
Consulta: https://api.osv.dev/v1/query con package y version
```

3. Si OSV no disponible, usar GitHub Advisory API

4. Si GitHub no disponible, usar NVD

5. Si ninguna API disponible, marcar como "APIs no disponibles" y continuar con Fase 2

6. Filtrar hallazgos con confianza < 70%

### Fase 2 (Con Docker - Si hay cambios en deps):

1. Verificar si hay cambios en dependencias:
```bash
git diff --name-only <base>...HEAD | grep -E 'repository_locations|repositories'
```

2. Si hay cambios, ejecutar:
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel run //tools/dependency:validate'
```

3. Parsear output para CVEs y warnings

4. Combinar con resultados de Fase 1

5. Generar reporte consolidado

---

## Detección de Código Inseguro

### Patrones a buscar en el diff:

```bash
# Crypto inseguro
git diff <base>...HEAD | grep -E '(MD5|SHA1|DES|RC4)' | grep -v test

# Hardcoded secrets
git diff <base>...HEAD | grep -iE '(password|secret|api_key|token)\s*=\s*["\047]'

# Random inseguro
git diff <base>...HEAD | grep -E '\brand\(\)|\bsrand\(' | grep -v test

# HTTP inseguro
git diff <base>...HEAD | grep -E 'http://' | grep -v '(localhost|127\.0\.0\.1|example\.com)'
```

---

## Ejemplos de Hallazgos

### Ejemplo 1: CVE en Dependencia
```json
{
  "id": "SA001",
  "type": "ERROR",
  "severity": "high",
  "category": "cve",
  "location": "nghttp2:1.43.0",
  "confidence": 98,
  "description": "CVE-2023-44487: HTTP/2 Rapid Reset Attack",
  "source": "https://nvd.nist.gov/vuln/detail/CVE-2023-44487",
  "cvss_score": 7.5,
  "affected_versions": "< 1.57.0",
  "current_version": "1.43.0",
  "fixed_version": "1.57.0",
  "suggestion": "Actualizar nghttp2 a versión 1.57.0 en bazel/repository_locations.bzl"
}
```

### Ejemplo 2: Dependencia Deprecated
```json
{
  "id": "SA002",
  "type": "WARNING",
  "severity": "medium",
  "category": "deprecated",
  "location": "old-library:2.0.0",
  "confidence": 90,
  "description": "Dependencia 'old-library' está en EOL desde 2023-01",
  "source": "https://github.com/old-library/old-library/releases",
  "suggestion": "Migrar a 'new-library' o actualizar a rama con soporte"
}
```

### Ejemplo 3: Código Inseguro
```json
{
  "id": "SA003",
  "type": "ERROR",
  "severity": "high",
  "category": "insecure_code",
  "location": "source/common/auth.cc:78",
  "confidence": 85,
  "description": "Uso de MD5 para hashing de credenciales",
  "code_snippet": "std::string hash = MD5::hash(password);",
  "suggestion": "Usar SHA-256 o bcrypt para hashing de credenciales"
}
```

---

## APIs de CVE - Detalles

### OSV API (Preferida)
```bash
curl -X POST https://api.osv.dev/v1/query \
  -H "Content-Type: application/json" \
  -d '{"package":{"name":"nghttp2","ecosystem":"C++"},"version":"1.43.0"}'
```

**Response contiene:**
- `vulns[]`: Lista de vulnerabilidades
- `vulns[].id`: ID de CVE
- `vulns[].summary`: Descripción
- `vulns[].severity[]`: Severidad CVSS
- `vulns[].affected[]`: Versiones afectadas

### GitHub Advisory API
```bash
curl -H "Accept: application/vnd.github+json" \
  "https://api.github.com/advisories?affects=nghttp2"
```

### Fallback: Búsqueda manual
Si las APIs no funcionan, buscar en:
- https://cve.mitre.org/cgi-bin/cvekey.cgi?keyword=<dependencia>
- https://security.snyk.io/package/

---

## Notas

- Las APIs externas pueden tener rate limits
- Cachear resultados de CVE para evitar consultas repetidas
- Los CVEs de severidad crítica/alta siempre deben reportarse
- Verificar que la versión específica está afectada, no solo el paquete
- Este agente complementa, no reemplaza, auditorías de seguridad profesionales
- Actualizar la lista de dependencias críticas según evolucione Envoy
