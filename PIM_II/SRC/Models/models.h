#ifndef MODELS_H
#define MODELS_H

#include <stddef.h>

#define STATUS_AGENDADA  "AGENDADA"
#define STATUS_REALIZADA "REALIZADA"
#define STATUS_CANCELADA "CANCELADA"

typedef struct {
    int    id;
    char   nome[100];
    double valor_mensal;
    int    ativo;
} Plano;

typedef struct {
    int  id;
    char nome[100];
    char cpf[12];              /* somente dígitos */
    char data_nascimento[11];  /* ISO: AAAA-MM-DD */
    char telefone[16];         /* somente dígitos (pode ser vazio) */
    int  plano_id;
    int  ativo;
    char plano_nome[100];      /* preenchido nas listagens (JOIN) */
} Beneficiario;

typedef struct {
    int  id;
    char nome[100];
    char crm[20];
    char especialidade[60];
    int  ativo;
} Medico;

typedef struct {
    int  id;
    int  beneficiario_id;
    int  medico_id;
    char data_hora[17];        /* ISO: AAAA-MM-DD HH:MM */
    char status[12];           /* AGENDADA | REALIZADA | CANCELADA */
    char observacao[200];
    char beneficiario_nome[100]; /* preenchido nas listagens (JOIN) */
    char medico_nome[100];       /* preenchido nas listagens (JOIN) */
} Consulta;

/* Copia com garantia de terminação nula. */
void models_copiar(char *dest, size_t tam, const char *origem);
/* Remove espaços no início e no fim (in-place). */
void models_trim(char *s);

#endif
