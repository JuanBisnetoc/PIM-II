/* Testes dos services (sem interface) usando um banco SQLite em memória. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DataBase.h"
#include "services.h"

static int falhas = 0, total = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        total++;                                                             \
        if (!(cond)) {                                                       \
            falhas++;                                                        \
            printf("  FALHOU (linha %d): %s\n", __LINE__, #cond);            \
        }                                                                    \
    } while (0)

#define CHECK_RES(esperado, expr) CHECK((expr) == (esperado))

static Beneficiario novo_benef(const char *nome, const char *cpf, int plano_id) {
    Beneficiario b;
    memset(&b, 0, sizeof b);
    models_copiar(b.nome, sizeof b.nome, nome);
    models_copiar(b.cpf, sizeof b.cpf, cpf);
    models_copiar(b.data_nascimento, sizeof b.data_nascimento, "1990-05-20");
    b.plano_id = plano_id;
    return b;
}

static void teste_validacoes(void) {
    printf("Validações\n");
    CHECK(srv_cpf_valido("52998224725"));
    CHECK(!srv_cpf_valido("52998224724"));
    CHECK(!srv_cpf_valido("11111111111"));
    CHECK(!srv_cpf_valido("123"));

    char cpf[12], tel[16], iso[17];
    CHECK_RES(SRV_OK, srv_normalizar_cpf("529.982.247-25", cpf));
    CHECK(strcmp(cpf, "52998224725") == 0);
    CHECK_RES(SRV_ERRO_CPF_INVALIDO, srv_normalizar_cpf("529.982.247-250", cpf));
    CHECK_RES(SRV_OK, srv_normalizar_telefone("(11) 91234-5678", tel));
    CHECK(strcmp(tel, "11912345678") == 0);
    CHECK_RES(SRV_OK, srv_normalizar_telefone("", tel));
    CHECK_RES(SRV_ERRO_TELEFONE_INVALIDO, srv_normalizar_telefone("1234", tel));

    CHECK(srv_data_br_para_iso("29/02/2024", iso, sizeof iso));
    CHECK(strcmp(iso, "2024-02-29") == 0);
    CHECK(!srv_data_br_para_iso("29/02/2023", iso, sizeof iso));
    CHECK(!srv_data_br_para_iso("31/04/2024", iso, sizeof iso));
    CHECK(!srv_data_br_para_iso("abc", iso, sizeof iso));
    CHECK(srv_datahora_br_para_iso("05/10/2030 14:30", iso, sizeof iso));
    CHECK(strcmp(iso, "2030-10-05 14:30") == 0);
    CHECK(!srv_datahora_br_para_iso("05/10/2030 25:00", iso, sizeof iso));

    char fmt[32];
    srv_formatar_cpf("52998224725", fmt, sizeof fmt);
    CHECK(strcmp(fmt, "529.982.247-25") == 0);
    srv_formatar_telefone("11912345678", fmt, sizeof fmt);
    CHECK(strcmp(fmt, "(11) 91234-5678") == 0);
}

static void teste_planos(void) {
    printf("Planos\n");
    Plano p = {0, "Teste", 100.0, 1};
    CHECK_RES(SRV_OK, srv_plano_inserir(&p));
    CHECK(p.id > 0);

    Plano dup = {0, "teste", 50.0, 1};
    CHECK_RES(SRV_ERRO_NOME_DUPLICADO, srv_plano_inserir(&dup));
    Plano zero = {0, "Zerado", 0.0, 1};
    CHECK_RES(SRV_ERRO_VALOR_INVALIDO, srv_plano_inserir(&zero));
    Plano vazio = {0, "   ", 10.0, 1};
    CHECK_RES(SRV_ERRO_CAMPO_VAZIO, srv_plano_inserir(&vazio));

    p.valor_mensal = 120.0;
    CHECK_RES(SRV_OK, srv_plano_atualizar(&p));
    Plano lido;
    CHECK_RES(SRV_OK, srv_plano_buscar(p.id, &lido));
    CHECK(lido.valor_mensal == 120.0);

    CHECK_RES(SRV_OK, srv_plano_alterar_ativo(p.id, 0));
    Plano *lista = NULL;
    int qtd = 0;
    CHECK_RES(SRV_OK, srv_plano_listar(1, &lista, &qtd));
    int achou = 0;
    for (int i = 0; i < qtd; i++) achou |= (lista[i].id == p.id);
    CHECK(!achou);
    free(lista);
    CHECK_RES(SRV_OK, srv_plano_alterar_ativo(p.id, 1));
    CHECK_RES(SRV_ERRO_NAO_ENCONTRADO, srv_plano_alterar_ativo(9999, 1));
}

static void teste_beneficiarios(void) {
    printf("Beneficiários\n");
    Beneficiario b = novo_benef("Maria Silva", "52998224725", 1);
    CHECK_RES(SRV_OK, srv_beneficiario_inserir(&b));
    CHECK(b.id > 0);

    Beneficiario dup = novo_benef("Outra Pessoa", "52998224725", 1);
    CHECK_RES(SRV_ERRO_CPF_DUPLICADO, srv_beneficiario_inserir(&dup));
    Beneficiario cpf_ruim = novo_benef("Fulano", "11111111111", 1);
    CHECK_RES(SRV_ERRO_CPF_INVALIDO, srv_beneficiario_inserir(&cpf_ruim));
    Beneficiario plano_ruim = novo_benef("Fulano", "16899535009", 9999);
    CHECK_RES(SRV_ERRO_PLANO_INVALIDO, srv_beneficiario_inserir(&plano_ruim));
    Beneficiario futuro = novo_benef("Fulano", "16899535009", 1);
    models_copiar(futuro.data_nascimento, sizeof futuro.data_nascimento, "2999-01-01");
    CHECK_RES(SRV_ERRO_DATA_INVALIDA, srv_beneficiario_inserir(&futuro));
    Beneficiario sem_nome = novo_benef("  ", "16899535009", 1);
    CHECK_RES(SRV_ERRO_CAMPO_VAZIO, srv_beneficiario_inserir(&sem_nome));

    b.plano_id = 2;
    CHECK_RES(SRV_OK, srv_beneficiario_atualizar(&b));
    Beneficiario lido;
    CHECK_RES(SRV_OK, srv_beneficiario_buscar(b.id, &lido));
    CHECK(lido.plano_id == 2);
    CHECK(strcmp(lido.nome, "Maria Silva") == 0);

    Beneficiario *lista = NULL;
    int qtd = 0;
    CHECK_RES(SRV_OK, srv_beneficiario_listar("maria", 1, &lista, &qtd));
    CHECK(qtd == 1);
    free(lista);
    CHECK_RES(SRV_OK, srv_beneficiario_listar("zzzz", 0, &lista, &qtd));
    CHECK(qtd == 0);
    free(lista);

    /* plano em uso não pode ser inativado */
    CHECK_RES(SRV_ERRO_PLANO_EM_USO, srv_plano_alterar_ativo(2, 0));

    CHECK_RES(SRV_OK, srv_beneficiario_alterar_ativo(b.id, 0));
    CHECK_RES(SRV_OK, srv_beneficiario_listar("", 1, &lista, &qtd));
    CHECK(qtd == 0);
    free(lista);
    CHECK_RES(SRV_OK, srv_beneficiario_alterar_ativo(b.id, 1));
}

static void teste_medicos_e_consultas(void) {
    printf("Médicos e consultas\n");
    Medico m = {0, "Dr. João", "CRM-SP 12345", "Cardiologia", 1};
    CHECK_RES(SRV_OK, srv_medico_inserir(&m));
    Medico dup = {0, "Dra. Ana", "crm-sp 12345", "Pediatria", 1};
    CHECK_RES(SRV_ERRO_CRM_DUPLICADO, srv_medico_inserir(&dup));
    Medico vazio = {0, "Dra. Ana", "", "Pediatria", 1};
    CHECK_RES(SRV_ERRO_CAMPO_VAZIO, srv_medico_inserir(&vazio));

    Beneficiario *bl = NULL;
    int nb = 0;
    CHECK_RES(SRV_OK, srv_beneficiario_listar("", 1, &bl, &nb));
    CHECK(nb >= 1);
    int benef_id = bl[0].id;
    free(bl);

    Consulta c;
    memset(&c, 0, sizeof c);
    c.beneficiario_id = benef_id;
    c.medico_id = m.id;
    models_copiar(c.data_hora, sizeof c.data_hora, "2099-12-31 10:00");
    CHECK_RES(SRV_OK, srv_consulta_agendar(&c));
    CHECK(c.id > 0);

    Consulta mesmo_medico = c;
    mesmo_medico.id = 0;
    CHECK_RES(SRV_ERRO_CONFLITO_MEDICO, srv_consulta_agendar(&mesmo_medico));

    Consulta passada = c;
    models_copiar(passada.data_hora, sizeof passada.data_hora, "2000-01-01 10:00");
    CHECK_RES(SRV_ERRO_DATAHORA_PASSADA, srv_consulta_agendar(&passada));

    Consulta ruim = c;
    models_copiar(ruim.data_hora, sizeof ruim.data_hora, "lixo");
    CHECK_RES(SRV_ERRO_DATAHORA_INVALIDA, srv_consulta_agendar(&ruim));

    /* não permite inativar com consulta agendada */
    CHECK_RES(SRV_ERRO_TEM_AGENDAMENTOS, srv_medico_alterar_ativo(m.id, 0));
    CHECK_RES(SRV_ERRO_TEM_AGENDAMENTOS, srv_beneficiario_alterar_ativo(benef_id, 0));

    Consulta *cl = NULL;
    int nc = 0;
    CHECK_RES(SRV_OK, srv_consulta_listar(benef_id, "", &cl, &nc));
    CHECK(nc == 1);
    CHECK(strcmp(cl[0].status, STATUS_AGENDADA) == 0);
    CHECK(strcmp(cl[0].medico_nome, "Dr. João") == 0);
    free(cl);

    CHECK_RES(SRV_ERRO_STATUS_INVALIDO, srv_consulta_alterar_status(c.id, "XYZ", ""));
    CHECK_RES(SRV_OK, srv_consulta_alterar_status(c.id, STATUS_CANCELADA, "Paciente desistiu"));
    CHECK_RES(SRV_ERRO_STATUS_INVALIDO, srv_consulta_alterar_status(c.id, STATUS_REALIZADA, ""));
    CHECK_RES(SRV_ERRO_NAO_ENCONTRADO, srv_consulta_alterar_status(9999, STATUS_REALIZADA, ""));

    /* cancelada libera o horário */
    Consulta nova = c;
    nova.id = 0;
    CHECK_RES(SRV_OK, srv_consulta_agendar(&nova));
    CHECK_RES(SRV_OK, srv_consulta_alterar_status(nova.id, STATUS_REALIZADA, ""));

    CHECK_RES(SRV_OK, srv_consulta_listar(0, STATUS_REALIZADA, &cl, &nc));
    CHECK(nc == 1);
    free(cl);

    /* sem pendências, agora pode inativar */
    CHECK_RES(SRV_OK, srv_medico_alterar_ativo(m.id, 0));
    Consulta com_inativo = c;
    com_inativo.id = 0;
    models_copiar(com_inativo.data_hora, sizeof com_inativo.data_hora, "2099-12-31 11:00");
    CHECK_RES(SRV_ERRO_MEDICO_INATIVO, srv_consulta_agendar(&com_inativo));
}

static void teste_relatorios(void) {
    printf("Relatórios\n");
    for (int t = 0; t < REL_TOTAL; t++) {
        char *txt = srv_relatorio_gerar(t);
        CHECK(txt != NULL && strlen(txt) > 20);
        if (t == REL_RESUMO_GERAL) printf("%s\n", txt);
        if (t == REL_BENEFICIARIOS_POR_PLANO) printf("%s\n", txt);
        free(txt);
    }
    CHECK(srv_salvar_texto("/tmp/pim_ii_relatorio_teste.txt", "ok"));
}

int main(void) {
    if (!db_open(":memory:") || !db_criar_tabelas() || !db_inserir_dados_iniciais()) {
        printf("Falha ao preparar o banco de testes.\n");
        return 1;
    }
    /* idempotência dos dados iniciais */
    CHECK(db_inserir_dados_iniciais());

    teste_validacoes();
    teste_planos();
    teste_beneficiarios();
    teste_medicos_e_consultas();
    teste_relatorios();

    db_close();
    printf("\n%d verificações, %d falha(s).\n", total, falhas);
    return falhas ? 1 : 0;
}
