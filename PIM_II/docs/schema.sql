-- PIM II: esquema do banco (SQLite). Gerado a partir de DataBase.c
PRAGMA foreign_keys = ON;

CREATE TABLE planos (  id INTEGER PRIMARY KEY AUTOINCREMENT,  nome TEXT NOT NULL UNIQUE,  valor_mensal REAL NOT NULL CHECK (valor_mensal >= 0),  ativo INTEGER NOT NULL DEFAULT 1);

CREATE TABLE beneficiarios (  id INTEGER PRIMARY KEY AUTOINCREMENT,  nome TEXT NOT NULL,  cpf TEXT NOT NULL UNIQUE,  data_nascimento TEXT NOT NULL,  telefone TEXT,  plano_id INTEGER NOT NULL,  ativo INTEGER NOT NULL DEFAULT 1,  FOREIGN KEY (plano_id) REFERENCES planos(id));

CREATE TABLE medicos (  id INTEGER PRIMARY KEY AUTOINCREMENT,  nome TEXT NOT NULL,  crm TEXT NOT NULL UNIQUE,  especialidade TEXT NOT NULL,  ativo INTEGER NOT NULL DEFAULT 1);

CREATE TABLE consultas (  id INTEGER PRIMARY KEY AUTOINCREMENT,  beneficiario_id INTEGER NOT NULL,  medico_id INTEGER NOT NULL,  data_hora TEXT NOT NULL,  status TEXT NOT NULL DEFAULT 'AGENDADA'    CHECK (status IN ('AGENDADA','REALIZADA','CANCELADA')),  observacao TEXT,  FOREIGN KEY (beneficiario_id) REFERENCES beneficiarios(id),  FOREIGN KEY (medico_id) REFERENCES medicos(id));

CREATE INDEX idx_consultas_medico_data  ON consultas(medico_id, data_hora);

CREATE INDEX idx_consultas_benef_data  ON consultas(beneficiario_id, data_hora);
