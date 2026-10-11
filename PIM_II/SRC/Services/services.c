#include "services.h"
#include "DataBase.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ========================================================================== */
/*  Mensagens                                                                 */
/* ========================================================================== */

const char *srv_mensagem(SrvResultado r) {
    switch (r) {
    case SRV_OK:                       return "Operação realizada com sucesso.";
    case SRV_ERRO_DB:                  return "Erro no banco de dados.";
    case SRV_ERRO_NAO_ENCONTRADO:      return "Registro não encontrado.";
    case SRV_ERRO_CAMPO_VAZIO:         return "Preencha todos os campos obrigatórios.";
    case SRV_ERRO_CPF_INVALIDO:        return "CPF inválido. Informe 11 dígitos.";
    case SRV_ERRO_CPF_DUPLICADO:       return "Já existe um beneficiário com este CPF.";
    case SRV_ERRO_DATA_INVALIDA:       return "Data inválida. Use DD/MM/AAAA (não pode ser futura).";
    case SRV_ERRO_TELEFONE_INVALIDO:   return "Telefone inválido. Informe DDD + número (10 ou 11 dígitos).";
    case SRV_ERRO_PLANO_INVALIDO:      return "Selecione um plano existente e ativo.";
    case SRV_ERRO_PLANO_EM_USO:        return "Este plano possui beneficiários ativos e não pode ser inativado.";
    case SRV_ERRO_NOME_DUPLICADO:      return "Já existe um registro com este nome.";
    case SRV_ERRO_VALOR_INVALIDO:      return "Valor inválido. Informe um número maior que zero.";
    case SRV_ERRO_CRM_DUPLICADO:       return "Já existe um médico com este CRM.";
    case SRV_ERRO_TEM_AGENDAMENTOS:    return "Existem consultas agendadas. Cancele-as ou conclua-as antes de inativar.";
    case SRV_ERRO_BENEFICIARIO_INATIVO:return "O beneficiário está inativo.";
    case SRV_ERRO_MEDICO_INATIVO:      return "O médico está inativo.";
    case SRV_ERRO_DATAHORA_INVALIDA:   return "Data/hora inválida. Use DD/MM/AAAA HH:MM.";
    case SRV_ERRO_DATAHORA_PASSADA:    return "A data/hora da consulta não pode estar no passado.";
    case SRV_ERRO_CONFLITO_MEDICO:     return "O médico já possui uma consulta neste horário.";
    case SRV_ERRO_CONFLITO_BENEFICIARIO:return "O beneficiário já possui uma consulta neste horário.";
    case SRV_ERRO_STATUS_INVALIDO:     return "Somente consultas agendadas podem ser marcadas como realizadas ou canceladas.";
    }
    return "Erro desconhecido.";
}

/* ========================================================================== */
/*  Helpers de SQLite                                                         */
/* ========================================================================== */

static int preparar(const char *sql, sqlite3_stmt **st) {
    return sqlite3_prepare_v2(db_get(), sql, -1, st, NULL) == SQLITE_OK;
}

/* Executa o statement e o finaliza. Retorna 1 se terminou sem erro. */
static int concluir(sqlite3_stmt *st) {
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE;
}

static void bind_texto(sqlite3_stmt *st, int idx, const char *t) {
    sqlite3_bind_text(st, idx, t ? t : "", -1, SQLITE_TRANSIENT);
}

static void copiar_coluna(char *dest, size_t tam, sqlite3_stmt *st, int col) {
    const unsigned char *t = sqlite3_column_text(st, col);
    models_copiar(dest, tam, t ? (const char *)t : "");
}

/* SELECT 1 ... WHERE campo = ?1 AND id <> ?2 */
static int existe_texto(const char *sql, const char *texto, int id_ignorar) {
    sqlite3_stmt *st;
    if (!preparar(sql, &st)) return 0;
    bind_texto(st, 1, texto);
    sqlite3_bind_int(st, 2, id_ignorar);
    int achou = (sqlite3_step(st) == SQLITE_ROW);
    sqlite3_finalize(st);
    return achou;
}

/* SELECT ativo ... WHERE id = ?1  ->  -1 se não encontrado */
static int obter_ativo(const char *sql, int id) {
    sqlite3_stmt *st;
    if (!preparar(sql, &st)) return -1;
    sqlite3_bind_int(st, 1, id);
    int valor = -1;
    if (sqlite3_step(st) == SQLITE_ROW) valor = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return valor;
}

/* SELECT COUNT(*) ... WHERE x = ?1 */
static int contar_por_id(const char *sql, int id) {
    sqlite3_stmt *st;
    if (!preparar(sql, &st)) return 0;
    sqlite3_bind_int(st, 1, id);
    int n = 0;
    if (sqlite3_step(st) == SQLITE_ROW) n = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return n;
}

static int escalar_int(const char *sql) {
    sqlite3_stmt *st;
    if (!preparar(sql, &st)) return 0;
    int n = 0;
    if (sqlite3_step(st) == SQLITE_ROW) n = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    return n;
}

static double escalar_double(const char *sql) {
    sqlite3_stmt *st;
    if (!preparar(sql, &st)) return 0.0;
    double v = 0.0;
    if (sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_double(st, 0);
    sqlite3_finalize(st);
    return v;
}

typedef void (*Preenchedor)(void *dest, sqlite3_stmt *st);

/* Lê todas as linhas do statement para um vetor alocado (free() pelo chamador). */
static SrvResultado coletar(sqlite3_stmt *st, size_t tam_item, Preenchedor preencher,
                            void **lista, int *qtd) {
    int cap = 16, n = 0, rc;
    char *v = malloc((size_t)cap * tam_item);
    if (!v) {
        sqlite3_finalize(st);
        return SRV_ERRO_DB;
    }
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        if (n == cap) {
            cap *= 2;
            char *novo = realloc(v, (size_t)cap * tam_item);
            if (!novo) {
                free(v);
                sqlite3_finalize(st);
                return SRV_ERRO_DB;
            }
            v = novo;
        }
        memset(v + (size_t)n * tam_item, 0, tam_item);
        preencher(v + (size_t)n * tam_item, st);
        n++;
    }
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) {
        free(v);
        return SRV_ERRO_DB;
    }
    *lista = v;
    *qtd = n;
    return SRV_OK;
}

/* ========================================================================== */
/*  Validações e formatação                                                   */
/* ========================================================================== */

void srv_somente_digitos(char *dest, size_t tam, const char *origem) {
    size_t j = 0;
    if (!dest || tam == 0) return;
    for (; origem && *origem && j + 1 < tam; origem++)
        if (isdigit((unsigned char)*origem)) dest[j++] = *origem;
    dest[j] = '\0';
}

/* Validação de CPF FLEXÍVEL: aceita qualquer sequência de 11 dígitos, inclusive
 * CPFs fictícios (dígitos verificadores incorretos ou todos iguais, como
 * 111.111.111-11). A unicidade continua sendo checada ao salvar.
 *
 * Para voltar à validação oficial (módulo 11), troque o corpo desta função pela
 * versão em srv_cpf_valido_oficial(), logo abaixo. */
int srv_cpf_valido(const char *cpf) {
    if (!cpf || strlen(cpf) != 11) return 0;
    for (int i = 0; i < 11; i++)
        if (!isdigit((unsigned char)cpf[i])) return 0;
    return 1;
}

/* Validação oficial (dígitos verificadores). Mantida para uso futuro. */
int srv_cpf_valido_oficial(const char *cpf) {
    if (!cpf || strlen(cpf) != 11) return 0;
    int iguais = 1;
    for (int i = 0; i < 11; i++) {
        if (!isdigit((unsigned char)cpf[i])) return 0;
        if (cpf[i] != cpf[0]) iguais = 0;
    }
    if (iguais) return 0;
    for (int t = 9; t <= 10; t++) {
        int soma = 0;
        for (int i = 0; i < t; i++) soma += (cpf[i] - '0') * (t + 1 - i);
        int dv = (soma * 10) % 11;
        if (dv == 10) dv = 0;
        if (dv != cpf[t] - '0') return 0;
    }
    return 1;
}

SrvResultado srv_normalizar_cpf(const char *texto, char *cpf_out) {
    char tmp[32];
    srv_somente_digitos(tmp, sizeof tmp, texto);
    if (!srv_cpf_valido(tmp)) return SRV_ERRO_CPF_INVALIDO;
    memcpy(cpf_out, tmp, 12); /* 11 dígitos + '\0' */
    return SRV_OK;
}

SrvResultado srv_normalizar_telefone(const char *texto, char *tel_out) {
    char tmp[32];
    srv_somente_digitos(tmp, sizeof tmp, texto);
    size_t n = strlen(tmp);
    if (n != 0 && n != 10 && n != 11) return SRV_ERRO_TELEFONE_INVALIDO;
    memcpy(tel_out, tmp, n + 1);
    return SRV_OK;
}

static int dias_no_mes(int m, int a) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && ((a % 4 == 0 && a % 100 != 0) || a % 400 == 0)) return 29;
    return d[m - 1];
}

static int data_ok(int d, int m, int a) {
    return a >= 1900 && a <= 9999 && m >= 1 && m <= 12 && d >= 1 && d <= dias_no_mes(m, a);
}

static int hora_ok(int h, int mi) { return h >= 0 && h <= 23 && mi >= 0 && mi <= 59; }

int srv_data_br_para_iso(const char *br, char *iso, size_t tam) {
    int d, m, a;
    char resto;
    if (!br || sscanf(br, "%d/%d/%d%c", &d, &m, &a, &resto) != 3) return 0;
    if (!data_ok(d, m, a)) return 0;
    snprintf(iso, tam, "%04d-%02d-%02d", a, m, d);
    return 1;
}

int srv_datahora_br_para_iso(const char *br, char *iso, size_t tam) {
    int d, m, a, h, mi;
    char resto;
    if (!br || sscanf(br, "%d/%d/%d %d:%d%c", &d, &m, &a, &h, &mi, &resto) != 5) return 0;
    if (!data_ok(d, m, a) || !hora_ok(h, mi)) return 0;
    snprintf(iso, tam, "%04d-%02d-%02d %02d:%02d", a, m, d, h, mi);
    return 1;
}

static int iso_data_valida(const char *iso) {
    int a, m, d;
    char resto;
    if (!iso || strlen(iso) != 10) return 0;
    if (sscanf(iso, "%d-%d-%d%c", &a, &m, &d, &resto) != 3) return 0;
    return data_ok(d, m, a);
}

static int iso_datahora_valida(const char *iso) {
    int a, m, d, h, mi;
    char resto;
    if (!iso || strlen(iso) != 16) return 0;
    if (sscanf(iso, "%d-%d-%d %d:%d%c", &a, &m, &d, &h, &mi, &resto) != 5) return 0;
    return data_ok(d, m, a) && hora_ok(h, mi);
}

void srv_data_iso_para_br(const char *iso, char *out, size_t tam) {
    int a, m, d;
    if (iso && sscanf(iso, "%d-%d-%d", &a, &m, &d) == 3)
        snprintf(out, tam, "%02d/%02d/%04d", d, m, a);
    else
        models_copiar(out, tam, iso);
}

void srv_datahora_iso_para_br(const char *iso, char *out, size_t tam) {
    int a, m, d, h, mi;
    if (iso && sscanf(iso, "%d-%d-%d %d:%d", &a, &m, &d, &h, &mi) == 5)
        snprintf(out, tam, "%02d/%02d/%04d %02d:%02d", d, m, a, h, mi);
    else
        models_copiar(out, tam, iso);
}

void srv_formatar_cpf(const char *dig, char *out, size_t tam) {
    if (dig && strlen(dig) == 11)
        snprintf(out, tam, "%.3s.%.3s.%.3s-%.2s", dig, dig + 3, dig + 6, dig + 9);
    else
        models_copiar(out, tam, dig);
}

void srv_formatar_telefone(const char *dig, char *out, size_t tam) {
    size_t n = dig ? strlen(dig) : 0;
    if (n == 11)
        snprintf(out, tam, "(%.2s) %.5s-%.4s", dig, dig + 2, dig + 7);
    else if (n == 10)
        snprintf(out, tam, "(%.2s) %.4s-%.4s", dig, dig + 2, dig + 6);
    else
        models_copiar(out, tam, dig);
}

static void agora_formatado(char *buf, size_t tam, const char *fmt) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, tam, fmt, tm);
}

/* ========================================================================== */
/*  Planos                                                                    */
/* ========================================================================== */

static void preencher_plano(void *dest, sqlite3_stmt *st) {
    Plano *p = dest;
    p->id = sqlite3_column_int(st, 0);
    copiar_coluna(p->nome, sizeof p->nome, st, 1);
    p->valor_mensal = sqlite3_column_double(st, 2);
    p->ativo = sqlite3_column_int(st, 3);
}

static SrvResultado validar_plano(Plano *p, int id_ignorar) {
    models_trim(p->nome);
    if (!p->nome[0]) return SRV_ERRO_CAMPO_VAZIO;
    if (!(p->valor_mensal > 0.0)) return SRV_ERRO_VALOR_INVALIDO;
    if (existe_texto("SELECT 1 FROM planos WHERE nome = ?1 COLLATE NOCASE AND id <> ?2 LIMIT 1",
                     p->nome, id_ignorar))
        return SRV_ERRO_NOME_DUPLICADO;
    return SRV_OK;
}

SrvResultado srv_plano_inserir(Plano *p) {
    SrvResultado v = validar_plano(p, 0);
    if (v != SRV_OK) return v;

    sqlite3_stmt *st;
    if (!preparar("INSERT INTO planos (nome, valor_mensal, ativo) VALUES (?1, ?2, 1)", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, p->nome);
    sqlite3_bind_double(st, 2, p->valor_mensal);
    if (!concluir(st)) return SRV_ERRO_DB;

    p->id = (int)sqlite3_last_insert_rowid(db_get());
    p->ativo = 1;
    return SRV_OK;
}

SrvResultado srv_plano_atualizar(const Plano *p_in) {
    Plano p = *p_in;
    if (obter_ativo("SELECT ativo FROM planos WHERE id = ?1", p.id) < 0)
        return SRV_ERRO_NAO_ENCONTRADO;
    SrvResultado v = validar_plano(&p, p.id);
    if (v != SRV_OK) return v;

    sqlite3_stmt *st;
    if (!preparar("UPDATE planos SET nome = ?1, valor_mensal = ?2 WHERE id = ?3", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, p.nome);
    sqlite3_bind_double(st, 2, p.valor_mensal);
    sqlite3_bind_int(st, 3, p.id);
    return concluir(st) ? SRV_OK : SRV_ERRO_DB;
}

SrvResultado srv_plano_alterar_ativo(int id, int ativo) {
    if (obter_ativo("SELECT ativo FROM planos WHERE id = ?1", id) < 0)
        return SRV_ERRO_NAO_ENCONTRADO;
    if (!ativo &&
        contar_por_id("SELECT COUNT(*) FROM beneficiarios WHERE plano_id = ?1 AND ativo = 1", id) > 0)
        return SRV_ERRO_PLANO_EM_USO;

    sqlite3_stmt *st;
    if (!preparar("UPDATE planos SET ativo = ?1 WHERE id = ?2", &st)) return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, ativo ? 1 : 0);
    sqlite3_bind_int(st, 2, id);
    return concluir(st) ? SRV_OK : SRV_ERRO_DB;
}

SrvResultado srv_plano_buscar(int id, Plano *out) {
    sqlite3_stmt *st;
    if (!preparar("SELECT id, nome, valor_mensal, ativo FROM planos WHERE id = ?1", &st))
        return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, id);
    SrvResultado r = SRV_ERRO_NAO_ENCONTRADO;
    if (sqlite3_step(st) == SQLITE_ROW) {
        memset(out, 0, sizeof *out);
        preencher_plano(out, st);
        r = SRV_OK;
    }
    sqlite3_finalize(st);
    return r;
}

SrvResultado srv_plano_listar(int apenas_ativos, Plano **lista, int *qtd) {
    sqlite3_stmt *st;
    if (!preparar("SELECT id, nome, valor_mensal, ativo FROM planos "
                  "WHERE (?1 = 0 OR ativo = 1) ORDER BY nome COLLATE NOCASE", &st))
        return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, apenas_ativos ? 1 : 0);
    void *tmp = NULL;
    SrvResultado r = coletar(st, sizeof(Plano), preencher_plano, &tmp, qtd);
    if (r == SRV_OK) *lista = tmp;
    return r;
}

/* ========================================================================== */
/*  Beneficiários                                                             */
/* ========================================================================== */

static void preencher_beneficiario(void *dest, sqlite3_stmt *st) {
    Beneficiario *b = dest;
    b->id = sqlite3_column_int(st, 0);
    copiar_coluna(b->nome, sizeof b->nome, st, 1);
    copiar_coluna(b->cpf, sizeof b->cpf, st, 2);
    copiar_coluna(b->data_nascimento, sizeof b->data_nascimento, st, 3);
    copiar_coluna(b->telefone, sizeof b->telefone, st, 4);
    b->plano_id = sqlite3_column_int(st, 5);
    b->ativo = sqlite3_column_int(st, 6);
    copiar_coluna(b->plano_nome, sizeof b->plano_nome, st, 7);
}

#define SQL_BENEFICIARIO_BASE \
    "SELECT b.id, b.nome, b.cpf, b.data_nascimento, COALESCE(b.telefone, ''), " \
    "b.plano_id, b.ativo, p.nome " \
    "FROM beneficiarios b JOIN planos p ON p.id = b.plano_id "

static SrvResultado validar_beneficiario(Beneficiario *b, int id_ignorar) {
    models_trim(b->nome);
    if (!b->nome[0]) return SRV_ERRO_CAMPO_VAZIO;

    SrvResultado r = srv_normalizar_cpf(b->cpf, b->cpf);
    if (r != SRV_OK) return r;
    r = srv_normalizar_telefone(b->telefone, b->telefone);
    if (r != SRV_OK) return r;

    if (!iso_data_valida(b->data_nascimento)) return SRV_ERRO_DATA_INVALIDA;
    char hoje[11];
    agora_formatado(hoje, sizeof hoje, "%Y-%m-%d");
    if (strcmp(b->data_nascimento, hoje) > 0) return SRV_ERRO_DATA_INVALIDA;

    if (obter_ativo("SELECT ativo FROM planos WHERE id = ?1", b->plano_id) != 1)
        return SRV_ERRO_PLANO_INVALIDO;

    if (existe_texto("SELECT 1 FROM beneficiarios WHERE cpf = ?1 AND id <> ?2 LIMIT 1",
                     b->cpf, id_ignorar))
        return SRV_ERRO_CPF_DUPLICADO;
    return SRV_OK;
}

SrvResultado srv_beneficiario_inserir(Beneficiario *b) {
    SrvResultado v = validar_beneficiario(b, 0);
    if (v != SRV_OK) return v;

    sqlite3_stmt *st;
    if (!preparar("INSERT INTO beneficiarios (nome, cpf, data_nascimento, telefone, plano_id, ativo) "
                  "VALUES (?1, ?2, ?3, ?4, ?5, 1)", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, b->nome);
    bind_texto(st, 2, b->cpf);
    bind_texto(st, 3, b->data_nascimento);
    bind_texto(st, 4, b->telefone);
    sqlite3_bind_int(st, 5, b->plano_id);
    if (!concluir(st)) return SRV_ERRO_DB;

    b->id = (int)sqlite3_last_insert_rowid(db_get());
    b->ativo = 1;
    return SRV_OK;
}

SrvResultado srv_beneficiario_atualizar(const Beneficiario *b_in) {
    Beneficiario b = *b_in;
    if (obter_ativo("SELECT ativo FROM beneficiarios WHERE id = ?1", b.id) < 0)
        return SRV_ERRO_NAO_ENCONTRADO;
    SrvResultado v = validar_beneficiario(&b, b.id);
    if (v != SRV_OK) return v;

    sqlite3_stmt *st;
    if (!preparar("UPDATE beneficiarios SET nome = ?1, cpf = ?2, data_nascimento = ?3, "
                  "telefone = ?4, plano_id = ?5 WHERE id = ?6", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, b.nome);
    bind_texto(st, 2, b.cpf);
    bind_texto(st, 3, b.data_nascimento);
    bind_texto(st, 4, b.telefone);
    sqlite3_bind_int(st, 5, b.plano_id);
    sqlite3_bind_int(st, 6, b.id);
    return concluir(st) ? SRV_OK : SRV_ERRO_DB;
}

SrvResultado srv_beneficiario_alterar_ativo(int id, int ativo) {
    if (obter_ativo("SELECT ativo FROM beneficiarios WHERE id = ?1", id) < 0)
        return SRV_ERRO_NAO_ENCONTRADO;

    if (ativo) {
        if (obter_ativo("SELECT p.ativo FROM planos p JOIN beneficiarios b ON b.plano_id = p.id "
                        "WHERE b.id = ?1", id) != 1)
            return SRV_ERRO_PLANO_INVALIDO;
    } else if (contar_por_id("SELECT COUNT(*) FROM consultas "
                             "WHERE beneficiario_id = ?1 AND status = 'AGENDADA'", id) > 0) {
        return SRV_ERRO_TEM_AGENDAMENTOS;
    }

    sqlite3_stmt *st;
    if (!preparar("UPDATE beneficiarios SET ativo = ?1 WHERE id = ?2", &st)) return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, ativo ? 1 : 0);
    sqlite3_bind_int(st, 2, id);
    return concluir(st) ? SRV_OK : SRV_ERRO_DB;
}

SrvResultado srv_beneficiario_buscar(int id, Beneficiario *out) {
    sqlite3_stmt *st;
    if (!preparar(SQL_BENEFICIARIO_BASE "WHERE b.id = ?1", &st)) return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, id);
    SrvResultado r = SRV_ERRO_NAO_ENCONTRADO;
    if (sqlite3_step(st) == SQLITE_ROW) {
        memset(out, 0, sizeof *out);
        preencher_beneficiario(out, st);
        r = SRV_OK;
    }
    sqlite3_finalize(st);
    return r;
}

SrvResultado srv_beneficiario_listar(const char *filtro, int apenas_ativos,
                                     Beneficiario **lista, int *qtd) {
    sqlite3_stmt *st;
    if (!preparar(SQL_BENEFICIARIO_BASE
                  "WHERE (?1 = '' OR b.nome LIKE '%' || ?1 || '%' OR b.cpf LIKE '%' || ?1 || '%') "
                  "AND (?2 = 0 OR b.ativo = 1) "
                  "ORDER BY b.nome COLLATE NOCASE", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, filtro);
    sqlite3_bind_int(st, 2, apenas_ativos ? 1 : 0);
    void *tmp = NULL;
    SrvResultado r = coletar(st, sizeof(Beneficiario), preencher_beneficiario, &tmp, qtd);
    if (r == SRV_OK) *lista = tmp;
    return r;
}

/* ========================================================================== */
/*  Médicos                                                                   */
/* ========================================================================== */

static void preencher_medico(void *dest, sqlite3_stmt *st) {
    Medico *m = dest;
    m->id = sqlite3_column_int(st, 0);
    copiar_coluna(m->nome, sizeof m->nome, st, 1);
    copiar_coluna(m->crm, sizeof m->crm, st, 2);
    copiar_coluna(m->especialidade, sizeof m->especialidade, st, 3);
    m->ativo = sqlite3_column_int(st, 4);
}

static SrvResultado validar_medico(Medico *m, int id_ignorar) {
    models_trim(m->nome);
    models_trim(m->crm);
    models_trim(m->especialidade);
    if (!m->nome[0] || !m->crm[0] || !m->especialidade[0]) return SRV_ERRO_CAMPO_VAZIO;
    if (existe_texto("SELECT 1 FROM medicos WHERE crm = ?1 COLLATE NOCASE AND id <> ?2 LIMIT 1",
                     m->crm, id_ignorar))
        return SRV_ERRO_CRM_DUPLICADO;
    return SRV_OK;
}

SrvResultado srv_medico_inserir(Medico *m) {
    SrvResultado v = validar_medico(m, 0);
    if (v != SRV_OK) return v;

    sqlite3_stmt *st;
    if (!preparar("INSERT INTO medicos (nome, crm, especialidade, ativo) VALUES (?1, ?2, ?3, 1)", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, m->nome);
    bind_texto(st, 2, m->crm);
    bind_texto(st, 3, m->especialidade);
    if (!concluir(st)) return SRV_ERRO_DB;

    m->id = (int)sqlite3_last_insert_rowid(db_get());
    m->ativo = 1;
    return SRV_OK;
}

SrvResultado srv_medico_atualizar(const Medico *m_in) {
    Medico m = *m_in;
    if (obter_ativo("SELECT ativo FROM medicos WHERE id = ?1", m.id) < 0)
        return SRV_ERRO_NAO_ENCONTRADO;
    SrvResultado v = validar_medico(&m, m.id);
    if (v != SRV_OK) return v;

    sqlite3_stmt *st;
    if (!preparar("UPDATE medicos SET nome = ?1, crm = ?2, especialidade = ?3 WHERE id = ?4", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, m.nome);
    bind_texto(st, 2, m.crm);
    bind_texto(st, 3, m.especialidade);
    sqlite3_bind_int(st, 4, m.id);
    return concluir(st) ? SRV_OK : SRV_ERRO_DB;
}

SrvResultado srv_medico_alterar_ativo(int id, int ativo) {
    if (obter_ativo("SELECT ativo FROM medicos WHERE id = ?1", id) < 0)
        return SRV_ERRO_NAO_ENCONTRADO;
    if (!ativo &&
        contar_por_id("SELECT COUNT(*) FROM consultas WHERE medico_id = ?1 AND status = 'AGENDADA'", id) > 0)
        return SRV_ERRO_TEM_AGENDAMENTOS;

    sqlite3_stmt *st;
    if (!preparar("UPDATE medicos SET ativo = ?1 WHERE id = ?2", &st)) return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, ativo ? 1 : 0);
    sqlite3_bind_int(st, 2, id);
    return concluir(st) ? SRV_OK : SRV_ERRO_DB;
}

SrvResultado srv_medico_buscar(int id, Medico *out) {
    sqlite3_stmt *st;
    if (!preparar("SELECT id, nome, crm, especialidade, ativo FROM medicos WHERE id = ?1", &st))
        return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, id);
    SrvResultado r = SRV_ERRO_NAO_ENCONTRADO;
    if (sqlite3_step(st) == SQLITE_ROW) {
        memset(out, 0, sizeof *out);
        preencher_medico(out, st);
        r = SRV_OK;
    }
    sqlite3_finalize(st);
    return r;
}

SrvResultado srv_medico_listar(const char *filtro, int apenas_ativos, Medico **lista, int *qtd) {
    sqlite3_stmt *st;
    if (!preparar("SELECT id, nome, crm, especialidade, ativo FROM medicos "
                  "WHERE (?1 = '' OR nome LIKE '%' || ?1 || '%' OR crm LIKE '%' || ?1 || '%' "
                  "      OR especialidade LIKE '%' || ?1 || '%') "
                  "AND (?2 = 0 OR ativo = 1) ORDER BY nome COLLATE NOCASE", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, filtro);
    sqlite3_bind_int(st, 2, apenas_ativos ? 1 : 0);
    void *tmp = NULL;
    SrvResultado r = coletar(st, sizeof(Medico), preencher_medico, &tmp, qtd);
    if (r == SRV_OK) *lista = tmp;
    return r;
}

/* ========================================================================== */
/*  Consultas                                                                 */
/* ========================================================================== */

static void preencher_consulta(void *dest, sqlite3_stmt *st) {
    Consulta *c = dest;
    c->id = sqlite3_column_int(st, 0);
    c->beneficiario_id = sqlite3_column_int(st, 1);
    c->medico_id = sqlite3_column_int(st, 2);
    copiar_coluna(c->data_hora, sizeof c->data_hora, st, 3);
    copiar_coluna(c->status, sizeof c->status, st, 4);
    copiar_coluna(c->observacao, sizeof c->observacao, st, 5);
    copiar_coluna(c->beneficiario_nome, sizeof c->beneficiario_nome, st, 6);
    copiar_coluna(c->medico_nome, sizeof c->medico_nome, st, 7);
}

/* coluna: "medico_id" ou "beneficiario_id" (constantes internas) */
static int existe_consulta_no_horario(const char *coluna, int id, const char *data_hora) {
    char sql[200];
    snprintf(sql, sizeof sql,
             "SELECT 1 FROM consultas WHERE %s = ?1 AND data_hora = ?2 "
             "AND status <> 'CANCELADA' LIMIT 1", coluna);
    sqlite3_stmt *st;
    if (!preparar(sql, &st)) return 0;
    sqlite3_bind_int(st, 1, id);
    bind_texto(st, 2, data_hora);
    int achou = (sqlite3_step(st) == SQLITE_ROW);
    sqlite3_finalize(st);
    return achou;
}

SrvResultado srv_consulta_agendar(Consulta *c) {
    models_trim(c->observacao);

    if (!iso_datahora_valida(c->data_hora)) return SRV_ERRO_DATAHORA_INVALIDA;
    char agora[17];
    agora_formatado(agora, sizeof agora, "%Y-%m-%d %H:%M");
    if (strcmp(c->data_hora, agora) < 0) return SRV_ERRO_DATAHORA_PASSADA;

    int ativo = obter_ativo("SELECT ativo FROM beneficiarios WHERE id = ?1", c->beneficiario_id);
    if (ativo < 0) return SRV_ERRO_NAO_ENCONTRADO;
    if (!ativo) return SRV_ERRO_BENEFICIARIO_INATIVO;

    ativo = obter_ativo("SELECT ativo FROM medicos WHERE id = ?1", c->medico_id);
    if (ativo < 0) return SRV_ERRO_NAO_ENCONTRADO;
    if (!ativo) return SRV_ERRO_MEDICO_INATIVO;

    if (existe_consulta_no_horario("medico_id", c->medico_id, c->data_hora))
        return SRV_ERRO_CONFLITO_MEDICO;
    if (existe_consulta_no_horario("beneficiario_id", c->beneficiario_id, c->data_hora))
        return SRV_ERRO_CONFLITO_BENEFICIARIO;

    sqlite3_stmt *st;
    if (!preparar("INSERT INTO consultas (beneficiario_id, medico_id, data_hora, status, observacao) "
                  "VALUES (?1, ?2, ?3, 'AGENDADA', ?4)", &st))
        return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, c->beneficiario_id);
    sqlite3_bind_int(st, 2, c->medico_id);
    bind_texto(st, 3, c->data_hora);
    bind_texto(st, 4, c->observacao);
    if (!concluir(st)) return SRV_ERRO_DB;

    c->id = (int)sqlite3_last_insert_rowid(db_get());
    models_copiar(c->status, sizeof c->status, STATUS_AGENDADA);
    return SRV_OK;
}

SrvResultado srv_consulta_alterar_status(int id, const char *novo_status, const char *observacao) {
    if (!novo_status ||
        (strcmp(novo_status, STATUS_REALIZADA) != 0 && strcmp(novo_status, STATUS_CANCELADA) != 0))
        return SRV_ERRO_STATUS_INVALIDO;

    sqlite3_stmt *st;
    if (!preparar("SELECT status FROM consultas WHERE id = ?1", &st)) return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, id);
    char atual[12] = "";
    int achou = (sqlite3_step(st) == SQLITE_ROW);
    if (achou) copiar_coluna(atual, sizeof atual, st, 0);
    sqlite3_finalize(st);

    if (!achou) return SRV_ERRO_NAO_ENCONTRADO;
    if (strcmp(atual, STATUS_AGENDADA) != 0) return SRV_ERRO_STATUS_INVALIDO;

    char obs[200];
    models_copiar(obs, sizeof obs, observacao);
    models_trim(obs);

    if (!preparar("UPDATE consultas SET status = ?1, "
                  "observacao = CASE WHEN ?2 <> '' THEN ?2 ELSE observacao END WHERE id = ?3", &st))
        return SRV_ERRO_DB;
    bind_texto(st, 1, novo_status);
    bind_texto(st, 2, obs);
    sqlite3_bind_int(st, 3, id);
    return concluir(st) ? SRV_OK : SRV_ERRO_DB;
}

SrvResultado srv_consulta_listar(int beneficiario_id, const char *status_filtro,
                                 Consulta **lista, int *qtd) {
    sqlite3_stmt *st;
    if (!preparar("SELECT c.id, c.beneficiario_id, c.medico_id, c.data_hora, c.status, "
                  "COALESCE(c.observacao, ''), b.nome, m.nome "
                  "FROM consultas c "
                  "JOIN beneficiarios b ON b.id = c.beneficiario_id "
                  "JOIN medicos m ON m.id = c.medico_id "
                  "WHERE (?1 = 0 OR c.beneficiario_id = ?1) AND (?2 = '' OR c.status = ?2) "
                  "ORDER BY c.data_hora DESC, c.id DESC", &st))
        return SRV_ERRO_DB;
    sqlite3_bind_int(st, 1, beneficiario_id);
    bind_texto(st, 2, status_filtro);
    void *tmp = NULL;
    SrvResultado r = coletar(st, sizeof(Consulta), preencher_consulta, &tmp, qtd);
    if (r == SRV_OK) *lista = tmp;
    return r;
}

/* ========================================================================== */
/*  Relatórios                                                                */
/* ========================================================================== */

typedef struct {
    char  *dados;
    size_t tam, cap;
} Buf;

static void buf_add(Buf *b, const char *fmt, ...) {
    char tmp[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);

    size_t n = strlen(tmp);
    if (b->tam + n + 1 > b->cap) {
        size_t nova = b->cap ? b->cap * 2 : 1024;
        while (nova < b->tam + n + 1) nova *= 2;
        char *t = realloc(b->dados, nova);
        if (!t) return;
        b->dados = t;
        b->cap = nova;
    }
    memcpy(b->dados + b->tam, tmp, n + 1);
    b->tam += n;
}

/* Escreve s em uma coluna de 'largura' caracteres (conta UTF-8 corretamente). */
static void buf_coluna(Buf *b, const char *s, int largura) {
    int chars = 0;
    size_t i = 0;
    for (; s[i]; i++) {
        if (((unsigned char)s[i] & 0xC0) != 0x80) {
            if (chars == largura) break;
            chars++;
        }
    }
    char tmp[256];
    size_t n = i < sizeof tmp - 1 ? i : sizeof tmp - 1;
    memcpy(tmp, s, n);
    tmp[n] = '\0';
    buf_add(b, "%s", tmp);
    for (int k = chars; k < largura; k++) buf_add(b, " ");
    buf_add(b, " ");
}

static void buf_linha(Buf *b, int n) {
    for (int i = 0; i < n; i++) buf_add(b, "-");
    buf_add(b, "\n");
}

static void buf_moeda(Buf *b, double v, int largura) {
    char tmp[48];
    snprintf(tmp, sizeof tmp, "R$ %.2f", v);
    buf_coluna(b, tmp, largura);
}

static void buf_inteiro(Buf *b, int v, int largura) {
    char tmp[24];
    snprintf(tmp, sizeof tmp, "%d", v);
    buf_coluna(b, tmp, largura);
}

const char *srv_relatorio_titulo(int tipo) {
    switch (tipo) {
    case REL_BENEFICIARIOS_POR_PLANO: return "Beneficiários por plano";
    case REL_CONSULTAS_POR_MEDICO:    return "Consultas por médico";
    case REL_CONSULTAS_POR_STATUS:    return "Consultas por status";
    case REL_RESUMO_GERAL:            return "Resumo geral";
    }
    return "Relatório";
}

static void rel_beneficiarios_por_plano(Buf *b) {
    sqlite3_stmt *st;
    if (!preparar("SELECT p.nome, p.valor_mensal, p.ativo, COUNT(b.id) "
                  "FROM planos p LEFT JOIN beneficiarios b ON b.plano_id = p.id AND b.ativo = 1 "
                  "GROUP BY p.id ORDER BY p.nome COLLATE NOCASE", &st)) {
        buf_add(b, "Erro ao consultar o banco.\n");
        return;
    }
    buf_coluna(b, "Plano", 22);
    buf_coluna(b, "Valor", 12);
    buf_coluna(b, "Situação", 9);
    buf_coluna(b, "Benef. ativos", 13);
    buf_coluna(b, "Receita mensal", 14);
    buf_add(b, "\n");
    buf_linha(b, 80);

    int total_benef = 0;
    double total_receita = 0.0;
    while (sqlite3_step(st) == SQLITE_ROW) {
        const char *nome = (const char *)sqlite3_column_text(st, 0);
        double valor = sqlite3_column_double(st, 1);
        int ativo = sqlite3_column_int(st, 2);
        int qtd = sqlite3_column_int(st, 3);
        buf_coluna(b, nome ? nome : "", 22);
        buf_moeda(b, valor, 12);
        buf_coluna(b, ativo ? "Ativo" : "Inativo", 9);
        buf_inteiro(b, qtd, 13);
        buf_moeda(b, valor * qtd, 14);
        buf_add(b, "\n");
        total_benef += qtd;
        total_receita += valor * qtd;
    }
    sqlite3_finalize(st);

    buf_linha(b, 80);
    buf_coluna(b, "TOTAL", 22);
    buf_coluna(b, "", 12);
    buf_coluna(b, "", 9);
    buf_inteiro(b, total_benef, 13);
    buf_moeda(b, total_receita, 14);
    buf_add(b, "\n");
}

static void rel_consultas_por_medico(Buf *b) {
    sqlite3_stmt *st;
    if (!preparar("SELECT m.nome, m.especialidade, COUNT(c.id), "
                  "SUM(c.status = 'AGENDADA'), SUM(c.status = 'REALIZADA'), "
                  "SUM(c.status = 'CANCELADA') "
                  "FROM medicos m LEFT JOIN consultas c ON c.medico_id = m.id "
                  "GROUP BY m.id ORDER BY m.nome COLLATE NOCASE", &st)) {
        buf_add(b, "Erro ao consultar o banco.\n");
        return;
    }
    buf_coluna(b, "Médico", 24);
    buf_coluna(b, "Especialidade", 18);
    buf_coluna(b, "Total", 6);
    buf_coluna(b, "Agend.", 7);
    buf_coluna(b, "Realiz.", 8);
    buf_coluna(b, "Cancel.", 8);
    buf_add(b, "\n");
    buf_linha(b, 80);

    int soma[4] = {0, 0, 0, 0};
    while (sqlite3_step(st) == SQLITE_ROW) {
        const char *nome = (const char *)sqlite3_column_text(st, 0);
        const char *esp = (const char *)sqlite3_column_text(st, 1);
        buf_coluna(b, nome ? nome : "", 24);
        buf_coluna(b, esp ? esp : "", 18);
        for (int i = 0; i < 4; i++) {
            int v = sqlite3_column_int(st, 2 + i);
            soma[i] += v;
            buf_inteiro(b, v, i == 0 ? 6 : (i == 1 ? 7 : 8));
        }
        buf_add(b, "\n");
    }
    sqlite3_finalize(st);

    buf_linha(b, 80);
    buf_coluna(b, "TOTAL", 24);
    buf_coluna(b, "", 18);
    for (int i = 0; i < 4; i++) buf_inteiro(b, soma[i], i == 0 ? 6 : (i == 1 ? 7 : 8));
    buf_add(b, "\n");
}

static void rel_consultas_por_status(Buf *b) {
    static const char *status[] = {STATUS_AGENDADA, STATUS_REALIZADA, STATUS_CANCELADA};
    buf_coluna(b, "Status", 14);
    buf_coluna(b, "Quantidade", 10);
    buf_add(b, "\n");
    buf_linha(b, 30);

    int total = 0;
    for (int i = 0; i < 3; i++) {
        sqlite3_stmt *st;
        int qtd = 0;
        if (preparar("SELECT COUNT(*) FROM consultas WHERE status = ?1", &st)) {
            bind_texto(st, 1, status[i]);
            if (sqlite3_step(st) == SQLITE_ROW) qtd = sqlite3_column_int(st, 0);
            sqlite3_finalize(st);
        }
        buf_coluna(b, status[i], 14);
        buf_inteiro(b, qtd, 10);
        buf_add(b, "\n");
        total += qtd;
    }
    buf_linha(b, 30);
    buf_coluna(b, "TOTAL", 14);
    buf_inteiro(b, total, 10);
    buf_add(b, "\n");
}

static void rel_resumo_geral(Buf *b) {
    int benef_a = escalar_int("SELECT COUNT(*) FROM beneficiarios WHERE ativo = 1");
    int benef_i = escalar_int("SELECT COUNT(*) FROM beneficiarios WHERE ativo = 0");
    int med_a = escalar_int("SELECT COUNT(*) FROM medicos WHERE ativo = 1");
    int med_i = escalar_int("SELECT COUNT(*) FROM medicos WHERE ativo = 0");
    int pl_a = escalar_int("SELECT COUNT(*) FROM planos WHERE ativo = 1");
    int pl_i = escalar_int("SELECT COUNT(*) FROM planos WHERE ativo = 0");
    int c_ag = escalar_int("SELECT COUNT(*) FROM consultas WHERE status = 'AGENDADA'");
    int c_re = escalar_int("SELECT COUNT(*) FROM consultas WHERE status = 'REALIZADA'");
    int c_ca = escalar_int("SELECT COUNT(*) FROM consultas WHERE status = 'CANCELADA'");
    double receita = escalar_double("SELECT COALESCE(SUM(p.valor_mensal), 0) FROM beneficiarios b "
                                    "JOIN planos p ON p.id = b.plano_id WHERE b.ativo = 1");

    buf_add(b, "Beneficiários:  %d ativos, %d inativos\n", benef_a, benef_i);
    buf_add(b, "Médicos:        %d ativos, %d inativos\n", med_a, med_i);
    buf_add(b, "Planos:         %d ativos, %d inativos\n", pl_a, pl_i);
    buf_add(b, "Consultas:      %d no total (%d agendadas, %d realizadas, %d canceladas)\n",
            c_ag + c_re + c_ca, c_ag, c_re, c_ca);
    buf_add(b, "\nReceita mensal estimada (beneficiários ativos): R$ %.2f\n", receita);
}

char *srv_relatorio_gerar(int tipo) {
    Buf b = {NULL, 0, 0};
    char data[32];
    agora_formatado(data, sizeof data, "%d/%m/%Y %H:%M");

    buf_add(&b, "PIM II - %s\n", srv_relatorio_titulo(tipo));
    buf_add(&b, "Gerado em %s\n\n", data);

    switch (tipo) {
    case REL_BENEFICIARIOS_POR_PLANO: rel_beneficiarios_por_plano(&b); break;
    case REL_CONSULTAS_POR_MEDICO:    rel_consultas_por_medico(&b); break;
    case REL_CONSULTAS_POR_STATUS:    rel_consultas_por_status(&b); break;
    case REL_RESUMO_GERAL:            rel_resumo_geral(&b); break;
    default: buf_add(&b, "Tipo de relatório desconhecido.\n"); break;
    }

    if (!b.dados) {
        b.dados = malloc(1);
        if (b.dados) b.dados[0] = '\0';
    }
    return b.dados;
}

int srv_salvar_texto(const char *caminho, const char *texto) {
    FILE *f = fopen(caminho, "wb");
    if (!f) return 0;
    int ok = fputs(texto ? texto : "", f) >= 0;
    if (fclose(f) != 0) ok = 0;
    return ok;
}