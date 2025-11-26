# Sub-agente: Development Environment Check

## Propósito
Verificar que el entorno de desarrollo está correctamente configurado según las instrucciones de Envoy.

## ACCIÓN: EJECUTAR SIEMPRE (comandos ls/cat instantáneos, sin Docker)

## Verificaciones

### 1. Git Hooks Instalados (ERROR)
Verificar que existen los hooks de git instalados por `./support/bootstrap`:

**Archivos a verificar:**
- `.git/hooks/pre-commit` - Debe existir y ser ejecutable
- `.git/hooks/pre-push` - Debe existir y ser ejecutable
- `.git/hooks/commit-msg` - Debe existir y ser ejecutable

**Comando:**
```bash
ls -la .git/hooks/pre-commit .git/hooks/pre-push .git/hooks/commit-msg 2>/dev/null
```

**Si no existen:**
```
ERROR: Git hooks no instalados.
Sugerencia: Ejecutar ./support/bootstrap desde la raíz del proyecto
```

### 2. Bootstrap Ejecutado (WARNING)
Verificar indicadores de que bootstrap fue ejecutado:
- Los hooks existen
- Son symlinks o copias de `support/hooks/`

**Verificar contenido:**
```bash
head -5 .git/hooks/pre-commit
```
Debería contener referencia a scripts de Envoy.

### 3. Archivo .env (INFO)
Si existe `.env`, verificar si contiene `NO_VERIFY=1`:

```bash
grep -q "NO_VERIFY" .env 2>/dev/null && echo "WARNING"
```

**Si NO_VERIFY está activo:**
```
INFO: NO_VERIFY está configurado en .env
Esto desactiva las verificaciones de pre-commit/pre-push.
Asegúrate de ejecutar las verificaciones manualmente antes del PR.
```

### 4. Herramientas de Desarrollo (INFO)
Verificaciones opcionales de herramientas comunes:
- clang-format disponible
- bazel disponible

**Nota:** Estas no son errores porque los comandos Docker incluyen las herramientas.

## Formato de Salida

```json
{
  "agent": "dev-env",
  "findings": [
    {
      "type": "ERROR|WARNING|INFO",
      "check": "git_hooks",
      "message": "Git hooks no instalados",
      "location": ".git/hooks/",
      "suggestion": "Ejecutar: ./support/bootstrap"
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

1. Verificar existencia de hooks:
```bash
for hook in pre-commit pre-push commit-msg; do
  if [ -f ".git/hooks/$hook" ]; then
    echo "$hook: OK"
  else
    echo "$hook: MISSING"
  fi
done
```

2. Verificar .env:
```bash
if [ -f ".env" ]; then
  cat .env
fi
```

3. Generar reporte
