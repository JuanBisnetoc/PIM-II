#ifndef SERVICES_H
#define SERVICES_H

#include <stddef.h>
#include "models.h"

typedef enum {
    SRV_OK = 0,
    SRV_ERRO_DB,
    SRV_ERRO_NAO_ENCONTRADO,
    SRV_ERRO_CAMPO_VAZIO,
    SRV_ERRO_CPF_INVALIDO,
    SRV_ERRO_CPF_DUPLICADO,
    SRV_ERRO_DATA_INVALIDA,
    SRV_ERRO_TELEFONE_INVALIDO,
    SRV_ERRO_PLANO_INVALIDO,
    SRV_ERRO_PLANO_EM_USO,
    SRV_ERRO_NOME_DUPLICADO,
    SRV_ERRO_VALOR_INVALIDO,
    SRV_ERRO_CRM_DUPLICADO,
    SRV_ERRO_TEM_AGENDAMENTOS,
    SRV_ERRO_BENEFICIARIO_INATIVO,
    SRV_ERRO_MEDICO_INATIVO,
    SRV_ERRO_DATAHORA_INVALIDA,
    SRV_ERRO_DATAHORA_PASSADA,
    SRV_ERRO_CONFLITO_MEDICO,
    SRV_ERRO_CONFLITO_BENEFICIARIO,
    SRV_ERRO_STATUS_INVALIDO
} SrvResultado;

/* Texto amigável (UTF-8) para exibir ao usuário. */
const char *srv_mensagem(SrvResultado r);

/* ------------------------- Validações e formatação ------------------------- */
void         srv_somente_digitos(char *dest, size_t tam, const char *origem);
int          srv_cpf_valido(const char *cpf_digitos);
/* Valida e devolve o CPF só com dígitos em cpf_out (12 bytes). */
SrvResultado srv_normalizar_cpf(const char *texto, char *cpf_out);
/* Vazio é aceito. Senão exige 10 ou 11 dígitos. tel_out: 16 bytes. */
SrvResultado srv_normalizar_telefone(const char *texto, char *tel_out);

/* "DD/MM/AAAA" -> "AAAA-MM-DD" (iso: 11 bytes). Retorna 1 se válida. */
int  srv_data_br_para_iso(const char *br, char *iso, size_t tam);
/* "DD/MM/AAAA HH:MM" -> "AAAA-MM-DD HH:MM" (iso: 17 bytes). */
int  srv_datahora_br_para_iso(const char *br, char *iso, size_t tam);
void srv_data_iso_para_br(const char *iso, char *out, size_t tam);
void srv_datahora_iso_para_br(const char *iso, char *out, size_t tam);
void srv_formatar_cpf(const char *digitos, char *out, size_t tam);
void srv_formatar_telefone(const char *digitos, char *out, size_t tam);

/* ------------------------------- Sprint 3: planos -------------------------- */
SrvResultado srv_plano_inserir(Plano *p);
SrvResultado srv_plano_atualizar(const Plano *p);
SrvResultado srv_plano_alterar_ativo(int id, int ativo);
SrvResultado srv_plano_buscar(int id, Plano *out);
/* O vetor devolvido deve ser liberado com free(). */
SrvResultado srv_plano_listar(int apenas_ativos, Plano **lista, int *qtd);

/* ---------------------------- Sprint 2: beneficiários ---------------------- */
SrvResultado srv_beneficiario_inserir(Beneficiario *b);
SrvResultado srv_beneficiario_atualizar(const Beneficiario *b);
SrvResultado srv_beneficiario_alterar_ativo(int id, int ativo);
SrvResultado srv_beneficiario_buscar(int id, Beneficiario *out);
/* filtro: parte do nome ou do CPF ("" = todos). */
SrvResultado srv_beneficiario_listar(const char *filtro, int apenas_ativos,
                                     Beneficiario **lista, int *qtd);

/* ------------------------------ Sprint 3: médicos -------------------------- */
SrvResultado srv_medico_inserir(Medico *m);
SrvResultado srv_medico_atualizar(const Medico *m);
SrvResultado srv_medico_alterar_ativo(int id, int ativo);
SrvResultado srv_medico_buscar(int id, Medico *out);
/* filtro: parte do nome, CRM ou especialidade ("" = todos). */
SrvResultado srv_medico_listar(const char *filtro, int apenas_ativos,
                               Medico **lista, int *qtd);

/* ----------------------------- Sprint 4: consultas ------------------------- */
SrvResultado srv_consulta_agendar(Consulta *c);
/* Só consultas AGENDADAS mudam, e apenas para REALIZADA ou CANCELADA.
   observacao ("" = mantém a atual). */
SrvResultado srv_consulta_alterar_status(int id, const char *novo_status,
                                         const char *observacao);
/* Remove definitivamente a consulta, em qualquer status. */
SrvResultado srv_consulta_excluir(int id);
/* beneficiario_id = 0 -> todos; status_filtro "" -> todos. */
SrvResultado srv_consulta_listar(int beneficiario_id, const char *status_filtro,
                                 Consulta **lista, int *qtd);

/* ----------------------------- Sprint 5: relatórios ------------------------ */
typedef enum {
    REL_BENEFICIARIOS_POR_PLANO = 0,
    REL_CONSULTAS_POR_MEDICO,
    REL_CONSULTAS_POR_STATUS,
    REL_RESUMO_GERAL,
    REL_TOTAL
} TipoRelatorio;

const char *srv_relatorio_titulo(int tipo);
/* Texto UTF-8 (liberar com free()). */
char *srv_relatorio_gerar(int tipo);
int   srv_salvar_texto(const char *caminho, const char *texto);

#endif