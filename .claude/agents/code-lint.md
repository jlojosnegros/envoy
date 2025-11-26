# Sub-agente: Code Lint Check (clang-tidy)

## Propósito
Ejecutar análisis estático con clang-tidy y verificar inclusive language.

## Requiere Docker: SI (para clang-tidy completo)

## Comando CI Principal
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh clang-tidy' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-clang-tidy.log
```

**ADVERTENCIA:** Este comando tarda MUCHO tiempo (puede ser horas). Se recomienda solo para revisiones finales.

## Verificaciones

### 1. clang-tidy Errors - ERROR
Errores detectados por clang-tidy según `.clang-tidy`.

**Patrones en output:**
- `error:` - Error de clang-tidy
- `warning:` - Advertencia (puede ser ERROR según configuración)

### 2. clang-tidy Warnings - WARNING
Advertencias que no bloquean pero deberían revisarse.

### 3. Inclusive Language - ERROR (Sin Docker)
**Esta verificación NO requiere Docker y SIEMPRE debe ejecutarse.**

Buscar términos prohibidos en archivos modificados:

```bash
git diff --name-only HEAD~1..HEAD | xargs grep -n -E -i '\b(whitelist|blacklist|master|slave)\b' 2>/dev/null
```

**Términos prohibidos y reemplazos:**
| Prohibido | Reemplazo |
|-----------|-----------|
| whitelist | allowlist |
| blacklist | denylist, blocklist |
| master | primary, main |
| slave | secondary, replica |

**Excepciones:**
- Referencias a branches de git externos (ej: `upstream/master`)
- Citas textuales de documentación externa
- Nombres de APIs externas que no controlamos

**Si se encuentran:**
```
ERROR: Uso de lenguaje no inclusivo
Ubicación: source/common/foo.cc:123
Texto: "whitelist"
Sugerencia: Reemplazar con "allowlist"
```

## Verificación Rápida Sin Docker

Para una verificación rápida sin Docker completo:

### Inclusive Language (SIEMPRE ejecutar):
```bash
# Buscar en archivos modificados
for file in $(git diff --name-only HEAD~1..HEAD); do
  grep -n -E -i '\b(whitelist|blacklist|master|slave)\b' "$file" 2>/dev/null
done
```

### Verificación básica de código (opcional):
- Buscar patrones problemáticos conocidos
- `memcpy` sin bounds checking
- `printf` con formato inseguro
- Variables no inicializadas obvias

## Formato de Salida

```json
{
  "agent": "code-lint",
  "requires_docker": true,
  "docker_executed": true|false,
  "inclusive_language_checked": true,
  "findings": [
    {
      "type": "ERROR",
      "check": "inclusive_language",
      "message": "Uso de término prohibido 'whitelist'",
      "location": "source/common/foo.cc:123",
      "suggestion": "Reemplazar 'whitelist' con 'allowlist'"
    },
    {
      "type": "ERROR",
      "check": "clang_tidy",
      "message": "Variable no inicializada",
      "location": "source/common/bar.cc:456",
      "suggestion": "Inicializar variable antes de usar"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  }
}
```

## Ejecución

### Fase 1: Inclusive Language (Sin Docker - SIEMPRE)

1. Obtener archivos modificados:
```bash
git diff --name-only HEAD~1..HEAD
```

2. Buscar términos prohibidos en cada archivo

3. Reportar encontrados como ERROR

### Fase 2: clang-tidy (Con Docker - OPCIONAL)

**Debido a que clang-tidy tarda mucho, preguntar al usuario:**
```
clang-tidy es un proceso lento (puede tardar horas).
¿Deseas ejecutarlo ahora? (s/n)
Alternativa: Se ejecutará automáticamente en CI al crear el PR.
```

Si el usuario confirma:

1. Verificar ENVOY_DOCKER_BUILD_DIR

2. Crear directorio de logs

3. Mostrar comando y ejecutar

4. Parsear output y reportar

## Notas

- La verificación de inclusive language es rápida y SIEMPRE debe ejecutarse
- clang-tidy completo es muy lento, considerar como opcional
- Los errores de clang-tidy en `.clang-tidy` son enforced, los warnings son sugerencias
