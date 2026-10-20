-- =====================================================================
-- HeartMed - Datos de prueba (SQLite)
-- Requiere: heartmed_sqlite.sql
--   sqlite3 heartmed_db.sqlite3 ".read heartmed_sqlite.sql" ".read heartmed_datos_sqlite.sql"
-- =====================================================================
-- Trae datos_01 (Seguridad y protocolos), los pacientes con sus historias
-- clínicas y (CAMBIO #12) datos de prueba para TODAS las tablas de los
-- módulos 2 y 3, que antes estaban vacías.
--
-- Usuarios disponibles:
--   1 Law (Capitán/Médico Principal)   2 Ikkaku (Cirujano Naval)
--   3 Shachi (Admin. de Inventario)     4 Penguin (Analista de Datos)
--   5 Bepo, 6 Jean Bart (Tripulantes)   7 Uni (Tripulante inactivo)
-- Contraseña de prueba para todos: Prueba123*
--
-- Pacientes:
--   1 Law (SANO)   2 Bepo (SANO, historia vacía)
--   3 Jean Bart (EN_RECUPERACION)   4 Clione (EN_TRATAMIENTO, sin cuenta)
--
-- Fechas: hora de Colombia (REV #9), formato 'AAAA-MM-DD HH:MM:SS' (CAMBIO #4).
--
-- Revisión del 20/10/2026 (REV #n), lo que cambió en este archivo:
--   * historias_clinicas ya no tiene la columna alergias: las alergias van
--     en la tabla alergias y la historia trae alergias_revisadas.
--   * controles_evolucion trae los signos vitales en columnas.
--   * medicamentos trae unidad_medida; tratamientos trae intervalo_horas.
--   * Los lotes se crean SIN cantidad (en 0). La cantidad de cada lote y el
--     total de cada medicamento salen solos de los movimientos.
--   * pacientes.estado_salud lo actualizan los controles.
-- Lo que la base no revisa sola se comprueba con verificacion_heartmed.sql.
-- =====================================================================

PRAGMA foreign_keys = ON;

BEGIN TRANSACTION;

-- =====================================================================
-- datos_01 - Seguridad y protocolos (Juan Pico)
-- =====================================================================
-- roles (los 5 roles definidos en el SRS)

INSERT INTO roles (id_rol, nombre_rol, descripcion) VALUES
(1, 'Capitán/Médico Principal',   'Máxima autoridad médica. Acceso total al sistema, gestiona usuarios y protocolos.'),
(2, 'Cirujano Naval',             'Atiende emergencias, registra eventos médicos y prescribe tratamientos.'),
(3, 'Administrador de Inventario','Gestiona medicamentos, lotes y movimientos de inventario.'),
(4, 'Analista de Datos',          'Consulta reportes y estadísticas. No modifica información clínica.'),
(5, 'Tripulante',                 'Paciente con cuenta de acceso. Consulta su propio perfil y tratamientos.');

-- permisos

INSERT INTO permisos (id_permiso, nombre_permiso, cod_acceso) VALUES
(1,  'Gestionar usuarios y roles',          'GESTIONAR_USUARIOS'),
(2,  'Gestionar protocolos de emergencia',  'GESTIONAR_PROTOCOLOS'),
(3,  'Registrar y editar pacientes',        'GESTIONAR_PACIENTES'),
(4,  'Consultar historias clínicas',        'VER_HISTORIA_CLINICA'),
(5,  'Registrar eventos y controles',       'REGISTRAR_EVENTOS'),
(6,  'Atender emergencias',                 'GESTIONAR_EMERGENCIAS'),
(7,  'Prescribir tratamientos',             'PRESCRIBIR_TRATAMIENTOS'),
(8,  'Gestionar inventario',                'GESTIONAR_INVENTARIO'),
(9,  'Consultar reportes y estadísticas',   'VER_REPORTES'),
(10, 'Consultar auditoría',                 'VER_AUDITORIA'),
(11, 'Consultar perfil propio',             'VER_PERFIL_PROPIO');

-- rol_permiso

-- Capitán/Médico Principal: todos los permisos
INSERT INTO rol_permiso (id_rol, id_permiso) VALUES
(1, 1), (1, 2), (1, 3), (1, 4), (1, 5), (1, 6),
(1, 7), (1, 8), (1, 9), (1, 10), (1, 11);

-- Cirujano Naval: parte clínica
INSERT INTO rol_permiso (id_rol, id_permiso) VALUES
(2, 3), (2, 4), (2, 5), (2, 6), (2, 7), (2, 11);

-- Administrador de Inventario
INSERT INTO rol_permiso (id_rol, id_permiso) VALUES
(3, 8), (3, 9), (3, 11);

-- Analista de Datos
INSERT INTO rol_permiso (id_rol, id_permiso) VALUES
(4, 9), (4, 10), (4, 11);

-- Tripulante
INSERT INTO rol_permiso (id_rol, id_permiso) VALUES
(5, 11);

-- usuarios
-- Contraseña de prueba para todos: Prueba123*
-- El hash es su SHA-256 (verificado), ya calculado porque SQLite no tiene SHA2().
-- En el sistema real el backend debe guardar el hash con bcrypt o argon2.

INSERT INTO usuarios (id_usuario, id_rol, nombre, correo, contrasena_hash, activo, fecha_creacion) VALUES
(1, 1, 'Trafalgar D. Water Law', 'law@heartpirates.com',      '0f99d7bf097a4c7a9c3c0c033e5bb6bbf61bdcc19c4ccf3ca10c8c702089e007', 1, '2026-09-01 08:00:00'),
(2, 2, 'Ikkaku',                 'ikkaku@heartpirates.com',   '0f99d7bf097a4c7a9c3c0c033e5bb6bbf61bdcc19c4ccf3ca10c8c702089e007', 1, '2026-09-01 08:10:00'),
(3, 3, 'Shachi',                 'shachi@heartpirates.com',   '0f99d7bf097a4c7a9c3c0c033e5bb6bbf61bdcc19c4ccf3ca10c8c702089e007', 1, '2026-09-01 08:20:00'),
(4, 4, 'Penguin',                'penguin@heartpirates.com',  '0f99d7bf097a4c7a9c3c0c033e5bb6bbf61bdcc19c4ccf3ca10c8c702089e007', 1, '2026-09-01 08:30:00'),
(5, 5, 'Bepo',                   'bepo@heartpirates.com',     '0f99d7bf097a4c7a9c3c0c033e5bb6bbf61bdcc19c4ccf3ca10c8c702089e007', 1, '2026-09-02 09:00:00'),
(6, 5, 'Jean Bart',              'jeanbart@heartpirates.com', '0f99d7bf097a4c7a9c3c0c033e5bb6bbf61bdcc19c4ccf3ca10c8c702089e007', 1, '2026-09-02 09:15:00'),
(7, 5, 'Uni',                    'uni@heartpirates.com',      '0f99d7bf097a4c7a9c3c0c033e5bb6bbf61bdcc19c4ccf3ca10c8c702089e007', 0, '2026-09-03 10:00:00');

-- protocolos (uno por cada valor del CHECK de tipo_emergencia)
-- CAMBIO #26: antes decía "del ENUM", pero en SQLite es un CHECK.

INSERT INTO protocolos (id_protocolo, tipo_emergencia, nombre) VALUES
(1, 'TRAUMATISMO',  'Protocolo de atención de traumatismos'),
(2, 'HEMORRAGIA',   'Protocolo de control de hemorragias'),
(3, 'INTOXICACION', 'Protocolo de atención de intoxicaciones'),
(4, 'QUEMADURA',    'Protocolo de atención de quemaduras'),
(5, 'OTRO',         'Protocolo general de evaluación inicial');

-- pasos_protocolo
-- Los id_paso quedan: protocolo 1 -> 1..4, 2 -> 5..8, 3 -> 9..12,
-- 4 -> 13..16, 5 -> 17..19 (el log de auditoría usa el 7).

INSERT INTO pasos_protocolo (id_protocolo, numero, descripcion) VALUES
-- 1. Traumatismo
(1, 1, 'Asegurar la zona y evaluar si el paciente responde.'),
(1, 2, 'Inmovilizar cuello y columna si hay sospecha de lesión.'),
(1, 3, 'Revisar vía aérea, respiración y circulación.'),
(1, 4, 'Inmovilizar la extremidad afectada y trasladar a enfermería.'),
-- 2. Hemorragia
(2, 1, 'Aplicar presión directa sobre la herida con un apósito limpio.'),
(2, 2, 'Elevar la zona afectada si no hay fractura.'),
(2, 3, 'Aplicar torniquete si la hemorragia en extremidad no se controla.'),
(2, 4, 'Vigilar signos de shock y registrar la hora de cada maniobra.'),
-- 3. Intoxicación
(3, 1, 'Identificar la sustancia, la cantidad y la hora de exposición.'),
(3, 2, 'Retirar al paciente de la fuente de exposición.'),
(3, 3, 'No inducir el vómito sin indicación del médico.'),
(3, 4, 'Vigilar consciencia y respiración y preparar el antídoto si aplica.'),
-- 4. Quemadura
(4, 1, 'Retirar al paciente de la fuente de calor.'),
(4, 2, 'Enfriar la zona con agua a temperatura ambiente por 20 minutos.'),
(4, 3, 'Cubrir la quemadura con un apósito estéril sin adherir.'),
(4, 4, 'Evaluar extensión y profundidad para definir el tratamiento.'),
-- 5. Otro
(5, 1, 'Evaluar el estado de consciencia del paciente.'),
(5, 2, 'Revisar signos vitales y registrar síntomas.'),
(5, 3, 'Clasificar el nivel de urgencia y avisar al médico de turno.');

-- logs_auditoria
-- CAMBIO #26: el detalle decía "activo: TRUE -> FALSE"; en SQLite es 1 -> 0.

INSERT INTO logs_auditoria (id_usuario, accion, tabla_afectada, id_registro, detalle_cambio, fecha_modificacion) VALUES
(1, 'INSERT', 'usuarios',        2, 'Creó el usuario ikkaku@heartpirates.com con rol Cirujano Naval.', '2026-09-01 08:10:00'),
(1, 'INSERT', 'usuarios',        3, 'Creó el usuario shachi@heartpirates.com con rol Administrador de Inventario.', '2026-09-01 08:20:00'),
(1, 'INSERT', 'protocolos',      2, 'Creó el protocolo de control de hemorragias.', '2026-09-01 11:00:00'),
(1, 'UPDATE', 'pasos_protocolo', 7, 'Ajustó la descripción del paso 3 del protocolo de hemorragias.', '2026-09-02 14:30:00'),
(1, 'UPDATE', 'usuarios',        7, 'Desactivó el usuario uni@heartpirates.com (activo: 1 -> 0).', '2026-09-05 16:45:00'),
(2, 'LOGIN',  'usuarios',        2, 'Inicio de sesión exitoso.', '2026-09-06 07:55:00');

-- =====================================================================
-- Pacientes de prueba (agregados el 19/10/2026 para KAN-18 y KAN-19)
-- =====================================================================
-- id_usuario = cuenta de acceso del paciente (NULL si no tiene).
-- documento_identidad = número de documento del tripulante (no se repite).
-- tipo_sangre usa los grupos de One Piece (F, S, X, XF): REV #12.
-- estado_salud se escribe aquí porque todavía no tienen controles; desde
-- su primer control lo mantiene la base (REV #5).

INSERT INTO pacientes (id_paciente, id_usuario, documento_identidad, nombre_completo, foto_perfil, fecha_nacimiento, tipo_sangre, rango_cargo, recompensa, estado_salud) VALUES
(1, 1,    'HP-001', 'Trafalgar D. Water Law', NULL, '2000-10-06', 'F', 'Capitán',    3000000000, 'SANO'),
(2, 5,    'HP-002', 'Bepo',                   NULL, '2004-11-15', 'F', 'Navegante',  500,        'SANO'),
(3, 6,    'HP-003', 'Jean Bart',              NULL, '1995-05-20', 'X', 'Timonel',    0,          'EN_RECUPERACION'),
(4, NULL, 'HP-004', 'Clione',                 NULL, '1999-03-03', 'S', 'Tripulante', 0,          'EN_TRATAMIENTO');

-- historias_clinicas
-- La del paciente 2 (Bepo) se crea VACÍA, como pide la historia de usuario.
-- CAMBIO #11: faltaba la historia del paciente 4 (Clione). Todo paciente
-- debe tener exactamente una historia (relación 1 a 1 del diagrama).
-- REV #2: la columna alergias salió a la tabla alergias (más abajo, después
-- de medicamentos). alergias_revisadas:
--   Law (1) y Clione (4) -> 1 y sin filas = "ninguna alergia conocida".
--   Bepo (2)             -> 0 = todavía no se le ha preguntado.
--   Jean Bart (3)        -> 1, con sus alergias en la tabla alergias.
-- REV #3: antecedentes y cirugias_previas = solo lo de ANTES del ingreso.
-- La cirugía del brazo de Jean Bart fue en el barco (04/09), así que va en
-- eventos_medicos (evento 2) y no en cirugias_previas.
INSERT INTO historias_clinicas (id_historia, id_paciente, alergias_revisadas, antecedentes, cirugias_previas, observaciones, fruta_diablo, fecha_creacion) VALUES
(1, 1, 1, 'Síndrome del plomo ámbar (curado)', 'Ninguna', 'Usuario de la fruta Ope Ope.', 1, '2026-09-01 09:00:00'),
(2, 2, 0, NULL, NULL, NULL, 0, '2026-09-02 09:30:00'),
(3, 3, 1, NULL, NULL, 'Control semanal.', 0, '2026-09-03 10:00:00'),
(4, 4, 1, NULL, NULL, NULL, 0, '2026-10-12 14:00:00');


-- =====================================================================
-- CAMBIO #12: datos de prueba del MÓDULO 2 (antes vacío)
-- Historia que cuentan los datos:
--   * 03/09: Jean Bart se fractura el brazo (emergencia 1), lo operan el 04/09
--     y sigue en recuperación.
--   * 05/10: Law y Bepo se intoxican con pescado (emergencia 2, dos pacientes).
--   * 12/10: Clione llega con infección respiratoria y empieza tratamiento.
--   * 19/10: Clione se quema la mano (emergencia 3, EN_ATENCION) y Bepo
--     reporta mareo (emergencia 4, ACTIVA, todavía sin acciones).
-- =====================================================================

-- eventos_medicos (siempre después de la creación de la historia)
INSERT INTO eventos_medicos (id_evento, id_historia, id_usuario_registra, tipo_evento, motivo, diagnostico, fecha_evento) VALUES
(1, 3, 1, 'LESION',      'Herida en el brazo izquierdo durante un combate.',   'Fractura expuesta de cúbito izquierdo.',      '2026-09-03 10:30:00'),
(2, 3, 1, 'CIRUGIA',     'Reconstrucción quirúrgica del brazo izquierdo.',     'Cirugía exitosa. Inicia recuperación.',       '2026-09-04 08:00:00'),
(3, 1, 2, 'OBSERVACION', 'Chequeo general de rutina.',                         'Sin hallazgos.',                              '2026-10-01 10:00:00'),
(4, 4, 2, 'ENFERMEDAD',  'Fiebre y tos persistente desde hace 3 días.',        'Infección respiratoria bacteriana.',          '2026-10-12 15:00:00');

-- controles_evolucion
-- REV #1: signos vitales en columnas (presión en mmHg, FC en lpm, T en °C,
-- saturación en %). El control 2 no tomó saturación (queda NULL).
-- REV #5: al insertar cada control, la base copia el estado del último
-- control a pacientes.estado_salud.
INSERT INTO controles_evolucion (id_control, id_paciente, id_usuario_registra, fecha, estado, observaciones, presion_sistolica, presion_diastolica, frecuencia_cardiaca, temperatura, saturacion_o2) VALUES
(1, 3, 1, '2026-09-04 18:00:00', 'CRITICO',         'Postoperatorio inmediato, en observación.', 100, 60, 110, 37.8, 94),
(2, 3, 2, '2026-09-11 09:00:00', 'EN_RECUPERACION', 'Buena evolución de la herida.',             120, 80,  78, 36.6, NULL),
(3, 1, 2, '2026-10-01 10:15:00', 'SANO',            'Chequeo sin novedades.',                    120, 80,  68, 36.5, 99),
(4, 4, 2, '2026-10-12 15:30:00', 'EN_TRATAMIENTO',  'Inicia antibiótico.',                       118, 76,  92, 38.4, 96);

-- emergencias (cubre los 3 estados y 4 de los 5 tipos)
INSERT INTO emergencias (id_emergencia, tipo, sintomas, lesiones, diagnostico_inicial, nivel_urgencia, estado, fecha_inicio, fecha_fin) VALUES
(1, 'TRAUMATISMO',  'Dolor intenso y deformidad en el brazo izquierdo.',     'Fractura expuesta de cúbito.',               'Fractura expuesta',              'ALTA',  'FINALIZADA',  '2026-09-03 10:15:00', '2026-09-03 12:00:00'),
(2, 'INTOXICACION', 'Náuseas y mareo después de comer pescado.',             NULL,                                         'Intoxicación alimentaria leve',  'MEDIA', 'FINALIZADA',  '2026-10-05 20:00:00', '2026-10-05 22:30:00'),
(3, 'QUEMADURA',    'Ardor y enrojecimiento en la mano derecha.',            'Quemadura de primer grado en mano derecha.', 'Quemadura superficial',          'BAJA',  'EN_ATENCION', '2026-10-19 08:00:00', NULL),
(4, 'OTRO',         'Mareo y visión borrosa al levantarse.',                 NULL,                                         'Por evaluar',                    'MEDIA', 'ACTIVA',      '2026-10-19 21:00:00', NULL);

-- emergencia_paciente (la emergencia 2 tiene dos pacientes)
INSERT INTO emergencia_paciente (id_emergencia, id_paciente) VALUES
(1, 3),
(2, 1), (2, 2),
(3, 4),
(4, 2);

-- acciones_emergencia (todas entre fecha_inicio y fecha_fin de su emergencia)
INSERT INTO acciones_emergencia (id_accion, id_emergencia, id_usuario_ejecutor, descripcion_accion, fecha_hora) VALUES
(1, 1, 1, 'Zona asegurada; el paciente responde.',                          '2026-09-03 10:20:00'),
(2, 1, 1, 'Inmovilización del brazo y traslado a enfermería.',              '2026-09-03 10:40:00'),
(3, 2, 2, 'Se identificó la fuente: pescado en mal estado.',                '2026-10-05 20:10:00'),
(4, 2, 2, 'Hidratación oral y vigilancia durante dos horas.',               '2026-10-05 20:30:00'),
(5, 3, 2, 'Enfriamiento con agua a temperatura ambiente por 20 minutos.',   '2026-10-19 08:10:00');


-- =====================================================================
-- CAMBIO #12: datos de prueba del MÓDULO 3 (antes vacío)
-- REV #6: los lotes se crean en 0 y los movimientos les dan su cantidad.
-- Resultado que calcula la base (lotes.cantidad = entradas - consumos - bajas):
--   lote 1 IBU-2601: +80                 = 80
--   lote 2 IBU-2509: +40 -4 -21          = 15   (vence pronto -> alerta 2)
--   lote 3 AMX-2604: +30 -18             = 12   (bajo el mínimo -> alerta 1)
--   lote 4 VEN-2602: +10 -2              = 8
--   lote 5 SUL-2603: +5  -1              = 4
--   lote 6 VEN-2609: +17                 = 17   (resolvió la alerta 4)
-- =====================================================================

-- medicamentos
-- cantidad_total_disponible NO se escribe (REV #7): la calculan los
-- triggers desde los lotes. Resultado: 95, 12, 25 y 4.
-- REV #13: unidad_medida. REV #19: tipo_insumo MEDICAMENTO o INSUMO.
INSERT INTO medicamentos (id_medicamento, nombre, tipo_insumo, unidad_medida, descripcion, umbral_minimo) VALUES
(1, 'Ibuprofeno 400 mg',         'MEDICAMENTO', 'TABLETA', 'Analgésico y antiinflamatorio.',                 30),
(2, 'Amoxicilina 500 mg',        'MEDICAMENTO', 'CAPSULA', 'Antibiótico de la familia de las penicilinas.',  20),
(3, 'Venda elástica 10 cm',      'INSUMO',      'ROLLO',   'Rollo de venda elástica.',                       10),
(4, 'Sulfadiazina de plata 1 %', 'MEDICAMENTO', 'TUBO',    'Crema para quemaduras. Tubo de 50 g.',           3);

-- alergias (REV #2)
-- Jean Bart es alérgico a la penicilina. La amoxicilina es de la familia
-- de las penicilinas, así que se registra ligada al medicamento 2: desde
-- ahora la base NO deja recetársela (trigger trg_tratamientos_bi_alergia).
INSERT INTO alergias (id_alergia, id_historia, sustancia, id_medicamento) VALUES
(1, 3, 'Penicilina',  NULL),
(2, 3, 'Amoxicilina', 2);

-- lotes (REV #6: sin cantidad, nacen en 0)
INSERT INTO lotes (id_lote, id_medicamento, num_lote, ubicacion, fecha_vencimiento) VALUES
(1, 1, 'IBU-2601', 'Enfermería - Estante A', '2027-06-30'),
(2, 1, 'IBU-2509', 'Enfermería - Estante A', '2026-11-10'),
(3, 2, 'AMX-2604', 'Enfermería - Estante B', '2027-03-31'),
(4, 3, 'VEN-2602', 'Bodega - Caja 3',        '2029-12-31'),
(5, 4, 'SUL-2603', 'Enfermería - Estante C', '2027-01-15'),
(6, 3, 'VEN-2609', 'Bodega - Caja 3',        '2030-06-30');

-- tratamientos (cubre ACTIVO, FINALIZADO y SUSPENDIDO)
-- La amoxicilina es para Clione: a Jean Bart la base ya no deja recetársela.
-- REV #18: intervalo_horas = la frecuencia en número.
INSERT INTO tratamientos (id_tratamiento, id_paciente, id_medicamento, id_usuario_registra, dosis, frecuencia, intervalo_horas, fecha_inicio, fecha_fin, fecha_finalizacion_efectiva, estado_tratamiento, fecha_registro) VALUES
(1, 3, 1, 1, '1 tableta de 400 mg',             'Cada 8 horas',  8,  '2026-09-04', '2026-09-10', '2026-09-10', 'FINALIZADO', '2026-09-04 09:00:00'),
(2, 4, 2, 2, '1 cápsula de 500 mg',             'Cada 8 horas',  8,  '2026-10-12', '2026-10-21', NULL,         'ACTIVO',     '2026-10-12 15:40:00'),
(3, 4, 4, 2, 'Capa delgada sobre la quemadura', 'Cada 12 horas', 12, '2026-10-19', '2026-10-26', NULL,         'ACTIVO',     '2026-10-19 08:30:00'),
(4, 1, 1, 2, '1 tableta de 400 mg',             'Cada 12 horas', 12, '2026-10-05', '2026-10-08', '2026-10-06', 'SUSPENDIDO', '2026-10-05 21:00:00');

-- movimientos_inventario (en orden de fecha)
-- Cada fila actualiza sola la cantidad de su lote y el total de su medicamento.
-- Los CONSUMO sin tratamiento llevan motivo (REV #20).
INSERT INTO movimientos_inventario (id_movimiento, id_lote, id_usuario, id_tratamiento, tipo_movimiento, cantidad, fecha_movimiento, motivo) VALUES
(1,  2, 3, NULL, 'ENTRADA', 40, '2026-09-01 08:45:00', 'Compra inicial de inventario.'),
(2,  4, 3, NULL, 'ENTRADA', 10, '2026-09-02 09:30:00', 'Compra inicial de inventario.'),
(3,  2, 1, NULL, 'CONSUMO',  4, '2026-09-03 10:45:00', 'Uso en la emergencia 1 (Jean Bart).'),
(4,  4, 1, NULL, 'CONSUMO',  2, '2026-09-03 10:45:00', 'Uso en la emergencia 1 (Jean Bart).'),
(5,  2, 3, 1,    'CONSUMO', 21, '2026-09-04 09:30:00', 'Entrega del tratamiento 1 (7 días).'),
(6,  6, 3, NULL, 'ENTRADA', 17, '2026-09-05 08:30:00', 'Reposición de vendas.'),
(7,  1, 3, NULL, 'ENTRADA', 80, '2026-09-15 10:00:00', 'Compra de ibuprofeno.'),
(8,  3, 3, NULL, 'ENTRADA', 30, '2026-09-20 11:00:00', 'Compra de amoxicilina.'),
(9,  5, 3, NULL, 'ENTRADA',  5, '2026-09-20 11:05:00', 'Compra de sulfadiazina de plata.'),
(10, 3, 3, 2,    'CONSUMO', 18, '2026-10-12 16:00:00', 'Entrega de 6 días del tratamiento 2.'),
(11, 5, 3, 3,    'CONSUMO',  1, '2026-10-19 08:35:00', 'Entrega del tratamiento 3.');

-- alertas (cubre los 3 tipos, abiertas y resueltas)
INSERT INTO alertas (id_alerta, id_medicamento, id_lote, id_tratamiento, tipo_alerta, mensaje, prioridad, notificada, resuelta, fecha_generacion, fecha_resolucion) VALUES
(1, 2,    NULL, NULL, 'STOCK_BAJO',          'Amoxicilina 500 mg: quedan 12 unidades (mínimo 20).',       'ALTA',  1, 0, '2026-10-12 16:01:00', NULL),
(2, NULL, 2,    NULL, 'VENCIMIENTO_PROXIMO', 'El lote IBU-2509 de Ibuprofeno 400 mg vence el 2026-11-10.', 'MEDIA', 1, 0, '2026-10-11 06:00:00', NULL),
(3, NULL, NULL, 2,    'DOSIS_PENDIENTE',     'Clione: la dosis de amoxicilina de las 14:00 no se registró.', 'MEDIA', 1, 1, '2026-10-15 14:30:00', '2026-10-15 15:10:00'),
(4, 3,    NULL, NULL, 'STOCK_BAJO',          'Venda elástica 10 cm: quedan 8 unidades (mínimo 10).',      'MEDIA', 1, 1, '2026-09-03 10:50:00', '2026-09-05 09:05:00');

-- logs_auditoria de los datos nuevos (algunos ejemplos)
INSERT INTO logs_auditoria (id_usuario, accion, tabla_afectada, id_registro, detalle_cambio, fecha_modificacion) VALUES
(1, 'INSERT', 'emergencias',  1, 'Registró la emergencia 1 (TRAUMATISMO) de Jean Bart.',      '2026-09-03 10:15:00'),
(2, 'INSERT', 'tratamientos', 2, 'Prescribió Amoxicilina 500 mg a Clione.',                    '2026-10-12 15:40:00'),
(3, 'INSERT', 'movimientos_inventario', 10, 'Consumo de 18 unidades del lote AMX-2604.',      '2026-10-12 16:00:00'),
(2, 'UPDATE', 'alertas',      3, 'Resolvió la alerta de dosis pendiente de Clione.',          '2026-10-15 15:10:00'),
-- REV #16: tabla con PK compuesta -> id_registro NULL y la llave en el detalle.
(1, 'INSERT', 'rol_permiso',  NULL, 'id_rol=5;id_permiso=11',                                  '2026-09-01 08:05:00');

COMMIT;
