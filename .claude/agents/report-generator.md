# Sub-agente: Report Generator

## Propósito
Consolidar todos los reportes de los sub-agentes en un reporte final unificado.

## Entrada
Resultados de todos los sub-agentes ejecutados en formato JSON.

## Formato de Reporte Final

```markdown
# Envoy PR Pre-Review Report

**Generado**: YYYY-MM-DD HH:MM:SS
**Branch**: [nombre del branch actual]
**Base commit**: [SHA del commit base]
**Head commit**: [SHA del commit actual]
**Commits analizados**: [número de commits]

---

## Resumen Ejecutivo

| Categoría | Errores | Warnings | Info |
|-----------|:-------:|:--------:|:----:|
| PR Metadata | X | Y | Z |
| Dev Environment | X | Y | Z |
| Code Format | X | Y | Z |
| Code Lint | X | Y | Z |
| Code Expert | X | Y | Z |
| Security Audit | X | Y | Z |
| Unit Tests | X | Y | Z |
| Test Coverage | X | Y | Z |
| Docs/Changelog | X | Y | Z |
| API Review | X | Y | Z |
| Dependencies | X | Y | Z |
| Extensions | X | Y | Z |
| Maintainer Review | X | Y | Z |
| **TOTAL** | **X** | **Y** | **Z** |

### Estado General

[EMOJI] **[ESTADO]**

Donde:
- 🔴 **BLOCKED** - Hay errores críticos que deben corregirse
- 🟡 **NEEDS_WORK** - Hay warnings que deberían revisarse
- 🟢 **READY** - No hay errores ni warnings significativos

**Review Readiness Score**: [score]/100

---

## 👥 Predicted Reviewer Comments

Basado en patrones de reviews anteriores de Envoy, estos son los comentarios
que probablemente recibirías de diferentes tipos de maintainers:

### 🎯 Performance-Focused Reviewer ([N] comentarios)
| Archivo:Línea | Comentario | Sugerencia |
|---------------|------------|------------|
| [ubicación] | [comentario predicho] | [fix sugerido] |

### 📐 Style-Focused Reviewer ([N] comentarios)
| Archivo:Línea | Comentario | Sugerencia |
|---------------|------------|------------|
| [ubicación] | [comentario predicho] | [fix sugerido] |

### 🔒 Security-Focused Reviewer ([N] comentarios)
| Archivo:Línea | Comentario | Sugerencia |
|---------------|------------|------------|
| [ubicación] | [comentario predicho] | [fix sugerido] |

### 🏗️ Architecture-Focused Reviewer ([N] comentarios)
| Archivo:Línea | Comentario | Sugerencia |
|---------------|------------|------------|
| [ubicación] | [comentario predicho] | [fix sugerido] |

### 🧪 Testing-Focused Reviewer ([N] comentarios)
| Archivo:Línea | Comentario | Sugerencia |
|---------------|------------|------------|
| [ubicación] | [comentario predicho] | [fix sugerido] |

**Tiempo estimado de review**: ~[X] minutos

---

## Archivos Analizados

```
[Lista de archivos modificados agrupados por tipo]

Source files (N):
  - source/common/foo.cc
  - source/common/bar.cc

Test files (N):
  - test/common/foo_test.cc

API files (N):
  - api/envoy/config/foo.proto

Documentation (N):
  - docs/root/intro/foo.rst
  - changelogs/current.yaml
```

---

## Hallazgos Detallados

### 🔴 Errores (Deben corregirse)

Estos problemas BLOQUEAN el merge del PR:

#### [E001] [Categoría] Título del error
- **Ubicación**: `archivo:línea`
- **Descripción**: Descripción detallada del problema
- **Sugerencia**: Cómo solucionarlo

```
[Código o diff relevante si aplica]
```

---

#### [E002] ...

---

### 🟡 Warnings (Deberían corregirse)

Estos problemas no bloquean pero deberían revisarse:

#### [W001] [Categoría] Título del warning
- **Ubicación**: `archivo:línea`
- **Descripción**: Descripción del problema
- **Sugerencia**: Cómo solucionarlo

---

### 🔵 Info (Mejoras opcionales)

Sugerencias de mejora que no son obligatorias:

#### [I001] [Categoría] Título
- **Ubicación**: `archivo:línea`
- **Descripción**: Descripción
- **Sugerencia**: Sugerencia

---

## Comandos de Corrección

### Correcciones Automáticas

```bash
# Formateo de código C++
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'

# Formateo de protos
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh fix_proto_format'

# Añadir DCO sign-off a commits
git commit --amend -s
# Para múltiples commits:
git rebase -i HEAD~N  # y añadir -s a cada uno

# Ejecutar tests específicos localmente
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel test //test/path/to:test'

# Verificar dependencias
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel run //tools/dependency:validate'
```

### Template para Release Notes

Si faltan release notes, añadir en `changelogs/current.yaml`:
```yaml
# Para nueva feature:
new_features:
- area: <subsystem>
  change: |
    Added <descripción de la feature>.

# Para bug fix:
bug_fixes:
- area: <subsystem>
  change: |
    Fixed <descripción del bug>.

# Para cambio de comportamiento:
behavior_changes:
- area: <subsystem>
  change: |
    Changed <descripción del cambio>. This can be reverted by setting
    runtime guard ``envoy.reloadable_features.<flag>`` to false.
```

### Correcciones Manuales Requeridas

1. [ ] Corregir [E001]: [descripción breve]
2. [ ] Corregir [E002]: [descripción breve]
3. [ ] Revisar [W001]: [descripción breve]

---

## Verificaciones No Ejecutadas

[Si algún sub-agente no se ejecutó, listarlo aquí con la razón]

| Check | Razón | Cómo ejecutar |
|-------|-------|---------------|
| clang-tidy | Requiere --full-lint | `/envoy-review --full-lint` |
| coverage (full) | Requiere --coverage-full | `/envoy-review --coverage-full` |
| deep-analysis | Requiere --deep-analysis | `/envoy-review --deep-analysis` |
| unit-tests | Omitido con --skip-tests | `/envoy-review` (por defecto) |
| security-deps | Sin cambios en dependencias | Automático si hay cambios en bazel/ |

---

## Próximos Pasos

1. [ ] Corregir todos los errores listados arriba
2. [ ] Revisar y corregir warnings aplicables
3. [ ] Ejecutar tests localmente: `bazel test //test/...`
4. [ ] Verificar que CI pasa
5. [ ] Crear/actualizar PR

---

## Información Adicional

### Unit Tests
| Métrica | Valor |
|---------|-------|
| Tests ejecutados | X |
| Pasados | X |
| Fallidos | X |
| Timeout | X |
| Duración | Xm Xs |

### Code Expert Analysis
| Métrica | Valor |
|---------|-------|
| Archivos analizados | X |
| Confianza promedio | X% |
| Categorías detectadas | memory, buffer, threading |

### Security Audit
| Métrica | Valor |
|---------|-------|
| Dependencias verificadas | X |
| CVEs encontrados | X |
| Severidad máxima | critical/high/medium/low |

### Coverage Estimado (Modo Semi)
- **Confianza**: X%
- **Archivos sin test aparente**: [lista]

### Maintainer Review
| Métrica | Valor |
|---------|-------|
| Comentarios predichos | X |
| Review Readiness Score | X/100 |
| Tiempo estimado de review | X minutos |
| Por reviewer: Performance | X |
| Por reviewer: Style | X |
| Por reviewer: Security | X |
| Por reviewer: Architecture | X |
| Por reviewer: Testing | X |

### Logs de Ejecución
Los logs detallados están en:
```
${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/
├── YYYYMMDDHHMM-format.log
├── YYYYMMDDHHMM-unit-tests.log
├── YYYYMMDDHHMM-security-deps.log
└── ...
```

---

*Reporte generado por Envoy PR Pre-Review Agent*
*Para más información: /envoy-review --help*
```

## Lógica de Generación

### 1. Determinar Estado General

```python
def determine_status(findings):
    total_errors = sum(f['errors'] for f in findings)
    total_warnings = sum(f['warnings'] for f in findings)

    if total_errors > 0:
        return "BLOCKED", "🔴"
    elif total_warnings > 0:
        return "NEEDS_WORK", "🟡"
    else:
        return "READY", "🟢"
```

### 2. Asignar IDs a Hallazgos

```
Errores: E001, E002, E003, ...
Warnings: W001, W002, W003, ...
Info: I001, I002, I003, ...
```

### 3. Agrupar por Categoría

Agrupar hallazgos por el agente que los generó:
- pr-metadata → "PR Metadata"
- dev-env → "Dev Environment"
- code-format → "Code Format"
- code-lint → "Code Lint"
- code-expert → "Code Expert"
- security-audit → "Security Audit"
- unit-tests → "Unit Tests"
- test-coverage → "Test Coverage"
- docs-changelog → "Docs/Changelog"
- api-review → "API Review"
- deps-check → "Dependencies"
- extension-review → "Extensions"
- maintainer-review → "Maintainer Review"

### 4. Generar Comandos de Fix

Incluir comandos específicos basados en errores encontrados:
- Si hay errores de formato → comando format
- Si hay errores de proto format → comando fix_proto_format
- Si faltan release notes → template de entrada

## Guardado del Reporte

Si --save-report está activo:

```bash
REPORT_FILE="${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-report.md"
```

También mostrar en consola un resumen corto:

```
════════════════════════════════════════════════════════════════
                    ENVOY PR PRE-REVIEW SUMMARY
════════════════════════════════════════════════════════════════

Status: 🟡 NEEDS_WORK

Errors:   2  (must fix before PR)
Warnings: 5  (should review)
Info:     3  (optional improvements)

Top Issues:
  [E001] DCO sign-off missing in commit abc1234
  [E002] Release notes not updated for user-facing change
  [W001] clang-format: 3 files need formatting

Full report: ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-report.md

To fix formatting automatically:
  ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'

════════════════════════════════════════════════════════════════
```

## Notas

- El reporte siempre se muestra en consola (resumen)
- El reporte completo se guarda en archivo si --save-report
- Usar colores/emojis para mejor legibilidad en terminal
- IDs permiten referencia fácil en discusiones
