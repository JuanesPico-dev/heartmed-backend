-- =====================================================================
-- HeartMed - Consultas de verificación (SQLite)
-- Ejecutar DESPUÉS de cargar la estructura y los datos:
--   sqlite3 heartmed_db.sqlite3 ".read verificacion_heartmed.sql"
-- o abrir el archivo en VS Code (extensión SQLite) y ejecutarlo.
--
-- Cómo leer los resultados:
--   * PRAGMA integrity_check  -> debe decir "ok".
--   * PRAGMA foreign_key_check -> no debe devolver filas.
--   * Bloque 2 (resumen): TODAS las filas deben tener problemas = 0.
--     (V01-V25 auditoría del 19/10; V26-V28 revisión del 20/10.)
--   * Bloque 3 (detalle): cada consulta devuelve SOLO las filas con error.
--   * Bloque 4 (avisos): no son errores de la base, son cosas que el
--     equipo debería revisar (cambian con la fecha de hoy).
-- =====================================================================

PRAGMA foreign_keys = ON;

-- =====================================================================
-- 1. Salud del archivo y llaves foráneas
-- =====================================================================
PRAGMA foreign_keys;          -- debe dar 1
PRAGMA integrity_check;       -- debe dar ok
PRAGMA foreign_key_check;     -- no debe devolver filas

-- Conteo de filas por tabla
SELECT 'roles' AS tabla, COUNT(*) AS filas FROM roles
UNION ALL SELECT 'permisos', COUNT(*) FROM permisos
UNION ALL SELECT 'rol_permiso', COUNT(*) FROM rol_permiso
UNION ALL SELECT 'usuarios', COUNT(*) FROM usuarios
UNION ALL SELECT 'logs_auditoria', COUNT(*) FROM logs_auditoria
UNION ALL SELECT 'protocolos', COUNT(*) FROM protocolos
UNION ALL SELECT 'pasos_protocolo', COUNT(*) FROM pasos_protocolo
UNION ALL SELECT 'pacientes', COUNT(*) FROM pacientes
UNION ALL SELECT 'historias_clinicas', COUNT(*) FROM historias_clinicas
UNION ALL SELECT 'eventos_medicos', COUNT(*) FROM eventos_medicos
UNION ALL SELECT 'controles_evolucion', COUNT(*) FROM controles_evolucion
UNION ALL SELECT 'emergencias', COUNT(*) FROM emergencias
UNION ALL SELECT 'emergencia_paciente', COUNT(*) FROM emergencia_paciente
UNION ALL SELECT 'acciones_emergencia', COUNT(*) FROM acciones_emergencia
UNION ALL SELECT 'medicamentos', COUNT(*) FROM medicamentos
UNION ALL SELECT 'lotes', COUNT(*) FROM lotes
UNION ALL SELECT 'tratamientos', COUNT(*) FROM tratamientos
UNION ALL SELECT 'movimientos_inventario', COUNT(*) FROM movimientos_inventario
UNION ALL SELECT 'alertas', COUNT(*) FROM alertas
UNION ALL SELECT 'alergias', COUNT(*) FROM alergias;


-- =====================================================================
-- 2. RESUMEN: todas las filas deben dar problemas = 0
-- =====================================================================
SELECT 'V01 pacientes sin historia clínica' AS chequeo,
       (SELECT COUNT(*) FROM pacientes p
         WHERE NOT EXISTS (SELECT 1 FROM historias_clinicas h WHERE h.id_paciente = p.id_paciente)) AS problemas
UNION ALL
SELECT 'V02 lotes cuya cantidad no cuadra con sus movimientos',
       (SELECT COUNT(*) FROM lotes l
         WHERE l.cantidad <> (SELECT COALESCE(SUM(CASE m.tipo_movimiento WHEN 'ENTRADA' THEN m.cantidad ELSE -m.cantidad END), 0)
                                FROM movimientos_inventario m WHERE m.id_lote = l.id_lote))
UNION ALL
SELECT 'V03 lotes que quedaron en negativo en algún momento',
       (SELECT COUNT(DISTINCT id_lote) FROM (
            SELECT id_lote,
                   SUM(CASE tipo_movimiento WHEN 'ENTRADA' THEN cantidad ELSE -cantidad END)
                       OVER (PARTITION BY id_lote ORDER BY fecha_movimiento, id_movimiento) AS saldo
              FROM movimientos_inventario) WHERE saldo < 0)
UNION ALL
SELECT 'V04 medicamentos cuyo total no es la suma de sus lotes',
       (SELECT COUNT(*) FROM medicamentos md
         WHERE md.cantidad_total_disponible <> (SELECT COALESCE(SUM(l.cantidad), 0) FROM lotes l WHERE l.id_medicamento = md.id_medicamento))
UNION ALL
SELECT 'V05 pacientes cuyo estado no es el del último control',
       (SELECT COUNT(*) FROM pacientes p
          JOIN controles_evolucion c ON c.id_paciente = p.id_paciente
         WHERE c.id_control = (SELECT c2.id_control FROM controles_evolucion c2
                                WHERE c2.id_paciente = p.id_paciente
                                ORDER BY c2.fecha DESC, c2.id_control DESC LIMIT 1)
           AND c.estado <> p.estado_salud)
UNION ALL
SELECT 'V06 medicamentos bajo el mínimo sin alerta STOCK_BAJO abierta',
       (SELECT COUNT(*) FROM medicamentos md
         WHERE md.cantidad_total_disponible < md.umbral_minimo
           AND NOT EXISTS (SELECT 1 FROM alertas a WHERE a.tipo_alerta = 'STOCK_BAJO'
                              AND a.id_medicamento = md.id_medicamento AND a.resuelta = 0))
UNION ALL
SELECT 'V07 alertas STOCK_BAJO abiertas con stock suficiente',
       (SELECT COUNT(*) FROM alertas a JOIN medicamentos md ON md.id_medicamento = a.id_medicamento
         WHERE a.tipo_alerta = 'STOCK_BAJO' AND a.resuelta = 0
           AND md.cantidad_total_disponible >= md.umbral_minimo)
UNION ALL
SELECT 'V08 alertas con la referencia equivocada para su tipo',
       (SELECT COUNT(*) FROM alertas
         WHERE NOT ((tipo_alerta = 'STOCK_BAJO' AND id_medicamento IS NOT NULL AND id_lote IS NULL AND id_tratamiento IS NULL)
                 OR (tipo_alerta = 'VENCIMIENTO_PROXIMO' AND id_lote IS NOT NULL AND id_medicamento IS NULL AND id_tratamiento IS NULL)
                 OR (tipo_alerta = 'DOSIS_PENDIENTE' AND id_tratamiento IS NOT NULL AND id_medicamento IS NULL AND id_lote IS NULL)))
UNION ALL
SELECT 'V09 emergencias sin ningún paciente',
       (SELECT COUNT(*) FROM emergencias e
         WHERE NOT EXISTS (SELECT 1 FROM emergencia_paciente ep WHERE ep.id_emergencia = e.id_emergencia))
UNION ALL
SELECT 'V10 acciones fuera del horario de su emergencia',
       (SELECT COUNT(*) FROM acciones_emergencia a JOIN emergencias e ON e.id_emergencia = a.id_emergencia
         WHERE a.fecha_hora < e.fecha_inicio OR (e.fecha_fin IS NOT NULL AND a.fecha_hora > e.fecha_fin))
UNION ALL
SELECT 'V11 eventos médicos anteriores a la creación de la historia',
       (SELECT COUNT(*) FROM eventos_medicos ev JOIN historias_clinicas h ON h.id_historia = ev.id_historia
         WHERE ev.fecha_evento < h.fecha_creacion)
UNION ALL
SELECT 'V12 consumos de un lote ya vencido',
       (SELECT COUNT(*) FROM movimientos_inventario m JOIN lotes l ON l.id_lote = m.id_lote
         WHERE m.tipo_movimiento = 'CONSUMO' AND date(m.fecha_movimiento) > l.fecha_vencimiento)
UNION ALL
SELECT 'V13 consumos de tratamiento antes de que empiece el tratamiento',
       (SELECT COUNT(*) FROM movimientos_inventario m JOIN tratamientos t ON t.id_tratamiento = m.id_tratamiento
         WHERE date(m.fecha_movimiento) < t.fecha_inicio)
UNION ALL
SELECT 'V14 consumos cuyo lote no es del medicamento del tratamiento',
       (SELECT COUNT(*) FROM movimientos_inventario m
          JOIN lotes l ON l.id_lote = m.id_lote
          JOIN tratamientos t ON t.id_tratamiento = m.id_tratamiento
         WHERE l.id_medicamento <> t.id_medicamento)
UNION ALL
SELECT 'V15 tratamientos registrados por alguien sin PRESCRIBIR_TRATAMIENTOS',
       (SELECT COUNT(*) FROM tratamientos t JOIN usuarios u ON u.id_usuario = t.id_usuario_registra
         WHERE NOT EXISTS (SELECT 1 FROM rol_permiso rp JOIN permisos pe ON pe.id_permiso = rp.id_permiso
                            WHERE rp.id_rol = u.id_rol AND pe.cod_acceso = 'PRESCRIBIR_TRATAMIENTOS'))
UNION ALL
SELECT 'V16 eventos o controles registrados por alguien sin REGISTRAR_EVENTOS',
       (SELECT COUNT(*) FROM (
            SELECT id_usuario_registra AS u FROM eventos_medicos
            UNION ALL SELECT id_usuario_registra FROM controles_evolucion) x
          JOIN usuarios u ON u.id_usuario = x.u
         WHERE NOT EXISTS (SELECT 1 FROM rol_permiso rp JOIN permisos pe ON pe.id_permiso = rp.id_permiso
                            WHERE rp.id_rol = u.id_rol AND pe.cod_acceso = 'REGISTRAR_EVENTOS'))
UNION ALL
SELECT 'V17 acciones de emergencia de alguien sin GESTIONAR_EMERGENCIAS',
       (SELECT COUNT(*) FROM acciones_emergencia a JOIN usuarios u ON u.id_usuario = a.id_usuario_ejecutor
         WHERE NOT EXISTS (SELECT 1 FROM rol_permiso rp JOIN permisos pe ON pe.id_permiso = rp.id_permiso
                            WHERE rp.id_rol = u.id_rol AND pe.cod_acceso = 'GESTIONAR_EMERGENCIAS'))
UNION ALL
SELECT 'V18 movimientos de inventario de alguien sin GESTIONAR_INVENTARIO',
       (SELECT COUNT(*) FROM movimientos_inventario m JOIN usuarios u ON u.id_usuario = m.id_usuario
         WHERE NOT EXISTS (SELECT 1 FROM rol_permiso rp JOIN permisos pe ON pe.id_permiso = rp.id_permiso
                            WHERE rp.id_rol = u.id_rol AND pe.cod_acceso = 'GESTIONAR_INVENTARIO'))
UNION ALL
SELECT 'V19 registros hechos por usuarios inactivos',
       (SELECT COUNT(*) FROM (
            SELECT id_usuario_registra AS u FROM eventos_medicos
            UNION ALL SELECT id_usuario_registra FROM controles_evolucion
            UNION ALL SELECT id_usuario_ejecutor FROM acciones_emergencia
            UNION ALL SELECT id_usuario_registra FROM tratamientos
            UNION ALL SELECT id_usuario FROM movimientos_inventario) x
          JOIN usuarios u ON u.id_usuario = x.u
         WHERE u.activo = 0)
UNION ALL
SELECT 'V20 tipos de emergencia sin protocolo o protocolos sin pasos',
       (SELECT COUNT(*) FROM (SELECT 'TRAUMATISMO' AS t UNION ALL SELECT 'HEMORRAGIA' UNION ALL
                              SELECT 'INTOXICACION' UNION ALL SELECT 'QUEMADURA' UNION ALL SELECT 'OTRO') tipos
         WHERE NOT EXISTS (SELECT 1 FROM protocolos pr JOIN pasos_protocolo pp ON pp.id_protocolo = pr.id_protocolo
                            WHERE pr.tipo_emergencia = tipos.t))
UNION ALL
SELECT 'V21 protocolos con pasos que no van 1, 2, 3... seguidos',
       (SELECT COUNT(*) FROM (SELECT id_protocolo FROM pasos_protocolo
                               GROUP BY id_protocolo HAVING MIN(numero) <> 1 OR MAX(numero) <> COUNT(*)))
UNION ALL
SELECT 'V22 duplicados sin distinguir mayúsculas (correo, documento, medicamento)',
       (SELECT COUNT(*) FROM (SELECT lower(correo) FROM usuarios GROUP BY lower(correo) HAVING COUNT(*) > 1)) +
       (SELECT COUNT(*) FROM (SELECT lower(documento_identidad) FROM pacientes GROUP BY lower(documento_identidad) HAVING COUNT(*) > 1)) +
       (SELECT COUNT(*) FROM (SELECT lower(nombre) FROM medicamentos GROUP BY lower(nombre) HAVING COUNT(*) > 1))
UNION ALL
SELECT 'V23 logs que apuntan a un registro que no existe',
       (SELECT COUNT(*) FROM logs_auditoria lg
         WHERE lg.id_registro IS NOT NULL
           AND CASE lg.tabla_afectada
                 WHEN 'usuarios'               THEN NOT EXISTS (SELECT 1 FROM usuarios WHERE id_usuario = lg.id_registro)
                 WHEN 'protocolos'             THEN NOT EXISTS (SELECT 1 FROM protocolos WHERE id_protocolo = lg.id_registro)
                 WHEN 'pasos_protocolo'        THEN NOT EXISTS (SELECT 1 FROM pasos_protocolo WHERE id_paso = lg.id_registro)
                 WHEN 'pacientes'              THEN NOT EXISTS (SELECT 1 FROM pacientes WHERE id_paciente = lg.id_registro)
                 WHEN 'emergencias'            THEN NOT EXISTS (SELECT 1 FROM emergencias WHERE id_emergencia = lg.id_registro)
                 WHEN 'tratamientos'           THEN NOT EXISTS (SELECT 1 FROM tratamientos WHERE id_tratamiento = lg.id_registro)
                 WHEN 'movimientos_inventario' THEN NOT EXISTS (SELECT 1 FROM movimientos_inventario WHERE id_movimiento = lg.id_registro)
                 WHEN 'alertas'                THEN NOT EXISTS (SELECT 1 FROM alertas WHERE id_alerta = lg.id_registro)
                 ELSE 0
               END)
UNION ALL
SELECT 'V24 fechas con formato distinto a AAAA-MM-DD [HH:MM:SS]',
       (SELECT COUNT(*) FROM usuarios        WHERE datetime(fecha_creacion) IS NOT fecha_creacion) +
       (SELECT COUNT(*) FROM logs_auditoria  WHERE datetime(fecha_modificacion) IS NOT fecha_modificacion) +
       (SELECT COUNT(*) FROM pacientes       WHERE date(fecha_nacimiento) IS NOT fecha_nacimiento) +
       (SELECT COUNT(*) FROM historias_clinicas WHERE datetime(fecha_creacion) IS NOT fecha_creacion) +
       (SELECT COUNT(*) FROM eventos_medicos WHERE datetime(fecha_evento) IS NOT fecha_evento) +
       (SELECT COUNT(*) FROM controles_evolucion WHERE datetime(fecha) IS NOT fecha) +
       (SELECT COUNT(*) FROM emergencias     WHERE datetime(fecha_inicio) IS NOT fecha_inicio OR datetime(fecha_fin) IS NOT fecha_fin) +
       (SELECT COUNT(*) FROM acciones_emergencia WHERE datetime(fecha_hora) IS NOT fecha_hora) +
       (SELECT COUNT(*) FROM lotes           WHERE date(fecha_vencimiento) IS NOT fecha_vencimiento) +
       (SELECT COUNT(*) FROM tratamientos    WHERE date(fecha_inicio) IS NOT fecha_inicio OR date(fecha_fin) IS NOT fecha_fin
                                                OR date(fecha_finalizacion_efectiva) IS NOT fecha_finalizacion_efectiva
                                                OR datetime(fecha_registro) IS NOT fecha_registro) +
       (SELECT COUNT(*) FROM movimientos_inventario WHERE datetime(fecha_movimiento) IS NOT fecha_movimiento) +
       (SELECT COUNT(*) FROM alertas         WHERE datetime(fecha_generacion) IS NOT fecha_generacion
                                                OR datetime(fecha_resolucion) IS NOT fecha_resolucion)
UNION ALL
SELECT 'V25 números guardados como texto o decimal',
       (SELECT COUNT(*) FROM lotes WHERE typeof(cantidad) <> 'integer') +
       (SELECT COUNT(*) FROM movimientos_inventario WHERE typeof(cantidad) <> 'integer') +
       (SELECT COUNT(*) FROM medicamentos WHERE typeof(cantidad_total_disponible) <> 'integer' OR typeof(umbral_minimo) <> 'integer') +
       (SELECT COUNT(*) FROM pacientes WHERE typeof(recompensa) <> 'integer') +
       (SELECT COUNT(*) FROM pasos_protocolo WHERE typeof(numero) <> 'integer')
UNION ALL
SELECT 'V26 tratamientos de un medicamento al que el paciente es alérgico',
       (SELECT COUNT(*) FROM tratamientos t
          JOIN historias_clinicas h ON h.id_paciente = t.id_paciente
          JOIN alergias a ON a.id_historia = h.id_historia AND a.id_medicamento = t.id_medicamento)
UNION ALL
SELECT 'V27 historias con alergias registradas pero alergias_revisadas = 0',
       (SELECT COUNT(*) FROM historias_clinicas h
         WHERE h.alergias_revisadas = 0
           AND EXISTS (SELECT 1 FROM alergias a WHERE a.id_historia = h.id_historia))
UNION ALL
SELECT 'V28 tipos de emergencia del CHECK de protocolos sin fila en protocolos',
       (SELECT COUNT(*) FROM (SELECT 'TRAUMATISMO' AS t UNION ALL SELECT 'HEMORRAGIA' UNION ALL
                              SELECT 'INTOXICACION' UNION ALL SELECT 'QUEMADURA' UNION ALL SELECT 'OTRO') tipos
         WHERE NOT EXISTS (SELECT 1 FROM protocolos pr WHERE pr.tipo_emergencia = tipos.t));


-- =====================================================================
-- 3. DETALLE: cada consulta devuelve solo las filas con problema
-- =====================================================================

-- V01 pacientes sin historia clínica
SELECT p.id_paciente, p.nombre_completo
  FROM pacientes p
 WHERE NOT EXISTS (SELECT 1 FROM historias_clinicas h WHERE h.id_paciente = p.id_paciente);

-- V02 cuadre de cada lote contra sus movimientos
SELECT l.id_lote, l.num_lote, l.cantidad AS cantidad_en_lote,
       COALESCE(SUM(CASE m.tipo_movimiento WHEN 'ENTRADA' THEN m.cantidad ELSE -m.cantidad END), 0) AS segun_movimientos
  FROM lotes l
  LEFT JOIN movimientos_inventario m ON m.id_lote = l.id_lote
 GROUP BY l.id_lote
HAVING cantidad_en_lote <> segun_movimientos;

-- V04 total de cada medicamento contra sus lotes
SELECT md.id_medicamento, md.nombre, md.cantidad_total_disponible,
       COALESCE(SUM(l.cantidad), 0) AS suma_lotes
  FROM medicamentos md
  LEFT JOIN lotes l ON l.id_medicamento = md.id_medicamento
 GROUP BY md.id_medicamento
HAVING md.cantidad_total_disponible <> suma_lotes;

-- V05 estado del paciente contra su último control
SELECT p.id_paciente, p.nombre_completo, p.estado_salud, c.estado AS estado_ultimo_control, c.fecha
  FROM pacientes p
  JOIN controles_evolucion c ON c.id_paciente = p.id_paciente
 WHERE c.id_control = (SELECT c2.id_control FROM controles_evolucion c2
                        WHERE c2.id_paciente = p.id_paciente
                        ORDER BY c2.fecha DESC, c2.id_control DESC LIMIT 1)
   AND c.estado <> p.estado_salud;

-- V10 acciones fuera del horario de su emergencia
SELECT a.id_accion, a.id_emergencia, a.fecha_hora, e.fecha_inicio, e.fecha_fin
  FROM acciones_emergencia a
  JOIN emergencias e ON e.id_emergencia = a.id_emergencia
 WHERE a.fecha_hora < e.fecha_inicio OR (e.fecha_fin IS NOT NULL AND a.fecha_hora > e.fecha_fin);

-- V23 logs que apuntan a un registro que no existe
SELECT lg.id_log, lg.tabla_afectada, lg.id_registro, lg.detalle_cambio
  FROM logs_auditoria lg
 WHERE lg.id_registro IS NOT NULL
   AND CASE lg.tabla_afectada
         WHEN 'usuarios'        THEN NOT EXISTS (SELECT 1 FROM usuarios WHERE id_usuario = lg.id_registro)
         WHEN 'protocolos'      THEN NOT EXISTS (SELECT 1 FROM protocolos WHERE id_protocolo = lg.id_registro)
         WHEN 'pasos_protocolo' THEN NOT EXISTS (SELECT 1 FROM pasos_protocolo WHERE id_paso = lg.id_registro)
         WHEN 'tratamientos'    THEN NOT EXISTS (SELECT 1 FROM tratamientos WHERE id_tratamiento = lg.id_registro)
         ELSE 0
       END;


-- =====================================================================
-- 4. AVISOS DE NEGOCIO (no son errores de la base; dependen de hoy)
-- REV #9: la base guarda hora de Colombia, por eso 'now' lleva '-5 hours'.
-- =====================================================================

-- A1 tratamientos ACTIVOS cuya fecha prevista de fin ya pasó
SELECT id_tratamiento, id_paciente, fecha_fin
  FROM tratamientos
 WHERE estado_tratamiento = 'ACTIVO' AND fecha_fin < date('now', '-5 hours');

-- A2 lotes con stock que vencen en los próximos 30 días sin alerta abierta
SELECT l.id_lote, l.num_lote, l.fecha_vencimiento, l.cantidad
  FROM lotes l
 WHERE l.cantidad > 0
   AND l.fecha_vencimiento BETWEEN date('now', '-5 hours') AND date('now', '-5 hours', '+30 days')
   AND NOT EXISTS (SELECT 1 FROM alertas a WHERE a.tipo_alerta = 'VENCIMIENTO_PROXIMO'
                      AND a.id_lote = l.id_lote AND a.resuelta = 0);

-- A3 lotes vencidos que todavía tienen stock (habría que darlos de baja, ver D-7)
SELECT id_lote, num_lote, fecha_vencimiento, cantidad
  FROM lotes
 WHERE cantidad > 0 AND fecha_vencimiento < date('now', '-5 hours');

-- A4 pacientes cuya cuenta no es de rol Tripulante (ver D-11)
SELECT p.id_paciente, p.nombre_completo, r.nombre_rol
  FROM pacientes p
  JOIN usuarios u ON u.id_usuario = p.id_usuario
  JOIN roles r ON r.id_rol = u.id_rol
 WHERE r.nombre_rol <> 'Tripulante';

-- A5 emergencias abiertas (ACTIVA o EN_ATENCION) desde hace más de 24 horas
SELECT id_emergencia, tipo, estado, fecha_inicio
  FROM emergencias
 WHERE estado <> 'FINALIZADA' AND fecha_inicio < datetime('now', '-5 hours', '-1 day');
