-- =====================================================================
-- HeartMed - Estructura completa de la base de datos (SQLite)
-- Une: 01_seguridad.sql (Juan Pico)
--      02_pacientes_emergencias.sql (Juan Salazar)
--      03_inventario_alertas.sql (Daniel Rodríguez)
-- Equipo Heart Pirates · Requiere SQLite 3.35 o superior
-- =====================================================================
-- Cómo ejecutarlo (crea el archivo heartmed_db.sqlite3):
--   sqlite3 heartmed_db.sqlite3 ".read heartmed_sqlite.sql" ".read heartmed_datos_sqlite.sql"
--
-- Cambios de sintaxis respecto a MySQL:
--   * No hay CREATE DATABASE ni USE: en SQLite la base es el archivo .db.
--   * INT AUTO_INCREMENT PRIMARY KEY  ->  INTEGER PRIMARY KEY AUTOINCREMENT
--   * ENUM(...)                       ->  TEXT + CHECK (columna IN (...))
--   * BOOLEAN                         ->  INTEGER + CHECK (0 o 1)
--   * Se quitó ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 (no existe en SQLite;
--     SQLite guarda todo el texto en UTF-8).
--   * SQLite no valida el largo de VARCHAR(n): desde la REV #11 lo valida
--     un CHECK length(col) <= n en cada columna.
-- Desde la auditoría del 19/10 la versión oficial es ESTE script; los .sql
-- de MySQL quedan como referencia histórica.
--
-- Cambios del 19/10/2026 (revisión de Juan Acevedo en la daily):
--   * pacientes.identificador_unico se renombró a documento_identidad.
--   * pacientes.id_usuario ahora es UNIQUE (una cuenta = un paciente).
--   * historias_clinicas: antecedentes, cirugias_previas y observaciones
--     aceptan NULL para poder crear la historia vacía.
--
-- Auditoría del 19/10/2026: cambios marcados "-- CAMBIO #n" (número del
-- hallazgo en informe_auditoria_heartmed.md, secciones 1 a 10).
--
-- Revisión de normalización del 20/10/2026: cambios marcados "-- REV #n"
-- (sección 11 del informe):
--   REV #1  controles_evolucion: parametros_clinicos (texto con varios valores,
--           rompía la 1FN) pasa a 5 columnas con tipo: presión sistólica y
--           diastólica, frecuencia cardiaca, temperatura y saturación.
--   REV #2  Nueva tabla alergias (una sustancia por fila, opcionalmente ligada
--           a un medicamento). Sale historias_clinicas.alergias y entra
--           alergias_revisadas. No se puede recetar un medicamento al que el
--           paciente es alérgico.
--   REV #3  antecedentes / cirugias_previas = solo lo ocurrido ANTES del ingreso.
--   REV #4  emergencias.tipo es FK a protocolos.tipo_emergencia (la lista de
--           tipos vive en un solo lugar).
--   REV #5  pacientes.estado_salud lo sincronizan triggers con el último control.
--   REV #6  lotes.cantidad la calculan triggers desde movimientos_inventario.
--   REV #7  medicamentos.cantidad_total_disponible no se puede escribir a mano.
--   REV #8  No se puede cambiar el medicamento de un lote o tratamiento con consumos.
--   REV #9  Todas las fechas en hora de Colombia (UTC-5), también los DEFAULT.
--   REV #10 Textos obligatorios no pueden ser vacíos.
--   REV #11 Largo máximo de cada VARCHAR(n) validado con CHECK.
--   REV #12 tipo_sangre solo acepta F, S, X, XF.
--   REV #13 medicamentos.unidad_medida.
--   REV #14 Las acciones caen dentro del horario de su emergencia.
--   REV #15 El correo no acepta espacios.
--   REV #16 Convención de logs para tablas con PK compuesta.
--   REV #17 Índices en emergencias (tipo y estado).
--   REV #18 tratamientos.intervalo_horas (para calcular la siguiente dosis).
--   REV #19 tipo_insumo solo acepta MEDICAMENTO o INSUMO.
--   REV #20 movimientos_inventario es de solo lectura, nuevo tipo BAJA y
--           motivo obligatorio en salidas que no vienen de un tratamiento.
--
-- IMPORTANTE PARA EL BACKEND:
--   1. Ejecutar PRAGMA foreign_keys = ON; en CADA conexión (SQLite trae las
--      FK apagadas). Si no, las FK, CASCADE y RESTRICT no se cumplen.
--   2. Fechas en HORA DE COLOMBIA (REV #9) y con este formato exacto:
--          DATE     -> 'AAAA-MM-DD'           ej. '2026-10-20'
--          DATETIME -> 'AAAA-MM-DD HH:MM:SS'  ej. '2026-10-20 08:30:00'
--      Lo más fácil: no enviar la fecha y dejar que la ponga el DEFAULT.
--   3. Los valores de los CHECK van en MAYÚSCULAS exactas ('SANO', no 'sano').
--   4. recompensa puede pasar de 2.147.483.647: en C++ usar int64_t con
--      sqlite3_bind_int64 / sqlite3_column_int64, nunca int.
--   5. Inventario (REV #6, #7, #20):
--        * Un lote se crea SIN cantidad (queda en 0) y luego se registra
--          un movimiento ENTRADA con la cantidad real.
--        * lotes.cantidad y medicamentos.cantidad_total_disponible solo se leen.
--        * Los movimientos no se editan ni se borran: un error se corrige
--          con un movimiento contrario (ENTRADA o BAJA) explicando el motivo.
--   6. pacientes.estado_salud (REV #5): cuando el paciente ya tiene controles,
--      el estado se cambia registrando un control nuevo, no con UPDATE.
--   7. Los mensajes de error traen el nombre de la regla
--      (ej. "CHECK constraint failed: chk_lotes_cantidad") o un texto en
--      español (triggers), para mostrarlos al usuario.
-- =====================================================================

PRAGMA foreign_keys = ON;

-- Borrar las tablas si ya existen (de la última a la primera),
-- para poder ejecutar el script varias veces.
-- Los índices y triggers se borran solos con su tabla.
DROP TABLE IF EXISTS alertas;
DROP TABLE IF EXISTS movimientos_inventario;
DROP TABLE IF EXISTS tratamientos;
DROP TABLE IF EXISTS alergias;
DROP TABLE IF EXISTS lotes;
DROP TABLE IF EXISTS medicamentos;
DROP TABLE IF EXISTS acciones_emergencia;
DROP TABLE IF EXISTS emergencia_paciente;
DROP TABLE IF EXISTS emergencias;
DROP TABLE IF EXISTS controles_evolucion;
DROP TABLE IF EXISTS eventos_medicos;
DROP TABLE IF EXISTS historias_clinicas;
DROP TABLE IF EXISTS pacientes;
DROP TABLE IF EXISTS pasos_protocolo;
DROP TABLE IF EXISTS protocolos;
DROP TABLE IF EXISTS logs_auditoria;
DROP TABLE IF EXISTS usuarios;
DROP TABLE IF EXISTS rol_permiso;
DROP TABLE IF EXISTS permisos;
DROP TABLE IF EXISTS roles;


-- #####################################################################
-- MÓDULO 1 - SEGURIDAD Y PROTOCOLOS (Juan Pico)
-- Tablas: roles, permisos, rol_permiso, usuarios, logs_auditoria,
--         protocolos, pasos_protocolo
-- #####################################################################

-- Convención de los CHECK de texto (CAMBIO #21, REV #10 y REV #11):
--   chk_<tabla>_<columna>_texto = no vacío (si es obligatoria) y largo máximo.

-- roles: cada usuario tiene un solo rol
CREATE TABLE roles (
    id_rol        INTEGER PRIMARY KEY AUTOINCREMENT,
    -- CAMBIO #6: COLLATE NOCASE ('tripulante' = 'Tripulante').
    nombre_rol    VARCHAR(50)  NOT NULL COLLATE NOCASE,
    descripcion   VARCHAR(300) NULL,
    CONSTRAINT uk_roles_nombre_rol UNIQUE (nombre_rol),
    CONSTRAINT chk_roles_nombre_rol_texto CHECK (trim(nombre_rol) <> '' AND length(nombre_rol) <= 50),
    CONSTRAINT chk_roles_descripcion_texto CHECK (length(descripcion) <= 300)
);

-- permisos: acciones que se pueden hacer en el sistema.
-- El backend valida por cod_acceso, no por id_permiso.
CREATE TABLE permisos (
    id_permiso      INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre_permiso  VARCHAR(100) NOT NULL,
    cod_acceso      VARCHAR(50)  NOT NULL COLLATE NOCASE,   -- CAMBIO #6
    CONSTRAINT uk_permisos_cod_acceso UNIQUE (cod_acceso),
    CONSTRAINT chk_permisos_nombre_permiso_texto CHECK (trim(nombre_permiso) <> '' AND length(nombre_permiso) <= 100),
    CONSTRAINT chk_permisos_cod_acceso_texto CHECK (trim(cod_acceso) <> '' AND length(cod_acceso) <= 50)
);

-- rol_permiso: qué permisos tiene cada rol (relación muchos a muchos)
CREATE TABLE rol_permiso (
    id_rol      INTEGER NOT NULL,
    id_permiso  INTEGER NOT NULL,
    CONSTRAINT pk_rol_permiso PRIMARY KEY (id_rol, id_permiso),
    CONSTRAINT fk_rol_permiso_roles
        FOREIGN KEY (id_rol) REFERENCES roles (id_rol)
        ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_rol_permiso_permisos
        FOREIGN KEY (id_permiso) REFERENCES permisos (id_permiso)
        ON DELETE CASCADE ON UPDATE CASCADE
);

-- usuarios: cuentas de acceso al sistema.
-- Un usuario no se borra: se desactiva con activo = 0.
CREATE TABLE usuarios (
    id_usuario       INTEGER PRIMARY KEY AUTOINCREMENT,
    id_rol           INTEGER      NOT NULL,
    nombre           VARCHAR(100) NOT NULL,
    -- CAMBIO #6: sin NOCASE se podían crear 'law@…' y 'LAW@…' como dos cuentas.
    correo           VARCHAR(150) NOT NULL COLLATE NOCASE,
    contrasena_hash  VARCHAR(255) NOT NULL,
    activo           INTEGER      NOT NULL DEFAULT 1
        CONSTRAINT chk_usuarios_activo CHECK (activo IN (0, 1)),
    -- REV #9: hora de Colombia.
    fecha_creacion   DATETIME     NOT NULL DEFAULT (datetime('now', '-5 hours')),
    CONSTRAINT uk_usuarios_correo UNIQUE (correo),
    CONSTRAINT chk_usuarios_nombre_texto CHECK (trim(nombre) <> '' AND length(nombre) <= 100),
    CONSTRAINT chk_usuarios_correo_texto CHECK (length(correo) <= 150),
    -- CAMBIO #21 + REV #15: forma algo@algo.algo y sin espacios.
    CONSTRAINT chk_usuarios_correo_formato
        CHECK (correo LIKE '_%@_%._%' AND instr(correo, ' ') = 0),
    CONSTRAINT chk_usuarios_contrasena_hash_texto
        CHECK (trim(contrasena_hash) <> '' AND length(contrasena_hash) <= 255),
    CONSTRAINT chk_usuarios_fecha_creacion_formato                      -- CAMBIO #4
        CHECK (datetime(fecha_creacion) IS fecha_creacion),
    CONSTRAINT fk_usuarios_roles
        FOREIGN KEY (id_rol) REFERENCES roles (id_rol)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- logs_auditoria: registro de cambios hechos por los usuarios (solo lectura).
-- tabla_afectada es VARCHAR(64), el largo máximo de un nombre de tabla en MySQL.
-- REV #16: para tablas con PK compuesta (rol_permiso, emergencia_paciente)
-- id_registro va en NULL y la llave completa va en detalle_cambio con el
-- formato "id_rol=2;id_permiso=5".
CREATE TABLE logs_auditoria (
    id_log              INTEGER PRIMARY KEY AUTOINCREMENT,
    id_usuario          INTEGER     NOT NULL,
    accion              VARCHAR(50) NOT NULL,
    tabla_afectada      VARCHAR(64) NOT NULL,
    id_registro         INTEGER     NULL,
    detalle_cambio      TEXT        NULL,
    fecha_modificacion  DATETIME    NOT NULL DEFAULT (datetime('now', '-5 hours')),   -- REV #9
    CONSTRAINT chk_logs_auditoria_accion_texto CHECK (trim(accion) <> '' AND length(accion) <= 50),
    CONSTRAINT chk_logs_auditoria_tabla_afectada_texto
        CHECK (trim(tabla_afectada) <> '' AND length(tabla_afectada) <= 64),
    CONSTRAINT chk_logs_auditoria_id_registro                           -- CAMBIO #5
        CHECK (typeof(id_registro) IN ('integer', 'null')),
    CONSTRAINT chk_logs_auditoria_fecha_modificacion_formato            -- CAMBIO #4
        CHECK (datetime(fecha_modificacion) IS fecha_modificacion),
    CONSTRAINT fk_logs_auditoria_usuarios
        FOREIGN KEY (id_usuario) REFERENCES usuarios (id_usuario)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- protocolos: un protocolo por cada tipo de emergencia.
-- REV #4: este CHECK es la ÚNICA lista de tipos de emergencia del sistema.
-- emergencias.tipo es FK a tipo_emergencia, así que para agregar un tipo
-- nuevo se cambia este CHECK y se inserta su protocolo.
CREATE TABLE protocolos (
    id_protocolo     INTEGER PRIMARY KEY AUTOINCREMENT,
    tipo_emergencia  TEXT NOT NULL
        CONSTRAINT chk_protocolos_tipo_emergencia
        CHECK (tipo_emergencia IN ('TRAUMATISMO', 'HEMORRAGIA', 'INTOXICACION',
                                   'QUEMADURA', 'OTRO')),
    nombre           VARCHAR(150) NOT NULL,
    CONSTRAINT uk_protocolos_tipo_emergencia UNIQUE (tipo_emergencia),
    CONSTRAINT chk_protocolos_nombre_texto CHECK (trim(nombre) <> '' AND length(nombre) <= 150)
);

-- pasos_protocolo: pasos ordenados de cada protocolo.
-- No se puede repetir el mismo número de paso en un protocolo.
CREATE TABLE pasos_protocolo (
    id_paso       INTEGER PRIMARY KEY AUTOINCREMENT,
    id_protocolo  INTEGER NOT NULL,
    numero        INTEGER NOT NULL,
    descripcion   TEXT    NOT NULL,
    CONSTRAINT uk_pasos_protocolo_numero UNIQUE (id_protocolo, numero),
    CONSTRAINT chk_pasos_protocolo_numero                               -- CAMBIO #5 y #20
        CHECK (typeof(numero) = 'integer' AND numero >= 1),
    CONSTRAINT chk_pasos_protocolo_descripcion_texto CHECK (trim(descripcion) <> ''),
    CONSTRAINT fk_pasos_protocolo_protocolos
        FOREIGN KEY (id_protocolo) REFERENCES protocolos (id_protocolo)
        ON DELETE CASCADE ON UPDATE CASCADE
);


-- #####################################################################
-- MÓDULO 2 - PACIENTES Y EMERGENCIAS (Juan Salazar)
-- Tablas: pacientes, historias_clinicas, alergias, eventos_medicos,
--         controles_evolucion, emergencias, emergencia_paciente,
--         acciones_emergencia
-- #####################################################################

-- pacientes
-- Hay tres identificadores y cada uno tiene un uso distinto:
--   id_paciente          -> llave primaria interna (la genera la BD).
--   id_usuario           -> cuenta de acceso del paciente, si tiene (puede ser NULL).
--   documento_identidad  -> número de documento del tripulante (como la cédula).
-- REV #5: estado_salud es un DATO DERIVADO (redundancia controlada): repite
-- el estado del último control de evolución para no tener que calcularlo en
-- cada consulta. Lo mantienen los triggers trg_controles_evolucion_*.
-- Mientras el paciente no tenga controles se escribe libre (al registrarlo).
CREATE TABLE pacientes (
    id_paciente             INTEGER PRIMARY KEY AUTOINCREMENT,
    id_usuario              INTEGER      NULL,
    documento_identidad     VARCHAR(50)  NOT NULL COLLATE NOCASE,       -- CAMBIO #6
    nombre_completo         VARCHAR(150) NOT NULL,
    foto_perfil             VARCHAR(255) NULL,
    fecha_nacimiento        DATE         NOT NULL,
    -- REV #12 (ajustado): grupos sanguíneos de One Piece.
    tipo_sangre             VARCHAR(5)   NOT NULL
        CONSTRAINT chk_pacientes_tipo_sangre CHECK (tipo_sangre IN ('F', 'S', 'X', 'XF')),
    rango_cargo             VARCHAR(50)  NOT NULL,
    recompensa              BIGINT       NOT NULL,
    estado_salud            TEXT         NOT NULL
        CONSTRAINT chk_pacientes_estado_salud
        CHECK (estado_salud IN ('SANO', 'EN_TRATAMIENTO', 'EN_RECUPERACION', 'CRITICO')),

    CONSTRAINT uk_pacientes_documento_identidad UNIQUE (documento_identidad),
    -- Una cuenta = un paciente (UNIQUE deja varios NULL: pacientes sin cuenta).
    CONSTRAINT uk_pacientes_id_usuario UNIQUE (id_usuario),

    CONSTRAINT chk_pacientes_documento_identidad_texto
        CHECK (trim(documento_identidad) <> '' AND length(documento_identidad) <= 50),
    CONSTRAINT chk_pacientes_nombre_completo_texto
        CHECK (trim(nombre_completo) <> '' AND length(nombre_completo) <= 150),
    CONSTRAINT chk_pacientes_foto_perfil_texto CHECK (length(foto_perfil) <= 255),
    CONSTRAINT chk_pacientes_rango_cargo_texto
        CHECK (trim(rango_cargo) <> '' AND length(rango_cargo) <= 50),
    CONSTRAINT chk_pacientes_fecha_nacimiento_formato                   -- CAMBIO #4
        CHECK (date(fecha_nacimiento) IS fecha_nacimiento),
    CONSTRAINT chk_pacientes_recompensa                                 -- CAMBIO #5 y #20
        CHECK (typeof(recompensa) = 'integer' AND recompensa >= 0),
    CONSTRAINT fk_pacientes_usuarios
        FOREIGN KEY (id_usuario) REFERENCES usuarios (id_usuario)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- historias_clinicas
-- La historia de usuario pide crearla VACÍA y llenarla después, por eso
-- antecedentes, cirugias_previas y observaciones aceptan NULL.
-- Regla de negocio: TODO paciente tiene exactamente una historia. La base
-- garantiza "máximo una" (UNIQUE); que exista la crea el backend al
-- registrar el paciente (decisión D-5 del informe, pendiente).
-- REV #3: antecedentes y cirugias_previas son SOLO lo ocurrido ANTES del
-- ingreso al sistema, en texto narrativo. Todo lo que pase después se
-- registra en eventos_medicos (tipo CIRUGIA, ENFERMEDAD, DIAGNOSTICO...).
-- REV #2: la columna alergias salió a la tabla alergias.
--   alergias_revisadas = 0 -> todavía no se le ha preguntado al paciente.
--   alergias_revisadas = 1 -> ya se preguntó; si no tiene filas en
--                             alergias, es "ninguna alergia conocida".
--   Se pone en 1 sola al registrar la primera alergia.
CREATE TABLE historias_clinicas (
    id_historia         INTEGER  PRIMARY KEY AUTOINCREMENT,
    id_paciente         INTEGER  NOT NULL,
    alergias_revisadas  INTEGER  NOT NULL DEFAULT 0
        CONSTRAINT chk_historias_clinicas_alergias_revisadas CHECK (alergias_revisadas IN (0, 1)),
    antecedentes        TEXT     NULL,
    cirugias_previas    TEXT     NULL,
    observaciones       TEXT     NULL,
    fruta_diablo        INTEGER  NOT NULL DEFAULT 0
        CONSTRAINT chk_historias_clinicas_fruta_diablo CHECK (fruta_diablo IN (0, 1)),
    fecha_creacion      DATETIME NOT NULL DEFAULT (datetime('now', '-5 hours')),   -- REV #9

    CONSTRAINT uk_historias_clinicas_id_paciente UNIQUE (id_paciente),
    CONSTRAINT chk_historias_clinicas_fecha_creacion_formato            -- CAMBIO #4
        CHECK (datetime(fecha_creacion) IS fecha_creacion),
    -- RESTRICT: una historia clínica nunca se borra en cascada.
    CONSTRAINT fk_historias_clinicas_pacientes
        FOREIGN KEY (id_paciente) REFERENCES pacientes (id_paciente)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- alergias (REV #2): una sustancia por fila (antes era una lista en un texto,
-- lo que rompía la 1FN).
-- id_medicamento (opcional) liga la alergia a un medicamento del inventario.
-- Si está lleno, la base NO deja recetarle ese medicamento al paciente
-- (trigger trg_tratamientos_bi_alergia). Ojo: "Penicilina" no se compara
-- sola contra "Amoxicilina 500 mg"; hay que ligar cada medicamento
-- peligroso con su propia fila.
-- La FK a medicamentos apunta a una tabla del módulo 3; en SQLite eso
-- funciona aunque medicamentos se cree más abajo.
CREATE TABLE alergias (
    id_alergia      INTEGER      PRIMARY KEY AUTOINCREMENT,
    id_historia     INTEGER      NOT NULL,
    sustancia       VARCHAR(100) NOT NULL COLLATE NOCASE,
    id_medicamento  INTEGER      NULL,
    CONSTRAINT uk_alergias_historia_sustancia UNIQUE (id_historia, sustancia),
    CONSTRAINT chk_alergias_sustancia_texto CHECK (trim(sustancia) <> '' AND length(sustancia) <= 100),
    -- CASCADE: la alergia es parte de la historia.
    CONSTRAINT fk_alergias_historias_clinicas
        FOREIGN KEY (id_historia) REFERENCES historias_clinicas (id_historia)
        ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_alergias_medicamentos
        FOREIGN KEY (id_medicamento) REFERENCES medicamentos (id_medicamento)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- eventos_medicos: lo que le pasa al paciente DESPUÉS de su ingreso (REV #3).
CREATE TABLE eventos_medicos (
    id_evento               INTEGER PRIMARY KEY AUTOINCREMENT,
    id_historia             INTEGER  NOT NULL,
    id_usuario_registra     INTEGER  NOT NULL,
    tipo_evento             TEXT     NOT NULL
        CONSTRAINT chk_eventos_medicos_tipo_evento
        CHECK (tipo_evento IN ('LESION', 'ENFERMEDAD', 'DIAGNOSTICO', 'CIRUGIA', 'OBSERVACION')),
    motivo                  TEXT     NOT NULL,
    diagnostico             TEXT     NOT NULL,
    fecha_evento            DATETIME NOT NULL,

    CONSTRAINT chk_eventos_medicos_motivo_texto CHECK (trim(motivo) <> ''),
    CONSTRAINT chk_eventos_medicos_diagnostico_texto CHECK (trim(diagnostico) <> ''),
    CONSTRAINT chk_eventos_medicos_fecha_evento_formato                 -- CAMBIO #4
        CHECK (datetime(fecha_evento) IS fecha_evento),
    CONSTRAINT fk_eventos_medicos_historias_clinicas
        FOREIGN KEY (id_historia) REFERENCES historias_clinicas (id_historia)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_eventos_medicos_usuarios
        FOREIGN KEY (id_usuario_registra) REFERENCES usuarios (id_usuario)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- controles_evolucion
-- REV #1 (ajustado): parametros_clinicos guardaba varios valores en un solo
-- texto (rompía la 1FN). Ahora cada signo vital es una columna con tipo y
-- rango. Las que no se midieron quedan en NULL.
-- El estado del último control se copia a pacientes.estado_salud (REV #5).
CREATE TABLE controles_evolucion (
    id_control              INTEGER PRIMARY KEY AUTOINCREMENT,
    id_paciente             INTEGER  NOT NULL,
    id_usuario_registra     INTEGER  NOT NULL,
    fecha                   DATETIME NOT NULL,
    -- Mismo dominio que pacientes.estado_salud.
    estado                  TEXT     NOT NULL
        CONSTRAINT chk_controles_evolucion_estado
        CHECK (estado IN ('SANO', 'EN_TRATAMIENTO', 'EN_RECUPERACION', 'CRITICO')),
    observaciones           TEXT     NOT NULL,
    presion_sistolica       INTEGER  NULL,   -- mmHg
    presion_diastolica      INTEGER  NULL,   -- mmHg
    frecuencia_cardiaca     INTEGER  NULL,   -- latidos por minuto
    temperatura             REAL     NULL,   -- °C
    saturacion_o2           INTEGER  NULL,   -- %

    CONSTRAINT chk_controles_evolucion_observaciones_texto CHECK (trim(observaciones) <> ''),
    -- La presión se registra completa (las dos cifras) o no se registra.
    CONSTRAINT chk_controles_evolucion_presion
        CHECK ((presion_sistolica IS NULL AND presion_diastolica IS NULL)
            OR (typeof(presion_sistolica) = 'integer' AND typeof(presion_diastolica) = 'integer'
                AND presion_sistolica  BETWEEN 40 AND 300
                AND presion_diastolica BETWEEN 20 AND 200
                AND presion_sistolica > presion_diastolica)),
    CONSTRAINT chk_controles_evolucion_frecuencia_cardiaca
        CHECK (frecuencia_cardiaca IS NULL
            OR (typeof(frecuencia_cardiaca) = 'integer' AND frecuencia_cardiaca BETWEEN 20 AND 250)),
    CONSTRAINT chk_controles_evolucion_temperatura
        CHECK (temperatura IS NULL
            OR (typeof(temperatura) = 'real' AND temperatura BETWEEN 30 AND 45)),
    CONSTRAINT chk_controles_evolucion_saturacion_o2
        CHECK (saturacion_o2 IS NULL
            OR (typeof(saturacion_o2) = 'integer' AND saturacion_o2 BETWEEN 50 AND 100)),
    CONSTRAINT chk_controles_evolucion_fecha_formato                    -- CAMBIO #4
        CHECK (datetime(fecha) IS fecha),
    CONSTRAINT fk_controles_evolucion_pacientes
        FOREIGN KEY (id_paciente) REFERENCES pacientes (id_paciente)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_controles_evolucion_usuarios
        FOREIGN KEY (id_usuario_registra) REFERENCES usuarios (id_usuario)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- emergencias
CREATE TABLE emergencias (
    id_emergencia           INTEGER PRIMARY KEY AUTOINCREMENT,
    -- REV #4: antes repetía en un CHECK la lista de protocolos.tipo_emergencia.
    -- Ahora es FK: solo acepta tipos que tienen protocolo.
    tipo                    TEXT     NOT NULL,
    sintomas                TEXT     NOT NULL,
    lesiones                TEXT     NULL,
    diagnostico_inicial     TEXT     NOT NULL,
    -- Mismo dominio que alertas.prioridad.
    nivel_urgencia          TEXT     NOT NULL
        CONSTRAINT chk_emergencias_nivel_urgencia
        CHECK (nivel_urgencia IN ('BAJA', 'MEDIA', 'ALTA', 'CRITICA')),
    estado                  TEXT     NOT NULL DEFAULT 'ACTIVA'
        CONSTRAINT chk_emergencias_estado
        CHECK (estado IN ('ACTIVA', 'EN_ATENCION', 'FINALIZADA')),
    fecha_inicio            DATETIME NOT NULL,
    fecha_fin               DATETIME NULL,

    CONSTRAINT chk_emergencias_sintomas_texto CHECK (trim(sintomas) <> ''),
    CONSTRAINT chk_emergencias_diagnostico_inicial_texto CHECK (trim(diagnostico_inicial) <> ''),
    CONSTRAINT chk_emergencias_fecha_inicio_formato                     -- CAMBIO #4
        CHECK (datetime(fecha_inicio) IS fecha_inicio),
    CONSTRAINT chk_emergencias_fecha_fin_formato
        CHECK (datetime(fecha_fin) IS fecha_fin),
    CONSTRAINT chk_emergencias_fechas                                   -- CAMBIO #9
        CHECK (fecha_fin IS NULL OR fecha_fin >= fecha_inicio),
    CONSTRAINT chk_emergencias_estado_fecha_fin                         -- CAMBIO #9
        CHECK ((estado = 'FINALIZADA' AND fecha_fin IS NOT NULL)
            OR (estado <> 'FINALIZADA' AND fecha_fin IS NULL)),
    CONSTRAINT fk_emergencias_protocolos                                -- REV #4
        FOREIGN KEY (tipo) REFERENCES protocolos (tipo_emergencia)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- emergencia_paciente: pacientes involucrados en cada emergencia
CREATE TABLE emergencia_paciente (
    id_emergencia   INTEGER NOT NULL,
    id_paciente     INTEGER NOT NULL,
    CONSTRAINT pk_emergencia_paciente PRIMARY KEY (id_emergencia, id_paciente),
    CONSTRAINT fk_emergencia_paciente_emergencias
        FOREIGN KEY (id_emergencia) REFERENCES emergencias (id_emergencia)
        ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_emergencia_paciente_pacientes
        FOREIGN KEY (id_paciente) REFERENCES pacientes (id_paciente)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- acciones_emergencia: lo que se hizo en cada emergencia y quién lo hizo.
-- REV #14: fecha_hora debe caer entre fecha_inicio y fecha_fin de la
-- emergencia (triggers trg_acciones_emergencia_* y trg_emergencias_bu_*).
-- Por eso a una emergencia FINALIZADA no se le agregan acciones posteriores.
CREATE TABLE acciones_emergencia (
    id_accion               INTEGER PRIMARY KEY AUTOINCREMENT,
    id_emergencia           INTEGER  NOT NULL,
    id_usuario_ejecutor     INTEGER  NOT NULL,
    descripcion_accion      TEXT     NOT NULL,
    fecha_hora              DATETIME NOT NULL DEFAULT (datetime('now', '-5 hours')),   -- REV #9

    CONSTRAINT chk_acciones_emergencia_descripcion_accion_texto CHECK (trim(descripcion_accion) <> ''),
    CONSTRAINT chk_acciones_emergencia_fecha_hora_formato               -- CAMBIO #4
        CHECK (datetime(fecha_hora) IS fecha_hora),
    CONSTRAINT fk_acciones_emergencia_emergencias
        FOREIGN KEY (id_emergencia) REFERENCES emergencias (id_emergencia)
        ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_acciones_emergencia_usuarios
        FOREIGN KEY (id_usuario_ejecutor) REFERENCES usuarios (id_usuario)
        ON DELETE RESTRICT ON UPDATE CASCADE
);


-- #####################################################################
-- MÓDULO 3 - INVENTARIO, TRATAMIENTOS Y ALERTAS (Daniel Rodríguez)
-- Tablas: medicamentos, lotes, tratamientos, movimientos_inventario,
--         alertas
--
-- Flujo del inventario (REV #6, #7 y #20):
--   movimientos_inventario  --(trigger)-->  lotes.cantidad
--   lotes.cantidad          --(trigger)-->  medicamentos.cantidad_total_disponible
-- Los movimientos son la única fuente de verdad. Las dos cantidades son
-- datos derivados que solo escriben los triggers.
-- #####################################################################

-- medicamentos: catálogo de medicamentos e insumos médicos.
-- umbral_minimo = si el total baja de este valor se genera una alerta de stock bajo.
CREATE TABLE medicamentos (
    id_medicamento             INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre                     VARCHAR(100) NOT NULL COLLATE NOCASE,   -- CAMBIO #6 y #7
    -- REV #19: lista cerrada (antes era texto libre).
    tipo_insumo                VARCHAR(50)  NOT NULL
        CONSTRAINT chk_medicamentos_tipo_insumo CHECK (tipo_insumo IN ('MEDICAMENTO', 'INSUMO')),
    -- REV #13: en qué se cuentan las cantidades de lotes, movimientos y umbral.
    unidad_medida              VARCHAR(20)  NOT NULL DEFAULT 'UNIDAD'
        CONSTRAINT chk_medicamentos_unidad_medida
        CHECK (unidad_medida IN ('UNIDAD', 'TABLETA', 'CAPSULA', 'AMPOLLA',
                                 'FRASCO', 'TUBO', 'ROLLO', 'ML')),
    descripcion                TEXT         NULL,
    -- REV #7: la calcula el trigger desde lotes; escribirla a mano se rechaza.
    cantidad_total_disponible  INTEGER      NOT NULL DEFAULT 0,
    umbral_minimo              INTEGER      NOT NULL DEFAULT 0,
    CONSTRAINT uk_medicamentos_nombre UNIQUE (nombre),
    CONSTRAINT chk_medicamentos_nombre_texto CHECK (trim(nombre) <> '' AND length(nombre) <= 100),
    CONSTRAINT chk_medicamentos_cantidad_total                          -- CAMBIO #5
        CHECK (typeof(cantidad_total_disponible) = 'integer' AND cantidad_total_disponible >= 0),
    CONSTRAINT chk_medicamentos_umbral_minimo
        CHECK (typeof(umbral_minimo) = 'integer' AND umbral_minimo >= 0)
);

-- lotes: cada medicamento llega en lotes con su propia fecha de vencimiento.
-- REV #6: cantidad = entradas - consumos - bajas. Un lote nace en 0 y solo
-- la cambian los movimientos; escribirla a mano se rechaza.
CREATE TABLE lotes (
    id_lote            INTEGER PRIMARY KEY AUTOINCREMENT,
    id_medicamento     INTEGER      NOT NULL,
    num_lote           VARCHAR(50)  NOT NULL COLLATE NOCASE,           -- CAMBIO #6
    ubicacion          VARCHAR(100) NOT NULL,
    fecha_vencimiento  DATE         NOT NULL,
    cantidad           INTEGER      NOT NULL DEFAULT 0,
    CONSTRAINT uk_lotes_medicamento_num_lote UNIQUE (id_medicamento, num_lote),
    CONSTRAINT chk_lotes_num_lote_texto CHECK (trim(num_lote) <> '' AND length(num_lote) <= 50),
    CONSTRAINT chk_lotes_ubicacion_texto CHECK (trim(ubicacion) <> '' AND length(ubicacion) <= 100),
    -- CAMBIO #5. Con REV #6 este CHECK es el que impide consumir más de lo que hay.
    CONSTRAINT chk_lotes_cantidad
        CHECK (typeof(cantidad) = 'integer' AND cantidad >= 0),
    CONSTRAINT chk_lotes_fecha_vencimiento_formato                      -- CAMBIO #4
        CHECK (date(fecha_vencimiento) IS fecha_vencimiento),
    CONSTRAINT fk_lotes_medicamentos
        FOREIGN KEY (id_medicamento) REFERENCES medicamentos (id_medicamento)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- tratamientos: un medicamento asignado a un paciente.
-- fecha_fin = fecha prevista; fecha_finalizacion_efectiva = cuándo terminó de verdad.
-- id_usuario_registra + fecha_registro = trazabilidad de quién y cuándo lo registró (RF-40).
-- frecuencia = texto para mostrar ("Cada 8 horas").
-- intervalo_horas = lo mismo en número, para que el backend calcule cuándo
--   toca la siguiente dosis y genere las alertas DOSIS_PENDIENTE (REV #18).
--   NULL si no tiene horario fijo.
-- Estados:
--   ACTIVO     -> todavía no tiene fecha_finalizacion_efectiva.
--   FINALIZADO -> siempre tiene fecha_finalizacion_efectiva.
--   SUSPENDIDO -> puede o no tenerla (se puede reanudar).
CREATE TABLE tratamientos (
    id_tratamiento               INTEGER PRIMARY KEY AUTOINCREMENT,
    id_paciente                  INTEGER      NOT NULL,
    id_medicamento               INTEGER      NOT NULL,
    id_usuario_registra          INTEGER      NOT NULL,
    dosis                        VARCHAR(100) NOT NULL,
    frecuencia                   VARCHAR(100) NOT NULL,
    intervalo_horas              INTEGER      NULL,
    fecha_inicio                 DATE         NOT NULL,
    fecha_fin                    DATE         NULL,
    fecha_finalizacion_efectiva  DATE         NULL,
    estado_tratamiento           TEXT         NOT NULL DEFAULT 'ACTIVO'
        CONSTRAINT chk_tratamientos_estado_tratamiento
        CHECK (estado_tratamiento IN ('ACTIVO', 'FINALIZADO', 'SUSPENDIDO')),
    fecha_registro               DATETIME     NOT NULL DEFAULT (datetime('now', '-5 hours')),   -- REV #9
    CONSTRAINT chk_tratamientos_dosis_texto CHECK (trim(dosis) <> '' AND length(dosis) <= 100),
    CONSTRAINT chk_tratamientos_frecuencia_texto CHECK (trim(frecuencia) <> '' AND length(frecuencia) <= 100),
    CONSTRAINT chk_tratamientos_intervalo_horas                         -- REV #18
        CHECK (intervalo_horas IS NULL
            OR (typeof(intervalo_horas) = 'integer' AND intervalo_horas BETWEEN 1 AND 168)),
    CONSTRAINT chk_tratamientos_fecha_inicio_formato                    -- CAMBIO #4
        CHECK (date(fecha_inicio) IS fecha_inicio),
    CONSTRAINT chk_tratamientos_fecha_fin_formato
        CHECK (date(fecha_fin) IS fecha_fin),
    CONSTRAINT chk_tratamientos_fecha_efectiva_formato
        CHECK (date(fecha_finalizacion_efectiva) IS fecha_finalizacion_efectiva),
    CONSTRAINT chk_tratamientos_fecha_registro_formato
        CHECK (datetime(fecha_registro) IS fecha_registro),
    CONSTRAINT chk_tratamientos_fechas
        CHECK (fecha_fin IS NULL OR fecha_fin >= fecha_inicio),
    CONSTRAINT chk_tratamientos_fecha_efectiva                          -- CAMBIO #10
        CHECK (fecha_finalizacion_efectiva IS NULL OR fecha_finalizacion_efectiva >= fecha_inicio),
    CONSTRAINT chk_tratamientos_estado_fecha_efectiva                   -- CAMBIO #10
        CHECK ((estado_tratamiento = 'ACTIVO'     AND fecha_finalizacion_efectiva IS NULL)
            OR (estado_tratamiento = 'FINALIZADO' AND fecha_finalizacion_efectiva IS NOT NULL)
            OR  estado_tratamiento = 'SUSPENDIDO'),
    CONSTRAINT fk_tratamientos_pacientes
        FOREIGN KEY (id_paciente) REFERENCES pacientes (id_paciente)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_tratamientos_medicamentos
        FOREIGN KEY (id_medicamento) REFERENCES medicamentos (id_medicamento)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_tratamientos_usuarios
        FOREIGN KEY (id_usuario_registra) REFERENCES usuarios (id_usuario)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- movimientos_inventario: historial de entradas y salidas de cada lote.
-- Es la única fuente de verdad del inventario (REV #6).
-- cantidad siempre es positiva; el tipo dice si suma o resta:
--   ENTRADA -> suma   (compra, reposición, o corrección de una salida mal registrada)
--   CONSUMO -> resta  (uso en un tratamiento o en una emergencia)
--   BAJA    -> resta  (lote vencido o dañado, o corrección de una entrada mal registrada)
-- REV #20: los movimientos NO se editan ni se borran (como un libro
-- contable). Un error se corrige con un movimiento contrario que diga en
-- el motivo cuál movimiento corrige.
-- id_tratamiento solo se llena en un CONSUMO que viene de un tratamiento.
CREATE TABLE movimientos_inventario (
    id_movimiento      INTEGER PRIMARY KEY AUTOINCREMENT,
    id_lote            INTEGER      NOT NULL,
    id_usuario         INTEGER      NOT NULL,
    id_tratamiento     INTEGER      NULL,
    tipo_movimiento    TEXT         NOT NULL
        CONSTRAINT chk_movimientos_inventario_tipo_movimiento
        CHECK (tipo_movimiento IN ('ENTRADA', 'CONSUMO', 'BAJA')),      -- REV #20: + BAJA
    cantidad           INTEGER      NOT NULL,
    fecha_movimiento   DATETIME     NOT NULL DEFAULT (datetime('now', '-5 hours')),   -- REV #9
    motivo             VARCHAR(300) NULL,
    CONSTRAINT chk_movimientos_inventario_cantidad                      -- CAMBIO #5
        CHECK (typeof(cantidad) = 'integer' AND cantidad > 0),
    CONSTRAINT chk_movimientos_inventario_tratamiento_solo_consumo      -- CAMBIO #15
        CHECK (id_tratamiento IS NULL OR tipo_movimiento = 'CONSUMO'),
    -- REV #20: toda salida que no viene de un tratamiento explica por qué.
    CONSTRAINT chk_movimientos_inventario_motivo_obligatorio
        CHECK (tipo_movimiento = 'ENTRADA'
            OR id_tratamiento IS NOT NULL
            OR COALESCE(trim(motivo), '') <> ''),
    CONSTRAINT chk_movimientos_inventario_motivo_texto CHECK (length(motivo) <= 300),
    CONSTRAINT chk_movimientos_inventario_fecha_formato                 -- CAMBIO #4
        CHECK (datetime(fecha_movimiento) IS fecha_movimiento),
    CONSTRAINT fk_movimientos_inventario_lotes
        FOREIGN KEY (id_lote) REFERENCES lotes (id_lote)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_movimientos_inventario_usuarios
        FOREIGN KEY (id_usuario) REFERENCES usuarios (id_usuario)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_movimientos_inventario_tratamientos
        FOREIGN KEY (id_tratamiento) REFERENCES tratamientos (id_tratamiento)
        ON DELETE RESTRICT ON UPDATE CASCADE
);

-- alertas: avisos automáticos del sistema.
-- Cada tipo usa UNA sola referencia y las otras dos quedan en NULL:
--   STOCK_BAJO          -> id_medicamento
--   VENCIMIENTO_PROXIMO -> id_lote
--   DOSIS_PENDIENTE     -> id_tratamiento
-- notificada = ya se avisó al responsable; resuelta = la alerta se cerró (RF-48).
CREATE TABLE alertas (
    id_alerta         INTEGER PRIMARY KEY AUTOINCREMENT,
    id_medicamento    INTEGER      NULL,
    id_lote           INTEGER      NULL,
    id_tratamiento    INTEGER      NULL,
    tipo_alerta       TEXT         NOT NULL
        CONSTRAINT chk_alertas_tipo_alerta
        CHECK (tipo_alerta IN ('STOCK_BAJO', 'VENCIMIENTO_PROXIMO', 'DOSIS_PENDIENTE')),
    mensaje           VARCHAR(300) NOT NULL,
    -- Mismo dominio que emergencias.nivel_urgencia.
    prioridad         TEXT         NOT NULL
        CONSTRAINT chk_alertas_prioridad
        CHECK (prioridad IN ('BAJA', 'MEDIA', 'ALTA', 'CRITICA')),
    notificada        INTEGER      NOT NULL DEFAULT 0
        CONSTRAINT chk_alertas_notificada CHECK (notificada IN (0, 1)),
    resuelta          INTEGER      NOT NULL DEFAULT 0
        CONSTRAINT chk_alertas_resuelta CHECK (resuelta IN (0, 1)),
    fecha_generacion  DATETIME     NOT NULL DEFAULT (datetime('now', '-5 hours')),   -- REV #9
    fecha_resolucion  DATETIME     NULL,
    CONSTRAINT chk_alertas_mensaje_texto CHECK (trim(mensaje) <> '' AND length(mensaje) <= 300),
    CONSTRAINT chk_alertas_referencia                                   -- CAMBIO #2
        CHECK ((tipo_alerta = 'STOCK_BAJO'
                    AND id_medicamento IS NOT NULL AND id_lote IS NULL AND id_tratamiento IS NULL)
            OR (tipo_alerta = 'VENCIMIENTO_PROXIMO'
                    AND id_lote IS NOT NULL AND id_medicamento IS NULL AND id_tratamiento IS NULL)
            OR (tipo_alerta = 'DOSIS_PENDIENTE'
                    AND id_tratamiento IS NOT NULL AND id_medicamento IS NULL AND id_lote IS NULL)),
    CONSTRAINT chk_alertas_resuelta_fecha                               -- CAMBIO #13
        CHECK ((resuelta = 1 AND fecha_resolucion IS NOT NULL)
            OR (resuelta = 0 AND fecha_resolucion IS NULL)),
    CONSTRAINT chk_alertas_fechas                                       -- CAMBIO #13
        CHECK (fecha_resolucion IS NULL OR fecha_resolucion >= fecha_generacion),
    CONSTRAINT chk_alertas_fecha_generacion_formato                     -- CAMBIO #4
        CHECK (datetime(fecha_generacion) IS fecha_generacion),
    CONSTRAINT chk_alertas_fecha_resolucion_formato
        CHECK (datetime(fecha_resolucion) IS fecha_resolucion),
    CONSTRAINT fk_alertas_medicamentos
        FOREIGN KEY (id_medicamento) REFERENCES medicamentos (id_medicamento)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_alertas_lotes
        FOREIGN KEY (id_lote) REFERENCES lotes (id_lote)
        ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT fk_alertas_tratamientos
        FOREIGN KEY (id_tratamiento) REFERENCES tratamientos (id_tratamiento)
        ON DELETE RESTRICT ON UPDATE CASCADE
);


-- #####################################################################
-- ÍNDICES
-- CAMBIO #18: MySQL (InnoDB) crea solo un índice por cada FK; SQLite no.
-- No se crean donde una PK o un UNIQUE ya empieza por esa columna
-- (rol_permiso.id_rol, pasos_protocolo.id_protocolo, pacientes.id_usuario,
-- historias_clinicas.id_paciente, alergias.id_historia,
-- emergencia_paciente.id_emergencia, lotes.id_medicamento).
-- #####################################################################

CREATE INDEX idx_usuarios_id_rol                         ON usuarios (id_rol);
CREATE INDEX idx_rol_permiso_id_permiso                  ON rol_permiso (id_permiso);
CREATE INDEX idx_logs_auditoria_id_usuario               ON logs_auditoria (id_usuario);
CREATE INDEX idx_logs_auditoria_tabla_registro           ON logs_auditoria (tabla_afectada, id_registro);
CREATE INDEX idx_alergias_id_medicamento                 ON alergias (id_medicamento);           -- REV #2
CREATE INDEX idx_eventos_medicos_id_historia             ON eventos_medicos (id_historia);
CREATE INDEX idx_eventos_medicos_id_usuario_registra     ON eventos_medicos (id_usuario_registra);
CREATE INDEX idx_controles_evolucion_paciente_fecha      ON controles_evolucion (id_paciente, fecha);
CREATE INDEX idx_controles_evolucion_id_usuario_registra ON controles_evolucion (id_usuario_registra);
CREATE INDEX idx_emergencias_tipo                        ON emergencias (tipo);                  -- REV #17
CREATE INDEX idx_emergencias_estado                      ON emergencias (estado);                -- REV #17
CREATE INDEX idx_emergencia_paciente_id_paciente         ON emergencia_paciente (id_paciente);
CREATE INDEX idx_acciones_emergencia_id_emergencia       ON acciones_emergencia (id_emergencia);
CREATE INDEX idx_acciones_emergencia_id_usuario_ejecutor ON acciones_emergencia (id_usuario_ejecutor);
CREATE INDEX idx_lotes_fecha_vencimiento                 ON lotes (fecha_vencimiento);
CREATE INDEX idx_tratamientos_id_paciente                ON tratamientos (id_paciente);
CREATE INDEX idx_tratamientos_id_medicamento             ON tratamientos (id_medicamento);
CREATE INDEX idx_tratamientos_id_usuario_registra        ON tratamientos (id_usuario_registra);
CREATE INDEX idx_movimientos_inventario_id_lote          ON movimientos_inventario (id_lote);
CREATE INDEX idx_movimientos_inventario_id_usuario       ON movimientos_inventario (id_usuario);
CREATE INDEX idx_movimientos_inventario_id_tratamiento   ON movimientos_inventario (id_tratamiento);
CREATE INDEX idx_alertas_id_medicamento                  ON alertas (id_medicamento);
CREATE INDEX idx_alertas_id_lote                         ON alertas (id_lote);
CREATE INDEX idx_alertas_id_tratamiento                  ON alertas (id_tratamiento);

-- CAMBIO #14: no puede haber dos alertas ABIERTAS de stock bajo para el
-- mismo medicamento, ni de vencimiento para el mismo lote.
CREATE UNIQUE INDEX uk_alertas_stock_bajo_abierta
    ON alertas (id_medicamento)
    WHERE tipo_alerta = 'STOCK_BAJO' AND resuelta = 0;
CREATE UNIQUE INDEX uk_alertas_vencimiento_abierta
    ON alertas (id_lote)
    WHERE tipo_alerta = 'VENCIMIENTO_PROXIMO' AND resuelta = 0;


-- #####################################################################
-- TRIGGERS
-- #####################################################################

-- ---------------------------------------------------------------------
-- Auditoría de solo lectura (CAMBIO #17)
-- (El DROP TABLE del inicio del script sí funciona: no dispara triggers.)
-- ---------------------------------------------------------------------
CREATE TRIGGER trg_logs_auditoria_bu_solo_lectura
BEFORE UPDATE ON logs_auditoria
BEGIN
    SELECT RAISE(ABORT, 'logs_auditoria es de solo lectura: no se puede modificar');
END;

CREATE TRIGGER trg_logs_auditoria_bd_solo_lectura
BEFORE DELETE ON logs_auditoria
BEGIN
    SELECT RAISE(ABORT, 'logs_auditoria es de solo lectura: no se puede borrar');
END;

-- ---------------------------------------------------------------------
-- REV #5: pacientes.estado_salud = estado del último control
-- Cada vez que un control se crea, cambia o se borra, se recalcula el
-- estado del paciente desde cero con el control más reciente (por fecha).
-- Si se carga tarde un control viejo, no pisa el estado actual.
-- ---------------------------------------------------------------------
CREATE TRIGGER trg_controles_evolucion_ai_estado_paciente
AFTER INSERT ON controles_evolucion
BEGIN
    UPDATE pacientes
       SET estado_salud = (SELECT c.estado FROM controles_evolucion c
                            WHERE c.id_paciente = NEW.id_paciente
                            ORDER BY c.fecha DESC, c.id_control DESC LIMIT 1)
     WHERE id_paciente = NEW.id_paciente;
END;

CREATE TRIGGER trg_controles_evolucion_au_estado_paciente
AFTER UPDATE OF estado, fecha, id_paciente ON controles_evolucion
BEGIN
    UPDATE pacientes
       SET estado_salud = (SELECT c.estado FROM controles_evolucion c
                            WHERE c.id_paciente = NEW.id_paciente
                            ORDER BY c.fecha DESC, c.id_control DESC LIMIT 1)
     WHERE id_paciente = NEW.id_paciente;
    -- Si el control se pasó a otro paciente, el anterior queda con su
    -- nuevo último control (o con el estado que tenía, si ya no tiene).
    UPDATE pacientes
       SET estado_salud = COALESCE((SELECT c.estado FROM controles_evolucion c
                                     WHERE c.id_paciente = OLD.id_paciente
                                     ORDER BY c.fecha DESC, c.id_control DESC LIMIT 1),
                                   estado_salud)
     WHERE id_paciente = OLD.id_paciente
       AND OLD.id_paciente <> NEW.id_paciente;
END;

CREATE TRIGGER trg_controles_evolucion_ad_estado_paciente
AFTER DELETE ON controles_evolucion
BEGIN
    UPDATE pacientes
       SET estado_salud = COALESCE((SELECT c.estado FROM controles_evolucion c
                                     WHERE c.id_paciente = OLD.id_paciente
                                     ORDER BY c.fecha DESC, c.id_control DESC LIMIT 1),
                                   estado_salud)
     WHERE id_paciente = OLD.id_paciente;
END;

-- Con controles registrados, el estado no se cambia a mano: se registra
-- un control nuevo (así queda quién y cuándo lo cambió).
CREATE TRIGGER trg_pacientes_bu_estado_salud
BEFORE UPDATE OF estado_salud ON pacientes
WHEN EXISTS (SELECT 1 FROM controles_evolucion c WHERE c.id_paciente = NEW.id_paciente)
 AND NEW.estado_salud IS NOT (SELECT c.estado FROM controles_evolucion c
                               WHERE c.id_paciente = NEW.id_paciente
                               ORDER BY c.fecha DESC, c.id_control DESC LIMIT 1)
BEGIN
    SELECT RAISE(ABORT, 'El paciente tiene controles: su estado de salud se cambia registrando un control nuevo');
END;

-- ---------------------------------------------------------------------
-- REV #2: alergias
-- ---------------------------------------------------------------------
-- Registrar una alergia marca la historia como revisada.
CREATE TRIGGER trg_alergias_ai_historia_revisada
AFTER INSERT ON alergias
BEGIN
    UPDATE historias_clinicas
       SET alergias_revisadas = 1
     WHERE id_historia = NEW.id_historia
       AND alergias_revisadas = 0;
END;

-- No se receta un medicamento al que el paciente es alérgico.
CREATE TRIGGER trg_tratamientos_bi_alergia
BEFORE INSERT ON tratamientos
BEGIN
    SELECT RAISE(ABORT, 'El paciente es alérgico a este medicamento')
      FROM alergias a
      JOIN historias_clinicas h ON h.id_historia = a.id_historia
     WHERE h.id_paciente = NEW.id_paciente
       AND a.id_medicamento = NEW.id_medicamento;
END;

CREATE TRIGGER trg_tratamientos_bu_alergia
BEFORE UPDATE OF id_paciente, id_medicamento ON tratamientos
BEGIN
    SELECT RAISE(ABORT, 'El paciente es alérgico a este medicamento')
      FROM alergias a
      JOIN historias_clinicas h ON h.id_historia = a.id_historia
     WHERE h.id_paciente = NEW.id_paciente
       AND a.id_medicamento = NEW.id_medicamento;
END;

-- ---------------------------------------------------------------------
-- REV #14: las acciones caen dentro del horario de su emergencia
-- ---------------------------------------------------------------------
CREATE TRIGGER trg_acciones_emergencia_bi_rango
BEFORE INSERT ON acciones_emergencia
BEGIN
    SELECT RAISE(ABORT, 'La acción queda fuera del horario de la emergencia')
      FROM emergencias e
     WHERE e.id_emergencia = NEW.id_emergencia
       AND (NEW.fecha_hora < e.fecha_inicio
            OR (e.fecha_fin IS NOT NULL AND NEW.fecha_hora > e.fecha_fin));
END;

CREATE TRIGGER trg_acciones_emergencia_bu_rango
BEFORE UPDATE OF fecha_hora, id_emergencia ON acciones_emergencia
BEGIN
    SELECT RAISE(ABORT, 'La acción queda fuera del horario de la emergencia')
      FROM emergencias e
     WHERE e.id_emergencia = NEW.id_emergencia
       AND (NEW.fecha_hora < e.fecha_inicio
            OR (e.fecha_fin IS NOT NULL AND NEW.fecha_hora > e.fecha_fin));
END;

-- La emergencia no puede cambiar sus fechas dejando acciones por fuera
-- (por ejemplo, finalizarla con una fecha_fin anterior a la última acción).
CREATE TRIGGER trg_emergencias_bu_rango_acciones
BEFORE UPDATE OF fecha_inicio, fecha_fin ON emergencias
WHEN EXISTS (SELECT 1 FROM acciones_emergencia a
              WHERE a.id_emergencia = NEW.id_emergencia
                AND (a.fecha_hora < NEW.fecha_inicio
                     OR (NEW.fecha_fin IS NOT NULL AND a.fecha_hora > NEW.fecha_fin)))
BEGIN
    SELECT RAISE(ABORT, 'Hay acciones de la emergencia fuera de las nuevas fechas');
END;

-- ---------------------------------------------------------------------
-- REV #7: medicamentos.cantidad_total_disponible = SUM(lotes.cantidad)
-- (CAMBIO #8 la recalcula; REV #7 impide escribirla a mano)
-- ---------------------------------------------------------------------
CREATE TRIGGER trg_medicamentos_bi_total_cero
BEFORE INSERT ON medicamentos
WHEN NEW.cantidad_total_disponible <> 0
BEGIN
    SELECT RAISE(ABORT, 'cantidad_total_disponible no se escribe: la calcula la base desde los lotes');
END;

CREATE TRIGGER trg_medicamentos_bu_total_calculado
BEFORE UPDATE OF cantidad_total_disponible ON medicamentos
WHEN NEW.cantidad_total_disponible IS NOT
     (SELECT COALESCE(SUM(l.cantidad), 0) FROM lotes l WHERE l.id_medicamento = NEW.id_medicamento)
BEGIN
    SELECT RAISE(ABORT, 'cantidad_total_disponible no se escribe: la calcula la base desde los lotes');
END;

CREATE TRIGGER trg_lotes_ai_total_medicamento
AFTER INSERT ON lotes
BEGIN
    UPDATE medicamentos
       SET cantidad_total_disponible =
           (SELECT COALESCE(SUM(cantidad), 0) FROM lotes WHERE id_medicamento = NEW.id_medicamento)
     WHERE id_medicamento = NEW.id_medicamento;
END;

CREATE TRIGGER trg_lotes_au_total_medicamento
AFTER UPDATE OF cantidad, id_medicamento ON lotes
BEGIN
    UPDATE medicamentos
       SET cantidad_total_disponible =
           (SELECT COALESCE(SUM(cantidad), 0) FROM lotes WHERE id_medicamento = NEW.id_medicamento)
     WHERE id_medicamento = NEW.id_medicamento;
    UPDATE medicamentos
       SET cantidad_total_disponible =
           (SELECT COALESCE(SUM(cantidad), 0) FROM lotes WHERE id_medicamento = OLD.id_medicamento)
     WHERE id_medicamento = OLD.id_medicamento
       AND OLD.id_medicamento <> NEW.id_medicamento;
END;

CREATE TRIGGER trg_lotes_ad_total_medicamento
AFTER DELETE ON lotes
BEGIN
    UPDATE medicamentos
       SET cantidad_total_disponible =
           (SELECT COALESCE(SUM(cantidad), 0) FROM lotes WHERE id_medicamento = OLD.id_medicamento)
     WHERE id_medicamento = OLD.id_medicamento;
END;

-- ---------------------------------------------------------------------
-- REV #6: lotes.cantidad = entradas - consumos - bajas
-- ---------------------------------------------------------------------
-- Un lote nace en 0: la cantidad entra con un movimiento ENTRADA.
CREATE TRIGGER trg_lotes_bi_cantidad_cero
BEFORE INSERT ON lotes
WHEN NEW.cantidad <> 0
BEGIN
    SELECT RAISE(ABORT, 'Un lote se crea con cantidad 0; la cantidad entra con un movimiento ENTRADA');
END;

-- Nadie escribe la cantidad a mano: solo vale el resultado de los movimientos.
CREATE TRIGGER trg_lotes_bu_cantidad_calculada
BEFORE UPDATE OF cantidad ON lotes
WHEN NEW.cantidad IS NOT
     (SELECT COALESCE(SUM(CASE m.tipo_movimiento WHEN 'ENTRADA' THEN m.cantidad ELSE -m.cantidad END), 0)
        FROM movimientos_inventario m WHERE m.id_lote = NEW.id_lote)
BEGIN
    SELECT RAISE(ABORT, 'lotes.cantidad no se escribe: la calcula la base desde los movimientos');
END;

-- Antes de una salida, revisar que haya stock (mensaje claro para el backend;
-- chk_lotes_cantidad también lo impediría).
CREATE TRIGGER trg_movimientos_inventario_bi_stock
BEFORE INSERT ON movimientos_inventario
WHEN NEW.tipo_movimiento IN ('CONSUMO', 'BAJA')
 AND NEW.cantidad > (SELECT l.cantidad FROM lotes l WHERE l.id_lote = NEW.id_lote)
BEGIN
    SELECT RAISE(ABORT, 'No hay suficiente stock en el lote');
END;

-- Cada movimiento recalcula su lote (y el lote, por CAMBIO #8, su medicamento).
CREATE TRIGGER trg_movimientos_inventario_ai_stock
AFTER INSERT ON movimientos_inventario
BEGIN
    UPDATE lotes
       SET cantidad = (SELECT COALESCE(SUM(CASE m.tipo_movimiento WHEN 'ENTRADA' THEN m.cantidad ELSE -m.cantidad END), 0)
                         FROM movimientos_inventario m WHERE m.id_lote = NEW.id_lote)
     WHERE id_lote = NEW.id_lote;
END;

-- ---------------------------------------------------------------------
-- REV #20: movimientos de solo lectura
-- (por eso ya no hace falta el trigger BEFORE UPDATE del CAMBIO #16)
-- ---------------------------------------------------------------------
CREATE TRIGGER trg_movimientos_inventario_bu_solo_lectura
BEFORE UPDATE ON movimientos_inventario
BEGIN
    SELECT RAISE(ABORT, 'Los movimientos no se editan: registre un movimiento contrario con el motivo');
END;

CREATE TRIGGER trg_movimientos_inventario_bd_solo_lectura
BEFORE DELETE ON movimientos_inventario
BEGIN
    SELECT RAISE(ABORT, 'Los movimientos no se borran: registre un movimiento contrario con el motivo');
END;

-- ---------------------------------------------------------------------
-- CAMBIO #16 + REV #8: el lote de un consumo es del medicamento del tratamiento
-- ---------------------------------------------------------------------
CREATE TRIGGER trg_movimientos_inventario_bi_lote_tratamiento
BEFORE INSERT ON movimientos_inventario
WHEN NEW.id_tratamiento IS NOT NULL
BEGIN
    SELECT RAISE(ABORT, 'El lote no es del medicamento del tratamiento')
      FROM lotes l, tratamientos t
     WHERE l.id_lote = NEW.id_lote
       AND t.id_tratamiento = NEW.id_tratamiento
       AND l.id_medicamento <> t.id_medicamento;
END;

-- REV #8: tampoco se puede romper la regla cambiando después el
-- medicamento del tratamiento o del lote.
CREATE TRIGGER trg_tratamientos_bu_medicamento_con_consumos
BEFORE UPDATE OF id_medicamento ON tratamientos
WHEN EXISTS (SELECT 1 FROM movimientos_inventario m
               JOIN lotes l ON l.id_lote = m.id_lote
              WHERE m.id_tratamiento = NEW.id_tratamiento
                AND l.id_medicamento <> NEW.id_medicamento)
BEGIN
    SELECT RAISE(ABORT, 'El tratamiento ya tiene consumos de otro medicamento');
END;

CREATE TRIGGER trg_lotes_bu_medicamento_con_consumos
BEFORE UPDATE OF id_medicamento ON lotes
WHEN EXISTS (SELECT 1 FROM movimientos_inventario m
               JOIN tratamientos t ON t.id_tratamiento = m.id_tratamiento
              WHERE m.id_lote = NEW.id_lote
                AND t.id_medicamento <> NEW.id_medicamento)
BEGIN
    SELECT RAISE(ABORT, 'El lote ya tiene consumos de tratamientos de otro medicamento');
END;
