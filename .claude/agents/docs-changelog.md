# Sub-agente: Documentation and Changelog Review

## Propósito
Verificar que los cambios de documentación y release notes cumplen con los requisitos de Envoy.

## ACCIÓN: EJECUTAR SIEMPRE si hay cambios en source/ o api/ (comandos git instantáneos, sin Docker)

## Entrada Esperada
- Lista de archivos modificados
- Tipo de cambio detectado (user-facing, API, extension, etc.)

## Verificaciones

### 1. Release Notes Actualizadas (ERROR condicional)

**Cuándo es ERROR:**
- Cambio user-facing (modifica comportamiento visible)
- Cambio en API (archivos en `api/envoy/`)
- Nueva feature o extensión
- Bug fix importante
- Deprecación

**Verificar:**
```bash
git diff --name-only <base>...HEAD | grep -q "changelogs/current.yaml"
```

**Si falta y debería existir:**
```
ERROR: Release notes no actualizadas para cambio user-facing
Ubicación: changelogs/current.yaml
Sugerencia: Añadir entrada en la sección apropiada:
- behavior_changes: Para cambios de comportamiento incompatibles
- minor_behavior_changes: Para cambios menores de comportamiento
- bug_fixes: Para correcciones de bugs
- removed_config_or_runtime: Para configuración eliminada
- new_features: Para nuevas funcionalidades
- deprecated: Para deprecaciones
```

### 2. Formato de Release Notes (WARNING)

Si hay cambios en `changelogs/current.yaml`, verificar:

**Estructura correcta:**
```yaml
- area: subsystem
  change: |
    Descripción del cambio con :ref:`links` apropiados.
    Puede mencionar runtime guard si aplica.
```

**Verificaciones:**
- `area:` presente y válido
- `change:` presente y no vacío
- Links en formato RST `:ref:\`texto <referencia>\``

### 3. Documentación para Cambios de Comportamiento (WARNING)

**Si hay cambios en `source/` que afectan comportamiento:**
- Verificar si hay cambios correspondientes en `docs/`
- Especialmente para nuevas features o cambios de API

```bash
# Verificar si hay cambios en docs
git diff --name-only <base>...HEAD | grep -q "^docs/"
```

### 4. Breaking Changes Documentados (ERROR)

**Si hay deprecaciones:**
- Verificar entrada en `deprecated` section de changelog
- Verificar que se documenta la alternativa

```bash
# Buscar en diff por deprecated
git diff <base>...HEAD | grep -i "deprecated"
```

### 5. Runtime Guard Documentado (WARNING)

**Si hay nuevo runtime guard:**
- Debe estar documentado en release notes
- Debe explicar cómo revertir comportamiento

**Buscar en diff:**
```bash
git diff <base>...HEAD | grep -E "reloadable_features\.|envoy\.reloadable_features"
```

### 6. Gramática y Puntuación (INFO)

**En archivos de documentación modificados:**
- Verificar uso de inglés correcto
- Un espacio después de punto
- Sin typos obvios

**Nota:** Esta es una verificación heurística, no exhaustiva.

## Detección de Cambio User-Facing

Un cambio es user-facing si:
1. Modifica archivos en `api/envoy/` (cambio de API)
2. Añade/modifica extensiones en `source/extensions/`
3. Contiene "runtime guard" o "reloadable_feature"
4. Modifica comportamiento de red/HTTP/routing
5. Añade nuevas estadísticas o métricas
6. Modifica configuración

## Formato de Salida

```json
{
  "agent": "docs-changelog",
  "findings": [
    {
      "type": "ERROR|WARNING|INFO",
      "check": "release_notes",
      "message": "Release notes no actualizadas para cambio user-facing",
      "location": "changelogs/current.yaml",
      "suggestion": "Añadir entrada describiendo: [descripción del cambio]"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "change_classification": {
    "is_user_facing": true|false,
    "is_breaking": true|false,
    "has_runtime_guard": true|false,
    "affected_areas": ["api", "extensions", ...]
  }
}
```

## Ejecución

1. Obtener archivos modificados:
```bash
git diff --name-only <base>...HEAD
```

2. Clasificar tipo de cambio

3. Verificar changelog:
```bash
git diff <base>...HEAD -- changelogs/current.yaml
```

4. Si es user-facing y no hay changelog, ERROR

5. Si hay changelog, verificar formato

6. Buscar runtime guards no documentados

7. Generar reporte
