# Auditoría de la base de datos HeartMed (SQLite)

**Fecha:** 19/10/2026 · **Equipo:** Heart Pirates · **Versión oficial auditada:** `heartmed_sqlite.sql` + `heartmed_datos_sqlite.sql`

**Archivos revisados:** `heartmed_db.sqlite3` (el que se subió), `heartmed_sqlite.sql`, `heartmed_datos_sqlite.sql`, `heartmed_relacional_v2.mmd`, `00_crear_bd.sql`, `01_seguridad.sql`, `datos_01.sql`.

> **Actualización 20/10/2026:** se aplicó la revisión de normalización (sección 11). Quedaron resueltas las decisiones D-2, D-3, D-4, D-6, D-7, D-12 y D-15, y el diagrama vigente es `heartmed_relacional_v4.mmd`. Las secciones 1 a 10 se dejan como estaban el 19/10, para que quede el historial.

**Cómo se revisó:** no fue una lectura a ojo. Se reconstruyó la base desde los scripts y se comparó con el `.sqlite3` subido. Después se intentaron guardar 39 datos inválidos para ver cuáles aceptaba. Se corrigió, se volvió a probar todo y se comparó el diagrama columna por columna contra la base con un script.

---

## 1. Resumen ejecutivo

**Estado general:** la base **corre sin errores** y los datos que tenía son coherentes. Las FK cuadran, el hash de `Prueba123*` es correcto y el log 7 sí apunta al paso 3 de hemorragias. El `.sqlite3` subido es **idéntico** a lo que generan los scripts.

El problema es otro: **la base deja entrar datos malos**. De 39 datos inválidos que se intentaron guardar, la base original **aceptó 31**. Solo rechazó 8: borrados protegidos por FK, UNIQUE de ids, cantidades negativas y el estado en minúscula. La versión corregida **rechaza los 39** y sigue aceptando todo lo válido (12 de 12 operaciones normales).

**Los 5 problemas más graves:**

1. **El diagrama `.mmd` v2 no es la base real.** Tiene 10 tablas catálogo que no existen y 9 tablas con columnas distintas. Es el diagrama relacional que se presenta a la profesora. → Se hizo el **v3**, verificado contra la base.
2. **Una alerta se puede guardar sin apuntar a nada** o apuntando a lo que no es (ej. `STOCK_BAJO` con un lote). → `CHECK` que amarra el tipo con su referencia.
3. **Las llaves foráneas vienen apagadas en cada conexión nueva** (comprobado: `PRAGMA foreign_keys` da 0). Si el backend en Crow no ejecuta `PRAGMA foreign_keys = ON;` al abrir cada conexión, ninguna FK, CASCADE ni RESTRICT funciona.
4. **Las cantidades aceptan texto y decimales.** `lotes.cantidad = 'abc'` pasa el `CHECK (cantidad >= 0)` porque en SQLite `'abc' >= 0` da VERDADERO. También pasa `5.5`. → `CHECK (typeof(...) = 'integer')`.
5. **Las fechas no tienen formato.** Se guardan `'31/02/2027'` y `'2027-02-31'`. Como SQLite compara fechas como texto, un formato mezclado rompe los `CHECK` de fechas y los `ORDER BY`. → `CHECK` de formato en las 17 columnas de fecha.

---

## 2. Tabla de hallazgos

Ordenada de más a menos grave. El número del hallazgo es el mismo `-- CAMBIO #n` que aparece en los scripts corregidos.

| # | Severidad | Archivo | Tabla.columna | Problema | Corrección propuesta |
|---|---|---|---|---|---|
| 1 | CRÍTICO | heartmed_relacional_v2.mmd | (todo el diagrama) | El diagrama no coincide con la base. Tiene 10 catálogos que no existen (`tipos_emergencia`, `niveles_prioridad`, `estados_emergencia`, `estados_salud`, `tipos_evento`, `tipos_insumo`, `estados_tratamiento`, `tipos_alerta`, `estados_alerta`, `tipos_movimiento`). Además difiere en pacientes, protocolos, eventos_medicos, controles_evolucion, emergencias, medicamentos, tratamientos, movimientos_inventario y alertas (detalle en la sección 7). El título dice "MySQL 8". | Nuevo `heartmed_relacional_v3.mmd`, sin catálogos (el equipo usa CHECK). Se verificó por código: 19 tablas y 0 diferencias en columnas, tipos, NULL y FK. |
| 2 | CRÍTICO | heartmed_sqlite.sql | alertas.id_medicamento / id_lote / id_tratamiento | Nada obliga a que cada tipo use su referencia. Se aceptaron una alerta sin ninguna referencia, una `STOCK_BAJO` con lote y otra con dos referencias. | `chk_alertas_referencia`: cada tipo exige exactamente su FK y las otras dos en NULL. |
| 3 | CRÍTICO | backend (Crow) | todas las FK | SQLite trae las FK apagadas en cada conexión nueva (verificado). El script lo advierte, pero falta comprobarlo en el código del backend. | Ejecutar `PRAGMA foreign_keys = ON;` justo después de `sqlite3_open` en **cada** conexión. Probar con `PRAGMA foreign_keys;`, que debe dar 1. |
| 4 | ALTO | heartmed_sqlite.sql | 17 columnas DATE/DATETIME | No se valida el formato. Se aceptaron `'31/02/2027'` y `'2027-02-31'`. Si el backend manda `'2026-10-10T10:00:00'`, deja de compararse bien contra `'2026-10-10 11:00:00'`. | `CHECK (date(col) IS col)` en DATE y `CHECK (datetime(col) IS col)` en DATETIME. El formato queda fijo: `AAAA-MM-DD` y `AAAA-MM-DD HH:MM:SS`. |
| 5 | ALTO | heartmed_sqlite.sql | lotes.cantidad, movimientos_inventario.cantidad, medicamentos.cantidad_total_disponible, medicamentos.umbral_minimo, pacientes.recompensa, pasos_protocolo.numero, logs_auditoria.id_registro | Aceptan texto y decimales. `'abc'` y `5.5` pasaron el `CHECK (cantidad >= 0)`. | `CHECK (typeof(col) = 'integer' AND ...)`. |
| 6 | ALTO | heartmed_sqlite.sql | usuarios.correo, pacientes.documento_identidad, roles.nombre_rol, permisos.cod_acceso, lotes.num_lote | En MySQL (`utf8mb4_unicode_ci`) el UNIQUE no distinguía mayúsculas; en SQLite sí. Se crearon `law@…` y `LAW@…` como dos cuentas distintas. | `COLLATE NOCASE` en esas columnas. |
| 7 | ALTO | heartmed_sqlite.sql | medicamentos.nombre | No es UNIQUE, aunque el diagrama dice UK. Se guardó "Ibuprofeno" dos veces. | `uk_medicamentos_nombre` con NOCASE. |
| 8 | ALTO | heartmed_sqlite.sql | medicamentos.cantidad_total_disponible | Es un dato derivado (suma de los lotes) y nada lo mantiene. Se descuadra con el primer error del backend. | 3 triggers en `lotes` (insert/update/delete) que recalculan el total desde cero. El backend solo lo lee. |
| 9 | ALTO | heartmed_sqlite.sql | emergencias.fecha_fin, estado | Se aceptaron `fecha_fin < fecha_inicio` y una emergencia `FINALIZADA` sin `fecha_fin`. | `chk_emergencias_fechas` y `chk_emergencias_estado_fecha_fin` (FINALIZADA ⇔ tiene fecha_fin). |
| 10 | ALTO | heartmed_sqlite.sql | tratamientos.fecha_finalizacion_efectiva, estado_tratamiento | La fecha efectiva podía ser anterior al inicio, un tratamiento ACTIVO podía tener fecha efectiva y uno FINALIZADO podía no tenerla. | `chk_tratamientos_fecha_efectiva` y `chk_tratamientos_estado_fecha_efectiva`. SUSPENDIDO queda libre porque se puede reanudar. |
| 11 | ALTO | heartmed_datos_sqlite.sql | historias_clinicas / pacientes 4 | Clione (paciente 4) no tiene historia clínica, aunque el diagrama dice 1 a 1 obligatorio. Además está `EN_TRATAMIENTO` sin ningún tratamiento. | Se agregó la historia 4 y tratamientos/control de Clione. Ver también D-5. |
| 12 | ALTO | heartmed_datos_sqlite.sql | 10 tablas | Había 10 de 19 tablas vacías: eventos, controles, emergencias, emergencia_paciente, acciones, medicamentos, lotes, tratamientos, movimientos y alertas. No había cómo probar los módulos 2 y 3. | Datos nuevos que cubren todos los estados y tipos, con el inventario cuadrado (sección 6). |
| 13 | MEDIO | heartmed_sqlite.sql | alertas.resuelta, fecha_resolucion | Se aceptaron `resuelta = 1` sin fecha y una resolución anterior a la generación. | `chk_alertas_resuelta_fecha` y `chk_alertas_fechas`. |
| 14 | MEDIO | heartmed_sqlite.sql | alertas | El proceso que revisa el inventario puede crear la misma alerta abierta muchas veces. | Índices únicos parciales: una `STOCK_BAJO` abierta por medicamento y una `VENCIMIENTO_PROXIMO` abierta por lote. |
| 15 | MEDIO | heartmed_sqlite.sql | movimientos_inventario.id_tratamiento | Se aceptó una ENTRADA asociada a un tratamiento. | `chk_movimientos_inventario_tratamiento_solo_consumo`. |
| 16 | MEDIO | heartmed_sqlite.sql | movimientos_inventario.id_lote | Se podía descontar ibuprofeno para un tratamiento de amoxicilina. | Trigger `trg_movimientos_inventario_bi/bu_lote_tratamiento`. |
| 17 | MEDIO | heartmed_sqlite.sql | logs_auditoria | La auditoría se podía editar y borrar (probado). | Triggers de solo lectura. Efecto lateral: el `ON UPDATE CASCADE` hacia logs queda bloqueado, lo cual no importa porque los id nunca cambian. |
| 18 | MEDIO | heartmed_sqlite.sql | 19 columnas FK | MySQL crea solo un índice por cada FK; SQLite no. Sin índice, cada JOIN o borrado recorre la tabla completa. | 19 índices de FK y 2 de búsqueda (`logs_auditoria` por registro y `lotes` por fecha de vencimiento). |
| 19 | MEDIO | heartmed_sqlite.sql | pacientes.id_usuario, historias_clinicas.id_paciente, eventos_medicos (2 FK), controles_evolucion (2 FK) | 6 FK sin `ON DELETE`/`ON UPDATE`. Hoy funcionan como NO ACTION (≈RESTRICT), pero no siguen el criterio declarado del equipo. | `ON DELETE RESTRICT ON UPDATE CASCADE` explícito, porque los datos clínicos nunca se borran en cascada. |
| 20 | MEDIO | heartmed_sqlite.sql | pacientes.recompensa, pasos_protocolo.numero | Se aceptaron recompensa = -5 y paso número 0. | `recompensa >= 0`, `numero >= 1`. |
| 21 | MEDIO | heartmed_sqlite.sql | documento_identidad, nombre_completo, correo, cod_acceso, nombre_rol, medicamentos.nombre, num_lote | NOT NULL deja pasar `''` y `'  '`. Se guardó un paciente con documento vacío. | `CHECK (trim(col) <> '')`, y el correo con la forma mínima `algo@algo.algo`. |
| 22 | MEDIO | heartmed_sqlite.sql | pacientes.tipo_sangre | No tiene dominio y se aceptó `'ZZ'`. Los datos usan F, X y S (grupos de One Piece). | **DECISIÓN PENDIENTE D-2.** |
| 23 | MEDIO | heartmed_sqlite.sql | medicamentos.tipo_insumo | Es la única categoría en texto libre; todas las demás tienen CHECK. | **DECISIÓN PENDIENTE D-3.** |
| 24 | MEDIO | ambos scripts | todas las fechas con DEFAULT | `CURRENT_TIMESTAMP` guarda en UTC, pero los datos de prueba están en hora de Bogotá. En la prueba, una acción con hora por defecto (03:42 UTC del 20/10) quedó *después* de una emergencia cerrada a las 23:00 del 19/10. | **DECISIÓN PENDIENTE D-4.** |
| 25 | MEDIO | backend (Crow) | pacientes.recompensa | La recompensa de Law es 3.000.000.000, que no cabe en un `int` de C++ (máximo 2.147.483.647). | Usar `int64_t` con `sqlite3_bind_int64` / `sqlite3_column_int64`. |
| 26 | BAJO | ambos scripts | comentarios | Hay comentarios desactualizados: "uno por cada valor del ENUM" (en SQLite es CHECK), "activo: TRUE -> FALSE" (es 1 -> 0) y la cabecera dice que "el diseño no cambió" respecto a MySQL, aunque sí cambió. | Comentarios corregidos. |
| 27 | BAJO | heartmed_sqlite.sql | nombres de constraints | Había tres estilos: el módulo 2 usaba `fk_historias_paciente` (singular), los módulos 1 y 3 `fk_<tabla>_<tablas>`, y los CHECK de los módulos 1 y 2 no tenían nombre. | Una sola convención: `pk_`, `fk_<tabla>_<referida>`, `uk_`, `chk_`, `idx_`, `trg_`. Con nombre, el error dice qué regla falló (ej. `chk_lotes_cantidad`). |
| 28 | BAJO | heartmed_sqlite.sql | varias | Los nombres de columnas no siguen un patrón: `fecha` / `fecha_evento` / `fecha_hora` / `fecha_movimiento`; `tipo` / `tipo_evento`; `estado` / `estado_salud`; `nivel_urgencia` / `prioridad` (mismo dominio). | **No se cambiaron** para no romper el backend. Ver D-14. |
| 29 | BAJO | heartmed_sqlite.sql | controles_evolucion.fecha, emergencias.fecha_inicio, eventos_medicos.fecha_evento | No tienen DEFAULT, mientras que `acciones_emergencia.fecha_hora` sí. | Opcional: `DEFAULT CURRENT_TIMESTAMP`. No se aplicó; depende de D-4. |
| 30 | BAJO | heartmed_sqlite.sql | logs_auditoria.accion | Texto libre: se puede escribir `'insert'`, `'Insert'` o `'INSERTAR'`. | **DECISIÓN PENDIENTE D-10.** |
| 31 | BAJO | 01_seguridad.sql, datos_01.sql | módulo 1 MySQL | El diseño del módulo 1 coincide con SQLite. No se pudieron comparar los módulos 2 y 3 porque `02_…sql` y `03_…sql` de MySQL no están en el proyecto. | **DECISIÓN PENDIENTE D-13.** |
| 32 | BAJO | heartmed_sqlite.sql | historias_clinicas.alergias, antecedentes, cirugias_previas; controles_evolucion.parametros_clinicos | Guardan listas en un solo texto (1FN débil). La base no puede impedir que se le recete amoxicilina a Jean Bart, que es alérgico a la penicilina. | **DECISIONES PENDIENTES D-12 y D-15.** |
| 33 | BAJO | heartmed_sqlite.sql | eventos_medicos.id_historia vs controles_evolucion.id_paciente | Los eventos cuelgan de la historia y los controles del paciente. Es equivalente porque la relación es 1 a 1, pero no es uniforme. | Dejarlo así: cambiarlo rompería el backend y no gana nada. |
| 34 | BAJO | heartmed_datos_sqlite.sql | pacientes 1 (Law) | La cuenta de paciente de Law es de rol Capitán, no Tripulante. | **DECISIÓN PENDIENTE D-11.** |
| 35 | BAJO | heartmed_sqlite.sql | eventos_medicos.diagnostico | Es obligatorio incluso para una `OBSERVACION` o una lesión recién registrada. | **DECISIÓN PENDIENTE D-9.** |

### Lo que se verificó y está bien

- El `.sqlite3` subido es idéntico a lo que generan los scripts del proyecto: mismo esquema, mismos datos y mismas secuencias.
- `PRAGMA integrity_check` = ok y `PRAGMA foreign_key_check` sin filas.
- El hash de `Prueba123*` en el script SQLite es el SHA-256 correcto (el mismo que daba `SHA2()` en MySQL).
- Los logs apuntan a registros que existen y el log de `pasos_protocolo` con id 7 sí es el paso 3 de hemorragias.
- Los 5 tipos de emergencia tienen protocolo y pasos numerados 1, 2, 3… sin huecos.
- El dominio de `estado_salud` es idéntico en pacientes y controles, el de tipo de emergencia es idéntico en protocolos y emergencias, y el de prioridad/urgencia es idéntico en alertas y emergencias.
- Los permisos por rol coinciden con las descripciones de los roles.
- `BIGINT` para recompensa es correcto: en SQLite los INTEGER son de 64 bits.

---

## 3. Decisiones pendientes para el equipo

Ya está decidido que **se usa TEXT + CHECK, no tablas catálogo**. Por eso el SQLite manda y el diagrama v3 se ajustó a él.

Todo el código de esta sección **se probó** sobre la base corregida.

**D-1 · Columnas que solo existían en el diagrama v2.** Esto es lo que se recomienda para cada una:

| Columna del v2 | Recomendación |
|---|---|
| `medicamentos.dias_aviso_vencimiento` | **Agregarla.** Sin ella, el backend tiene que inventarse cuántos días antes avisar `VENCIMIENTO_PROXIMO`. |
| `eventos_medicos.observaciones` | Agregarla como `TEXT NULL` si el formulario la pide. |
| `alertas.id_usuario_destino` | No agregarla si la alerta se avisa por rol (stock al Admin. de Inventario, dosis al médico). Si un RF pide avisar a una persona puntual, agregarla como FK NULL. |
| `tratamientos.duracion_dias` | **No agregarla:** se calcula con `fecha_fin - fecha_inicio` y guardarla rompe la 3FN. |
| `tratamientos.id_usuario_prescribe` | Mantener `id_usuario_registra` (RF-40), salvo que quien prescribe y quien registra puedan ser personas distintas. |

```sql
-- dentro de CREATE TABLE medicamentos:
dias_aviso_vencimiento INTEGER NOT NULL DEFAULT 30
    CONSTRAINT chk_medicamentos_dias_aviso
    CHECK (typeof(dias_aviso_vencimiento) = 'integer' AND dias_aviso_vencimiento >= 0),
```

**D-2 · Dominio de `tipo_sangre`.** Opción A: grupos de One Piece (F, S, X, XF), que es lo que usan los datos. Opción B: grupos reales (A+, O−…). **Recomendación: A.**
```sql
tipo_sangre VARCHAR(5) NOT NULL
    CONSTRAINT chk_pacientes_tipo_sangre CHECK (tipo_sangre IN ('F', 'S', 'X', 'XF')),
```

**D-3 · Dominio de `tipo_insumo`.** Los datos de prueba usan `'MEDICAMENTO'` e `'INSUMO'`. El equipo define la lista definitiva y se agrega:
```sql
tipo_insumo VARCHAR(50) NOT NULL
    CONSTRAINT chk_medicamentos_tipo_insumo CHECK (tipo_insumo IN ('MEDICAMENTO', 'INSUMO')),
```

**D-4 · Zona horaria.** Opción A: todo en UTC; la base ya lo hace con los DEFAULT y el frontend muestra la hora de Bogotá (UTC−5). Opción B: todo en hora local (`DEFAULT (datetime('now', '-5 hours'))`). **Recomendación: A.** Es lo estándar, y el servidor puede estar en cualquier país. Si se escoge A, los datos de prueba se leen como si estuvieran en UTC y no hay que cambiarlos.

**D-5 · Crear la historia clínica automáticamente.** Con este trigger, todo paciente nace con su historia vacía y la regla 1 a 1 queda garantizada por la base. Condición: el backend (KAN-18/KAN-19) **ya no debe insertar la historia** al crear el paciente, porque daría error de UNIQUE. **Recomendación: sí, coordinándolo con Juan Salazar.**
```sql
CREATE TRIGGER trg_pacientes_ai_historia AFTER INSERT ON pacientes
BEGIN
    INSERT INTO historias_clinicas (id_paciente) VALUES (NEW.id_paciente);
END;
```

**D-6 · `pacientes.estado_salud` = estado del último control.** Hoy es un dato duplicado que depende del backend. Con el trigger, se actualiza solo al registrar un control, y si llega un control viejo cargado tarde no lo pisa. **Recomendación: sí.**
```sql
CREATE TRIGGER trg_controles_evolucion_ai_estado AFTER INSERT ON controles_evolucion
WHEN NOT EXISTS (SELECT 1 FROM controles_evolucion c
                  WHERE c.id_paciente = NEW.id_paciente
                    AND (c.fecha > NEW.fecha OR (c.fecha = NEW.fecha AND c.id_control > NEW.id_control)))
BEGIN
    UPDATE pacientes SET estado_salud = NEW.estado WHERE id_paciente = NEW.id_paciente;
END;
```

**D-7 · `lotes.cantidad` calculada desde los movimientos, y bajas de vencidos.** Con el trigger, cada movimiento suma o resta en su lote, y un consumo mayor al stock **falla solo** gracias a `chk_lotes_cantidad` (probado). Hay dos condiciones:

- El backend deja de actualizar `lotes.cantidad` a mano.
- Los lotes se crean con cantidad 0 más un movimiento ENTRADA. Los datos de prueba se ajustan restando las entradas.

Falta además un tipo de movimiento para dar de baja lotes vencidos o dañados (`'BAJA'`), y conviene exigir `motivo` en un consumo que no viene de un tratamiento. **Recomendación: sí, coordinándolo con Daniel.**
```sql
CREATE TRIGGER trg_movimientos_inventario_ai_stock AFTER INSERT ON movimientos_inventario
BEGIN
    UPDATE lotes
       SET cantidad = cantidad + CASE NEW.tipo_movimiento WHEN 'ENTRADA' THEN NEW.cantidad ELSE -NEW.cantidad END
     WHERE id_lote = NEW.id_lote;
END;
```

**D-8 · Un solo tratamiento ACTIVO por paciente y medicamento.** Así se evita recetar dos veces lo mismo; para cambiar la dosis se finaliza el anterior. **Recomendación: sí.**
```sql
CREATE UNIQUE INDEX uk_tratamientos_activo_paciente_medicamento
    ON tratamientos (id_paciente, id_medicamento) WHERE estado_tratamiento = 'ACTIVO';
```

**D-9 · `eventos_medicos.diagnostico` NULL.** Si se registra una observación o una lesión sin diagnóstico todavía, hoy toca escribir algo de relleno. **Recomendación:** permitir NULL (`diagnostico TEXT NULL`).

**D-10 · Dominio de `logs_auditoria.accion`.** **Recomendación:**
```sql
CONSTRAINT chk_logs_auditoria_accion CHECK (accion IN ('INSERT', 'UPDATE', 'DELETE', 'LOGIN', 'LOGOUT')),
```

**D-11 · ¿Un usuario que no es Tripulante puede ser paciente?** Law es paciente con cuenta de Capitán. Es válido (el médico también se enferma), pero entonces podría editar su propia historia. **Recomendación:** permitirlo y que el backend no deje a un usuario editar su propia historia clínica.

**D-12 · Alergias estructuradas.** Con texto libre no hay forma de bloquear una receta peligrosa. Opción: tabla `paciente_alergia (id_paciente, id_medicamento o principio activo)`. **Recomendación:** dejarlo para un sprint posterior. Por ahora, que el backend muestre las alergias en la pantalla de prescripción.

**D-13 · Scripts MySQL.** **Recomendación:** marcarlos como obsoletos (carpeta `mysql_obsoleto/`) para que nadie los use como referencia. Mantener dos versiones sincronizadas es doble trabajo.

**D-14 · Renombrar columnas inconsistentes** (hallazgo 28). **Recomendación:** no hacerlo ahora, salvo que el backend todavía no use esas columnas.

**D-15 · `parametros_clinicos`.** Si algún día se quieren gráficas de signos vitales, se puede guardar como JSON y validarlo con `CHECK (json_valid(parametros_clinicos))`, o pasarlo a columnas (presión, FC, temperatura). **Recomendación:** dejarlo como texto mientras no haya un requisito de gráficas.

---

## 4. Script SQLite corregido

Archivos `heartmed_sqlite.sql` y `heartmed_datos_sqlite.sql`. Cada cambio está marcado con `-- CAMBIO #n` (n = número del hallazgo). Se probó con el comando oficial, dos veces seguidas, para confirmar que se puede volver a ejecutar:

```
sqlite3 heartmed_db.sqlite3 ".read heartmed_sqlite.sql" ".read heartmed_datos_sqlite.sql"
```

**Compatibilidad con el backend:** no se renombró ni se quitó ninguna columna ni tabla, así que las consultas que ya existan siguen funcionando. Lo único que puede afectar al backend:

- Fechas con otro formato.
- Números enviados como texto.
- Datos que antes entraban y ahora son inválidos.
- Escribir a mano `medicamentos.cantidad_total_disponible` (ahora la calculan los triggers).

### Notas para el backend (Crow / C++)
1. Ejecutar `PRAGMA foreign_keys = ON;` en **cada** conexión.
2. Fechas: `AAAA-MM-DD` para DATE y `AAAA-MM-DD HH:MM:SS` para DATETIME, sin `T`, milisegundos ni zona horaria.
3. Los valores de los CHECK van en MAYÚSCULAS exactas (`'SANO'`, no `'sano'`).
4. `recompensa` se maneja como `int64_t` (`sqlite3_bind_int64` / `sqlite3_column_int64`).
5. Los mensajes de error traen el nombre de la regla (ej. `CHECK constraint failed: chk_lotes_cantidad`), así que se pueden traducir a un mensaje para el usuario.
6. Si Crow atiende varias peticiones a la vez, conviene `PRAGMA journal_mode = WAL;` (una vez) y `sqlite3_busy_timeout(db, 5000)` en cada conexión, para evitar errores de "database is locked".
7. Se recomienda bcrypt o argon2 para las contraseñas; el SHA-256 sin sal es solo para los datos de prueba.

---

## 5. Diagrama

`heartmed_relacional_v3.mmd` (más `heartmed_relacional_v3.png` para verlo). Reemplaza al v2. Muestra los valores permitidos de cada CHECK, cuáles columnas son NULL, los DEFAULT y la relación protocolos–emergencias con línea punteada, porque es por valor y no por FK.

---

## 6. Verificación

Archivo `verificacion_heartmed.sql`. El bloque de resumen tiene **25 chequeos** y todos deben dar 0. Resultado sobre la base corregida: **25 de 25 en 0**, `integrity_check` = ok y `foreign_key_check` sin filas.

Sobre la base original, los chequeos marcan **V01 = 1** (Clione sin historia).

Algunos de los chequeos:

- Lotes que no cuadran con sus movimientos, y lotes que alguna vez quedaron en negativo.
- Totales de medicamentos que no cuadran con sus lotes.
- Estado del paciente distinto al de su último control.
- Medicamentos bajo el mínimo sin alerta abierta.
- Acciones fuera del horario de su emergencia.
- Consumos de lotes vencidos.
- Registros hechos por usuarios sin el permiso correspondiente o inactivos.
- Logs que apuntan a registros que no existen.
- Duplicados sin distinguir mayúsculas.
- Fechas con formato malo y números guardados como texto.

### Cuadre del inventario en los datos de prueba

| Lote | Medicamento | Movimientos | Cantidad |
|---|---|---|---|
| IBU-2601 | Ibuprofeno 400 mg | +80 | 80 |
| IBU-2509 | Ibuprofeno 400 mg | +40 −4 −21 | 15 (vence 10/11 → alerta) |
| AMX-2604 | Amoxicilina 500 mg | +30 −18 | 12 (< 20 → alerta STOCK_BAJO) |
| VEN-2602 | Venda elástica 10 cm | +10 −2 | 8 |
| VEN-2609 | Venda elástica 10 cm | +17 | 17 |
| SUL-2603 | Sulfadiazina de plata 1 % | +5 −1 | 4 |

Totales que calculan los triggers: ibuprofeno 95, amoxicilina 12, vendas 25 y sulfadiazina 4.

---

## 7. Detalle: diagrama v2 vs. SQLite (hallazgo 1)

| Tabla | En el diagrama v2 | En SQLite |
|---|---|---|
| pacientes | `identificador_unico`, `id_estado_salud` FK | `documento_identidad`, `estado_salud` TEXT+CHECK |
| protocolos | `id_tipo_emergencia` FK | `tipo_emergencia` TEXT+CHECK |
| eventos_medicos | `id_tipo_evento` FK, `observaciones` | `tipo_evento` TEXT+CHECK, sin observaciones |
| controles_evolucion | `id_estado_salud` FK | `estado` TEXT+CHECK |
| emergencias | `id_tipo_emergencia`, `id_nivel_prioridad`, `id_estado_emergencia` | `tipo`, `nivel_urgencia`, `estado` TEXT+CHECK |
| medicamentos | `nombre` VARCHAR(150) UK, `id_tipo_insumo`, `dias_aviso_vencimiento` | `nombre` VARCHAR(100) sin UK, `tipo_insumo` texto, `cantidad_total_disponible` |
| movimientos_inventario | `id_tipo_movimiento`, `fecha` | `tipo_movimiento`, `fecha_movimiento`, más `id_tratamiento` y `motivo` |
| tratamientos | `id_usuario_prescribe`, dosis/frecuencia VARCHAR(50), `duracion_dias`, fechas DATETIME, `id_estado_tratamiento` | `id_usuario_registra`, VARCHAR(100), fechas DATE, `estado_tratamiento`, `fecha_finalizacion_efectiva`, `fecha_registro` |
| alertas | `id_usuario_destino`, `id_tipo_alerta`, `id_nivel_prioridad`, `id_estado_alerta`, `mensaje` VARCHAR(500); el comentario dice `PROXIMO_A_VENCER` | `tipo_alerta`, `prioridad`, `notificada`, `resuelta`, `fecha_resolucion`, `mensaje` VARCHAR(300); el valor real es `VENCIMIENTO_PROXIMO` |

---

## 8. NULL / NOT NULL por tabla

Todas las columnas que no aparecen aquí son NOT NULL y está bien que lo sean, porque sin ellas el registro no tiene sentido.

- **roles:** `descripcion` NULL ✔ (es opcional).
- **logs_auditoria:** `id_registro` NULL ✔ (hay acciones sin registro concreto); `detalle_cambio` NULL ✔.
- **pacientes:**
  - `id_usuario` NULL ✔ (paciente sin cuenta, relación 0..1).
  - `foto_perfil` NULL ✔.
  - `tipo_sangre` y `fecha_nacimiento` NOT NULL ✔ para tripulantes registrados. Si algún día se registran pacientes de urgencia sin esos datos, habría que permitir NULL.
  - `estado_salud` NOT NULL sin DEFAULT: aceptable, porque obliga a escoger un estado al registrar.
- **historias_clinicas:** `alergias`, `antecedentes`, `cirugias_previas` y `observaciones` NULL ✔ (historia vacía, KAN-18/19). Convención: NULL = "no se ha preguntado"; `'Ninguna conocida'` = "se preguntó y no tiene".
- **eventos_medicos:** `diagnostico` NOT NULL ⚠ (ver D-9).
- **emergencias:** `lesiones` NULL ✔ (una intoxicación no tiene lesiones); `fecha_fin` NULL ✔, ahora amarrada al estado.
- **medicamentos:** `descripcion` NULL ✔.
- **tratamientos:** `fecha_fin` NULL ✔ (tratamiento sin fecha de fin prevista); `fecha_finalizacion_efectiva` NULL ✔, ahora amarrada al estado.
- **movimientos_inventario:** `id_tratamiento` NULL ✔, solo en CONSUMO; `motivo` NULL ⚠ (en un consumo sin tratamiento debería ser obligatorio, ver D-7).
- **alertas:** las tres FK NULL ✔, ahora amarradas al tipo; `fecha_resolucion` NULL ✔, amarrada a `resuelta`.

## 9. Normalización (1FN, 2FN, 3FN)

| Tabla | Resultado | Nota |
|---|---|---|
| roles, permisos, rol_permiso, usuarios, protocolos, pasos_protocolo, eventos_medicos, emergencias, emergencia_paciente, acciones_emergencia, tratamientos, movimientos_inventario, alertas, logs_auditoria | ✔ 3FN | `logs_auditoria.id_registro` no tiene FK a propósito (apunta a distintas tablas). |
| pacientes | ✔ 3FN con un dato derivado | `estado_salud` repite el último control (D-6). |
| historias_clinicas | ⚠ 1FN débil | Listas dentro de un texto (D-12). |
| controles_evolucion | ⚠ 1FN débil | `parametros_clinicos` junta varios valores (D-15). |
| medicamentos | ✔ con un dato derivado controlado | `cantidad_total_disponible` = suma de lotes, mantenida por trigger (#8). |
| lotes | ✔ con un dato derivado | `cantidad` = entradas − consumos (D-7). |

---

## 10. Lo que no se pudo revisar

- **Requisitos (punto 8):** el SRS no se adjuntó, así que no se comprobó cada RF contra la base. Lo único que se pudo confirmar es que RF-40 (trazabilidad de tratamientos) y RF-48 (notificada/resuelta) tienen sus columnas.
- **Módulos 2 y 3 en MySQL:** `02_pacientes_emergencias.sql` y `03_inventario_alertas.sql` no están en el proyecto.
- **El código del backend:** no se vio, así que el hallazgo 3 (PRAGMA) y el 25 (int64) hay que confirmarlos en el repo de Andrés.

---

## 11. Revisión de normalización (20/10/2026)

Se evaluó el documento *"HeartMed · Revisión de normalización de la BD"*, que propone 17 correcciones. El script con sus cambios `REV #n` no existía (el archivo que llegó era la versión del 19/10), así que cada cambio se implementó a partir del documento. En los scripts quedan marcados con `-- REV #n`.

**Resultado de las pruebas:**
- `sqlite3` carga el esquema y los datos sin errores, dos veces seguidas y también encima de la base del 19/10.
- Los 28 chequeos de `verificacion_heartmed.sql` dan 0. Son los 25 de antes más 3 nuevos (V26–V28).
- Se rechazaron **57 de 57** datos inválidos: 22 de la auditoría que siguen aplicando con el esquema nuevo y 35 nuevas, al menos una por cada REV.
- Se aceptaron **15 de 15** operaciones válidas.
- El diagrama v4 coincide con la base: 20 tablas y 0 diferencias.

### Veredicto de cada corrección

| REV | Corrección | Veredicto | Cómo quedó |
|---|---|---|---|
| 1 | `parametros_clinicos` rompe la 1FN | Problema real; **solución ajustada** | La propuesta era una tabla `parametros_control` con el valor como texto (`'120/80'`), que no deja comparar números. Quedaron 5 columnas con tipo en `controles_evolucion`: `presion_sistolica`, `presion_diastolica`, `frecuencia_cardiaca`, `temperatura` (REAL) y `saturacion_o2`, con rangos. La presión va completa o no va. |
| 2 | `alergias` rompe la 1FN | **Sí, con ajuste** | Nueva tabla `alergias` (una sustancia por fila, UNIQUE sin mayúsculas) con `id_medicamento` opcional; si está lleno, un trigger impide recetar ese medicamento. `historias_clinicas.alergias` se cambia por `alergias_revisadas` (0 = no se ha preguntado, 1 = ya se preguntó), que se pone en 1 sola al registrar una alergia. Por texto, "Penicilina" no coincide con "Amoxicilina 500 mg"; por eso la amoxicilina se liga a Jean Bart con su propia fila. |
| 3 | antecedentes/cirugías se solapan con eventos | Sí | Regla documentada: antes del ingreso → historia; después → `eventos_medicos`. Por eso la cirugía del brazo de Jean Bart (hecha en el barco) salió de `cirugias_previas`. |
| 4 | Tipos de emergencia repetidos en dos CHECK | Sí | `emergencias.tipo` es FK a `protocolos.tipo_emergencia`. |
| 5 | `estado_salud` derivado del último control | Sí | 3 triggers lo recalculan cuando un control se crea, cambia o se borra (un control viejo cargado tarde no pisa el estado), y otro rechaza cambiarlo a mano si hay controles. **Nota:** no es una "dependencia transitiva" como dice el documento, sino un **dato derivado** (redundancia controlada). |
| 6 | `lotes.cantidad` sin protección | **Sí, con ajuste** | Un lote nace en 0, cada movimiento lo recalcula, no se puede editar a mano y una salida mayor al stock se rechaza. **Ajuste:** el documento dejaba editar o borrar movimientos. Ahora son de solo lectura y un error se corrige con un movimiento contrario. Se agregó el tipo `BAJA` (vencidos o dañados) y el motivo es obligatorio en toda salida que no venga de un tratamiento. |
| 7 | Total del medicamento editable | Sí | Triggers rechazan cualquier valor distinto de la suma de lotes. |
| 8 | Cambiar medicamento rompía el CAMBIO #16 | Sí | Triggers en tratamientos y lotes. |
| 9 | Zona horaria mezclada | Sí | Todo en hora de Colombia: los DEFAULT usan `datetime('now', '-5 hours')` y las consultas de avisos también. |
| 10 | Textos obligatorios vacíos | Sí | `CHECK (trim(col) <> '')` en todas las columnas de texto obligatorias. |
| 11 | VARCHAR(n) sin validar | Sí | `CHECK (length(col) <= n)`. Junto con REV #10 queda una sola regla por columna: `chk_<tabla>_<col>_texto`. |
| 12 | `tipo_sangre` sin dominio | **Ajustado: lista decidida** | El documento dejaba la lista pendiente. Quedó `CHECK (tipo_sangre IN ('F','S','X','XF'))`, los grupos de One Piece. |
| 13 | Sin unidad de medida | Sí, con lista | `unidad_medida` con CHECK: UNIDAD, TABLETA, CAPSULA, AMPOLLA, FRASCO, TUBO, ROLLO, ML. |
| 14 | Acciones fuera del horario | Sí | Triggers en acciones y en emergencias. Por eso a una emergencia FINALIZADA no se le agregan acciones posteriores. |
| 15 | Correo con espacios | Sí | `instr(correo, ' ') = 0`. |
| 16 | Logs de PK compuestas | Sí | Convención documentada y un log de ejemplo (`id_rol=5;id_permiso=11`). |
| 17 | Índices de emergencias | Sí | `idx_emergencias_tipo` e `idx_emergencias_estado`. |

**Agregados que el documento recomendaba sin aplicar:**
- **REV #18** `tratamientos.intervalo_horas`, para que el backend calcule la próxima dosis.
- **REV #19** CHECK de `tipo_insumo` con los valores MEDICAMENTO e INSUMO.
- **REV #20** movimientos de solo lectura y tipo `BAJA`.

Lo que **no** se aplicó:
- Tablas STRICT: obligarían a cambiar todos los tipos del diagrama.
- Catálogo de `rango_cargo`: es texto descriptivo.
- Trigger de historia automática (D-5): sigue pendiente porque choca con el backend actual.

### Decisiones del informe que quedaron resueltas

| Decisión | Quedó así |
|---|---|
| D-2 tipo de sangre | F, S, X, XF (REV #12) |
| D-3 tipo_insumo | MEDICAMENTO, INSUMO (REV #19) |
| D-4 zona horaria | Hora de Colombia (REV #9) |
| D-6 estado_salud | Triggers (REV #5) |
| D-7 lotes.cantidad | Triggers desde movimientos, más BAJA y motivo obligatorio (REV #6, #20) |
| D-12 alergias | Tabla `alergias` con bloqueo de receta (REV #2) |
| D-15 parametros_clinicos | Columnas con tipo (REV #1) |

Siguen pendientes: D-1 (`dias_aviso_vencimiento`), D-5 (historia automática), D-8 (un tratamiento activo por medicamento), D-9 (diagnóstico NULL), D-10 (dominio de `accion`), D-11 (Law como paciente), D-13 (MySQL obsoleto) y D-14 (renombrar columnas).

### Normalización después de la revisión

- `controles_evolucion` y `historias_clinicas` ya cumplen 1FN: los signos vitales son columnas y las alergias son filas.
- Quedan **tres datos derivados a propósito**, todos mantenidos por triggers y de solo lectura:
  - `pacientes.estado_salud`
  - `lotes.cantidad`
  - `medicamentos.cantidad_total_disponible`

Todo lo demás está en 3FN.

### Qué tiene que cambiar el backend (avisar al equipo)

| Módulo | Cambio | Responsable |
|---|---|---|
| Pacientes | `historias_clinicas.alergias` **ya no existe**. Ahora se insertan filas en `alergias` (y `alergias_revisadas` = 1 cuando se pregunta y no tiene). | Juan Salazar |
| Pacientes | `controles_evolucion.parametros_clinicos` **ya no existe**. Ahora se usan las 5 columnas de signos vitales. | Juan Salazar |
| Pacientes | Con controles registrados, `estado_salud` no se cambia con UPDATE: se registra un control. | Juan Salazar |
| Emergencias | El tipo debe existir en `protocolos`, y las acciones van dentro del horario de la emergencia. | Juan Salazar |
| Inventario | Un lote se crea sin cantidad y luego se registra una ENTRADA. Nunca se escribe `lotes.cantidad` ni `cantidad_total_disponible`. | Daniel |
| Inventario | Los movimientos no se editan ni se borran; para corregir, se registra un movimiento contrario. Las salidas sin tratamiento llevan motivo. Existe el tipo `BAJA`. | Daniel |
| Inventario | `unidad_medida` en medicamentos e `intervalo_horas` en tratamientos (opcional). Si el paciente es alérgico, la base rechaza la receta. | Daniel |
| Todos | Fechas en **hora de Colombia**. Lo más fácil es no enviar la fecha y dejar que la ponga el DEFAULT. | Todos |
