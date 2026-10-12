#include "DataBase.h"

#include <stdio.h>

static sqlite3 *g_db = NULL;

int db_open(const char *caminho) {
    if (sqlite3_open(caminho, &g_db) != SQLITE_OK) {
        fprintf(stderr, "Erro ao abrir banco: %s\n", sqlite3_errmsg(g_db));
        sqlite3_close(g_db);
        g_db = NULL;
        return 0;
    }
    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    return 1;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

sqlite3 *db_get(void) { return g_db; }

static int executar_script(const char *sql, const char *contexto) {
    char *erro = NULL;
    if (sqlite3_exec(g_db, sql, NULL, NULL, &erro) != SQLITE_OK) {
        fprintf(stderr, "%s: %s\n", contexto, erro ? erro : "erro desconhecido");
        sqlite3_free(erro);
        return 0;
    }
    return 1;
}

int db_criar_tabelas(void) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS planos ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  nome TEXT NOT NULL UNIQUE,"
        "  valor_mensal REAL NOT NULL CHECK (valor_mensal >= 0),"
        "  ativo INTEGER NOT NULL DEFAULT 1"
        ");"
        "CREATE TABLE IF NOT EXISTS beneficiarios ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  nome TEXT NOT NULL,"
        "  cpf TEXT NOT NULL UNIQUE,"
        "  data_nascimento TEXT NOT NULL,"
        "  telefone TEXT,"
        "  plano_id INTEGER NOT NULL,"
        "  ativo INTEGER NOT NULL DEFAULT 1,"
        "  FOREIGN KEY (plano_id) REFERENCES planos(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS medicos ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  nome TEXT NOT NULL,"
        "  crm TEXT NOT NULL UNIQUE,"
        "  especialidade TEXT NOT NULL,"
        "  ativo INTEGER NOT NULL DEFAULT 1"
        ");"
        "CREATE TABLE IF NOT EXISTS consultas ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  beneficiario_id INTEGER NOT NULL,"
        "  medico_id INTEGER NOT NULL,"
        "  data_hora TEXT NOT NULL,"
        "  status TEXT NOT NULL DEFAULT 'AGENDADA'"
        "    CHECK (status IN ('AGENDADA','REALIZADA','CANCELADA')),"
        "  observacao TEXT,"
        "  FOREIGN KEY (beneficiario_id) REFERENCES beneficiarios(id),"
        "  FOREIGN KEY (medico_id) REFERENCES medicos(id)"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_consultas_medico_data"
        "  ON consultas(medico_id, data_hora);"
        "CREATE INDEX IF NOT EXISTS idx_consultas_benef_data"
        "  ON consultas(beneficiario_id, data_hora);";

    return executar_script(sql, "Erro ao criar tabelas");
}

int db_inserir_dados_iniciais(void) {
    const char *sql =
        "INSERT OR IGNORE INTO planos (nome, valor_mensal) VALUES "
        "('Básico', 199.90),"
        "('Intermediário', 349.90),"
        "('Premium', 599.90);";

    return executar_script(sql, "Erro ao inserir dados iniciais");
}
