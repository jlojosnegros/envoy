# Sub-agente: API Review

## Propósito
Verificar que los cambios en la API cumplen con el checklist de revisión de Envoy.

## Se Activa Cuando
Hay cambios en el directorio `api/`

## Requiere Docker: SI (para api_compat)

## Verificaciones

### Fase 1: Análisis Estático (Sin Docker)

#### 1.1 Default Values Safe - ERROR
Verificar que los valores por defecto no causan cambios de comportamiento:

```bash
git diff HEAD~1..HEAD -- 'api/**/*.proto' | grep -E 'default|= [0-9]|= true|= false'
```

**Preguntas a considerar:**
- ¿Los valores por defecto causarán cambios para usuarios existentes?
- ¿Es necesario un runtime guard?

#### 1.2 Validation Rules - WARNING
Verificar presencia de reglas protoc-gen-validate:

```bash
git diff HEAD~1..HEAD -- 'api/**/*.proto' | grep -E '\[(validate\.|rules)'
```

**Campos a verificar:**
- Campos numéricos tienen bounds
- Campos required están marcados
- Campos repeated tienen min/max

#### 1.3 Deprecation Documentation - ERROR
Si hay campos deprecated, verificar documentación:

```bash
git diff HEAD~1..HEAD -- 'api/**/*.proto' | grep -i 'deprecated'
```

**Si hay deprecated:**
- Debe existir alternativa documentada
- Debe estar en release notes
- No deprecated sin alternativa lista

#### 1.4 Style Compliance - WARNING
Verificar cumplimiento con api/STYLE.md:

- Nombres de campos en snake_case
- Nombres de mensajes en PascalCase
- Comentarios de documentación presentes
- Uso correcto de WKT (Well-Known Types)

#### 1.5 Extension Point Usage - INFO
Verificar si debería usar TypedExtensionConfig:

```bash
git diff HEAD~1..HEAD -- 'api/**/*.proto' | grep -E 'oneof|Any|typed_config'
```

**Considerar:**
- ¿Es esta una nueva funcionalidad extensible?
- ¿Debería ser un plugin en lugar de código core?

### Fase 2: Breaking Changes (Con Docker)

#### Comando CI
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh api_compat' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-api-compat.log
```

**Este comando detecta:**
- Campos renumerados
- Tipos cambiados
- Campos renombrados
- Breaking changes en wire format

## Checklist de api/review_checklist.md

### Feature Enablement
- [ ] ¿Valores por defecto causan cambios de comportamiento?
- [ ] ¿Usuarios pueden deshabilitar el cambio?
- [ ] ¿Necesita WKT para valores que podrían cambiar?

### Validation Rules
- [ ] ¿Tiene reglas validate?
- [ ] ¿Campo obligatorio marcado como required?
- [ ] ¿Campos numéricos tienen bounds?

### Deprecations
- [ ] ¿Alternativa disponible en clientes conocidos?
- [ ] ¿Documentación apunta al reemplazo?

### Extensibility
- [ ] ¿Debería ser extension point?
- [ ] ¿Enum debería ser oneof con mensajes vacíos?

### Consistency
- [ ] ¿Reutiliza tipos existentes donde sea posible?
- [ ] ¿Nombres consistentes con API existente?

### Failure Modes
- [ ] ¿Modo de fallo documentado?
- [ ] ¿Comportamiento consistente entre clientes?

### Documentation
- [ ] ¿Comentarios claros en proto?
- [ ] ¿Ejemplos donde sean útiles?

## Formato de Salida

```json
{
  "agent": "api-review",
  "requires_docker": true,
  "docker_executed": true|false,
  "api_files_changed": ["api/envoy/config/foo.proto"],
  "findings": [
    {
      "type": "ERROR",
      "check": "breaking_change",
      "message": "Campo renombrado causa breaking change",
      "location": "api/envoy/config/foo.proto:45",
      "suggestion": "No renombrar campos existentes, añadir nuevo campo y deprecar el antiguo"
    },
    {
      "type": "WARNING",
      "check": "validation_missing",
      "message": "Campo numérico sin validación de bounds",
      "location": "api/envoy/config/foo.proto:67",
      "suggestion": "Añadir [(validate.rules).uint32 = {gte: 0, lte: 100}]"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "checklist_status": {
    "feature_enablement": "PASS|WARN|FAIL",
    "validation_rules": "PASS|WARN|FAIL",
    "deprecations": "PASS|WARN|FAIL|N/A",
    "extensibility": "PASS|WARN|FAIL",
    "consistency": "PASS|WARN|FAIL"
  }
}
```

## Ejecución

### Fase 1 (Sin Docker - SIEMPRE):

1. Identificar archivos proto modificados:
```bash
git diff --name-only HEAD~1..HEAD | grep '\.proto$'
```

2. Analizar diff para cada verificación estática

3. Revisar checklist

### Fase 2 (Con Docker):

1. Verificar ENVOY_DOCKER_BUILD_DIR

2. Ejecutar api_compat:
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh api_compat'
```

3. Parsear output para breaking changes

4. Combinar con resultados de Fase 1

## Notas

- api_compat compara contra el commit base para detectar breaking changes
- Las reglas de backwards compatibility son estrictas en Envoy
- Cambios en protos pueden afectar a múltiples clientes xDS
