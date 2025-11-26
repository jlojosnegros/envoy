# Sub-agente: PR Metadata Review

## Propósito
Verificar que los commits y la metadata del PR cumplen con los requisitos de Envoy.

## ACCIÓN: EJECUTAR SIEMPRE (comandos git instantáneos, sin Docker)

## Entrada Esperada
- Lista de commits a analizar (desde git log)
- Archivos modificados (para calcular LOC)

## Verificaciones

### 1. Formato del Título del Commit (ERROR)
El título debe seguir el formato: `subsystem: descripción`
- Todo en minúsculas (excepto acrónimos conocidos como HTTP, TLS, etc.)
- Subsistema seguido de dos puntos y espacio
- Descripción breve y descriptiva

**Ejemplos válidos:**
- `docs: fix grammar error`
- `http conn man: add new feature`
- `router: add x-envoy-overloaded header`
- `tls: add support for specifying TLS session ticket keys`

**Patrón regex:** `^[a-z][a-z0-9 /_-]*: .+$`

### 2. DCO Sign-off (ERROR)
Cada commit DEBE tener la línea:
```
Signed-off-by: Nombre <email@ejemplo.com>
```

**Comando para verificar:**
```bash
git log --format='%B' HEAD~N..HEAD | grep -c "Signed-off-by:"
```

Si falta, proporcionar instrucciones:
```
Para añadir DCO a commits existentes:
git commit --amend -s
# o para múltiples commits:
git rebase -i HEAD~N
# y añadir -s en cada commit
```

### 3. Commit Message (ERROR si vacío)
El mensaje del commit debe:
- Explicar QUÉ hace el cambio y POR QUÉ
- No estar vacío
- Tener gramática y puntuación correctas en inglés

### 4. Risk Level (WARNING)
Para PRs, verificar que el usuario ha considerado:
- Low: Small bug fix or small optional feature
- Medium: New features not enabled, small-medium additions to existing components
- High: Complicated changes like flow control, rewrites of critical components

### 5. Testing Documentation (WARNING)
Verificar que hay información sobre qué testing se realizó:
- Unit tests
- Integration tests
- Manual testing

### 6. Co-authored-by (INFO)
Si hay múltiples autores, verificar formato:
```
Co-authored-by: name <name@example.com>
```

### 7. Issue Reference (INFO para cambios >100 LOC)
Para cambios mayores (>100 líneas), debería haber:
- Referencia a un issue de GitHub
- O link a documento de diseño

**Cálculo de LOC:**
```bash
git diff --stat HEAD~1..HEAD | tail -1
```

### 8. Fixes Format (INFO)
Si cierra un issue, verificar formato:
```
Fixes #XXX
```

## Formato de Salida

```json
{
  "agent": "pr-metadata",
  "findings": [
    {
      "type": "ERROR|WARNING|INFO",
      "check": "nombre_del_check",
      "message": "Descripción del problema",
      "location": "commit SHA o línea",
      "suggestion": "Cómo solucionarlo"
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

1. Obtener lista de commits:
```bash
git log --oneline HEAD~10..HEAD
```

2. Para cada commit, analizar:
```bash
git log -1 --format='%s' <SHA>  # título
git log -1 --format='%B' <SHA>  # mensaje completo
git log -1 --format='%an <%ae>' <SHA>  # autor
```

3. Calcular líneas cambiadas:
```bash
git diff --shortstat HEAD~1..HEAD
```

4. Generar reporte con hallazgos
