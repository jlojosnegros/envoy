# Sub-agente: Extension Review

## Propósito
Verificar que los cambios en extensiones cumplen con la política de extensiones de Envoy.

## Se Activa Cuando
Hay cambios en:
- `source/extensions/`
- `contrib/`
- `api/envoy/extensions/`
- `api/contrib/`

## Requiere Docker: NO (análisis estático)

## Verificaciones

### 1. Nueva Extensión - Sponsor (INFO)
Si es una nueva extensión, verificar sponsor:

**Detectar nueva extensión:**
```bash
git diff --name-only --diff-filter=A HEAD~1..HEAD | grep -E 'source/extensions/.*/[^/]+\.(cc|h)$'
```

**Si es nueva:**
```
INFO: Nueva extensión detectada
Según EXTENSION_POLICY.md, las nuevas extensiones requieren:
- Sponsor: Un maintainer existente que apadrine la extensión
- Reviewers: Al menos 2 reviewers para el código
Por favor, asegúrate de tener sponsor y reviewers antes del PR.
```

### 2. CODEOWNERS Actualizado - WARNING
Si hay nueva extensión, debe actualizarse CODEOWNERS:

```bash
git diff HEAD~1..HEAD -- CODEOWNERS | grep -E 'extensions|contrib'
```

**Si hay nueva extensión sin entrada en CODEOWNERS:**
```
WARNING: Nueva extensión sin entrada en CODEOWNERS
Ubicación: source/extensions/filters/http/new_filter/
Sugerencia: Añadir entrada en CODEOWNERS:
/source/extensions/filters/http/new_filter/ @reviewer1 @reviewer2
```

### 3. Security Posture Tag - WARNING
Verificar que envoy_cc_extension tiene security_posture:

```bash
git diff HEAD~1..HEAD -- 'source/extensions/**/BUILD' | grep -E 'envoy_cc_extension|security_posture'
```

**Tags válidos:**
- `robust_to_untrusted_downstream`
- `robust_to_untrusted_downstream_and_upstream`
- `requires_trusted_downstream_and_upstream`
- `unknown`
- `data_plane_agnostic`

### 4. Status Tag - INFO
Verificar que tiene status tag:

```bash
git diff HEAD~1..HEAD -- 'source/extensions/**/BUILD' | grep -E 'status\s*='
```

**Tags válidos:**
- `stable`
- `alpha`
- `wip`

### 5. Metadata en extensions_metadata.yaml - WARNING
Para contrib extensions, verificar entrada en metadata:

```bash
git diff HEAD~1..HEAD -- 'contrib/extensions_metadata.yaml'
```

### 6. Extensión Contrib vs Core - INFO
Si es contrib, advertir sobre diferencias:

```
INFO: Esta es una extensión contrib
- Requiere end-user sponsor
- NO está incluida en imagen Docker por defecto
- NO tiene cobertura del equipo de seguridad de Envoy
- Debería usar v3alpha para API
```

### 7. Platform Specific Features - WARNING
Si la extensión usa features específicas de plataforma:

```bash
git diff HEAD~1..HEAD | grep -E '#ifdef|#if defined|__linux__|__APPLE__|_WIN32'
```

**Si hay código específico de plataforma:**
```
WARNING: Código específico de plataforma detectado
Según EXTENSION_POLICY.md:
- Evitar #ifdef <OSNAME>
- Preferir feature guards en sistema de build
- Añadir a *_SKIP_TARGETS en bazel/repositories.bzl si no soporta alguna plataforma
```

## Verificación de Archivo BUILD

Para nuevas extensiones, verificar estructura correcta del BUILD:

```python
envoy_cc_extension(
    name = "config",
    # ... deps ...
    security_posture = "robust_to_untrusted_downstream",
    status = "alpha",  # o stable, wip
)
```

## Formato de Salida

```json
{
  "agent": "extension-review",
  "requires_docker": false,
  "extension_type": "core|contrib|new",
  "extension_paths": ["source/extensions/filters/http/new_filter/"],
  "findings": [
    {
      "type": "WARNING",
      "check": "codeowners_missing",
      "message": "Nueva extensión sin entrada en CODEOWNERS",
      "location": "source/extensions/filters/http/new_filter/",
      "suggestion": "Añadir reviewers en CODEOWNERS para esta extensión"
    },
    {
      "type": "WARNING",
      "check": "security_posture_missing",
      "message": "Falta security_posture en envoy_cc_extension",
      "location": "source/extensions/filters/http/new_filter/BUILD",
      "suggestion": "Añadir security_posture apropiado basado en el threat model"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "extension_info": {
    "is_new": true|false,
    "is_contrib": true|false,
    "has_sponsor": "unknown",
    "security_posture": "robust_to_untrusted_downstream|...|missing",
    "status": "stable|alpha|wip|missing"
  }
}
```

## Ejecución

1. Identificar cambios en extensiones:
```bash
git diff --name-only HEAD~1..HEAD | grep -E '(source/extensions|contrib)'
```

2. Determinar si es extensión nueva o modificación

3. Si es nueva:
   - Informar sobre requisito de sponsor
   - Verificar CODEOWNERS

4. Verificar BUILD file:
   - security_posture presente
   - status presente

5. Verificar si es contrib:
   - extensions_metadata.yaml actualizado
   - Informar sobre diferencias

6. Buscar código específico de plataforma

7. Generar reporte

## Referencias

- EXTENSION_POLICY.md: Política completa
- CODEOWNERS: Asignación de reviewers
- extensions_metadata.yaml (contrib): Metadata de contrib
