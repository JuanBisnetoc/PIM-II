/* Sprint 2: cadastro, consulta, alteração, inativação e validações de beneficiários. */
#include "ui.h"

#include <stdlib.h>
#include "services.h"

typedef struct {
    GtkWindow    *janela;
    GtkWidget    *nome, *cpf, *nascimento, *telefone, *plano, *busca, *btn_ativo, *chk_inativos;
    GtkListBox   *lista;
    Beneficiario *itens;
    int           qtd;
    Plano        *planos;
    int           qtd_planos;
    int           sel_id;     /* 0 = novo registro */
    int           sel_ativo;
} TelaBeneficiarios;

static TelaBeneficiarios *g_tela = NULL;

/* Carrega todos os planos no dropdown (inativos aparecem marcados). */
static void carregar_planos(TelaBeneficiarios *t) {
    free(t->planos);
    t->planos = NULL;
    t->qtd_planos = 0;
    if (srv_plano_listar(0, &t->planos, &t->qtd_planos) != SRV_OK) return;

    char **nomes = g_new0(char *, t->qtd_planos + 1);
    for (int i = 0; i < t->qtd_planos; i++)
        nomes[i] = g_strdup_printf("%s (R$ %.2f)%s", t->planos[i].nome, t->planos[i].valor_mensal,
                                   t->planos[i].ativo ? "" : " [inativo]");
    ui_dropdown_preencher(t->plano, (const char *const *)nomes, t->qtd_planos);
    g_strfreev(nomes);
}

static void selecionar_plano_por_id(TelaBeneficiarios *t, int plano_id) {
    for (int i = 0; i < t->qtd_planos; i++)
        if (t->planos[i].id == plano_id) {
            gtk_drop_down_set_selected(GTK_DROP_DOWN(t->plano), (guint)i);
            return;
        }
}

static void limpar_form(TelaBeneficiarios *t) {
    ui_definir_texto(t->nome, "");
    ui_definir_texto(t->cpf, "");
    ui_definir_texto(t->nascimento, "");
    ui_definir_texto(t->telefone, "");
    carregar_planos(t);
    t->sel_id = 0;
    t->sel_ativo = 1;
    gtk_button_set_label(GTK_BUTTON(t->btn_ativo), "Inativar");
    gtk_widget_set_sensitive(t->btn_ativo, FALSE);
}

static void recarregar(TelaBeneficiarios *t) {
    int mostrar_inativos = gtk_check_button_get_active(GTK_CHECK_BUTTON(t->chk_inativos));

    ui_lista_limpar(t->lista);
    free(t->itens);
    t->itens = NULL;
    t->qtd = 0;

    SrvResultado r = srv_beneficiario_listar(ui_texto(t->busca), !mostrar_inativos,
                                             &t->itens, &t->qtd);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    for (int i = 0; i < t->qtd; i++) {
        const Beneficiario *b = &t->itens[i];
        char cpf[20], nasc[16];
        srv_formatar_cpf(b->cpf, cpf, sizeof cpf);
        srv_data_iso_para_br(b->data_nascimento, nasc, sizeof nasc);
        char *texto = g_strdup_printf("%s  |  CPF %s  |  Nasc. %s  |  Plano %s%s", b->nome, cpf,
                                      nasc, b->plano_nome, b->ativo ? "" : "  [INATIVO]");
        ui_lista_adicionar(t->lista, texto);
        g_free(texto);
    }
}

static void ao_selecionar(GtkListBox *lista, GtkListBoxRow *linha, gpointer dados) {
    (void)lista;
    TelaBeneficiarios *t = dados;
    if (!linha) return;
    int i = gtk_list_box_row_get_index(linha);
    if (i < 0 || i >= t->qtd) return;

    const Beneficiario *b = &t->itens[i];
    char cpf[20], nasc[16], tel[24];
    srv_formatar_cpf(b->cpf, cpf, sizeof cpf);
    srv_data_iso_para_br(b->data_nascimento, nasc, sizeof nasc);
    srv_formatar_telefone(b->telefone, tel, sizeof tel);

    ui_definir_texto(t->nome, b->nome);
    ui_definir_texto(t->cpf, cpf);
    ui_definir_texto(t->nascimento, nasc);
    ui_definir_texto(t->telefone, tel);
    carregar_planos(t);
    selecionar_plano_por_id(t, b->plano_id);

    t->sel_id = b->id;
    t->sel_ativo = b->ativo;
    gtk_button_set_label(GTK_BUTTON(t->btn_ativo), b->ativo ? "Inativar" : "Reativar");
    gtk_widget_set_sensitive(t->btn_ativo, TRUE);
}

static void ao_novo(GtkButton *b, gpointer dados) {
    (void)b;
    TelaBeneficiarios *t = dados;
    gtk_list_box_unselect_all(t->lista);
    limpar_form(t);
    gtk_widget_grab_focus(t->nome);
}

static void ao_salvar(GtkButton *botao, gpointer dados) {
    (void)botao;
    TelaBeneficiarios *t = dados;
    SrvResultado r;

    Beneficiario b = {0};
    b.id = t->sel_id;
    g_strlcpy(b.nome, ui_texto(t->nome), sizeof b.nome);

    if ((r = srv_normalizar_cpf(ui_texto(t->cpf), b.cpf)) != SRV_OK ||
        (r = srv_normalizar_telefone(ui_texto(t->telefone), b.telefone)) != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }

    char *nasc = g_strstrip(g_strdup(ui_texto(t->nascimento)));
    int data_ok = srv_data_br_para_iso(nasc, b.data_nascimento, sizeof b.data_nascimento);
    g_free(nasc);
    if (!data_ok) {
        ui_mensagem(t->janela, srv_mensagem(SRV_ERRO_DATA_INVALIDA));
        return;
    }

    int idx = ui_dropdown_indice(t->plano);
    if (idx < 0 || idx >= t->qtd_planos) {
        ui_mensagem(t->janela, "Cadastre um plano antes de cadastrar beneficiários.");
        return;
    }
    b.plano_id = t->planos[idx].id;

    r = b.id ? srv_beneficiario_atualizar(&b) : srv_beneficiario_inserir(&b);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    limpar_form(t);
    recarregar(t);
    ui_mensagem(t->janela, "Beneficiário salvo com sucesso.");
}

static void ao_alternar_ativo(GtkButton *b, gpointer dados) {
    (void)b;
    TelaBeneficiarios *t = dados;
    if (!t->sel_id) return;

    SrvResultado r = srv_beneficiario_alterar_ativo(t->sel_id, !t->sel_ativo);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    limpar_form(t);
    recarregar(t);
}

static void ao_filtro_texto(GtkEditable *e, gpointer dados) {
    (void)e;
    recarregar(dados);
}

static void ao_filtro_check(GtkCheckButton *c, gpointer dados) {
    (void)c;
    recarregar(dados);
}

static void ao_destruir(GtkWidget *w, gpointer dados) {
    (void)w;
    TelaBeneficiarios *t = dados;
    free(t->itens);
    free(t->planos);
    g_free(t);
    g_tela = NULL;
}

void ui_beneficiarios_abrir(GtkWindow *pai) {
    if (g_tela) {
        gtk_window_present(g_tela->janela);
        return;
    }
    TelaBeneficiarios *t = g_new0(TelaBeneficiarios, 1);
    g_tela = t;

    GtkWidget *janela = ui_janela_criar(pai, "Beneficiários", 760, 680);
    t->janela = GTK_WINDOW(janela);
    g_signal_connect(janela, "destroy", G_CALLBACK(ao_destruir), t);

    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    ui_margens(caixa, 14);

    GtkWidget *grade = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grade), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grade), 10);
    t->nome = ui_entry_criar(45, NULL);
    t->cpf = ui_entry_criar(14, "000.000.000-00");
    t->nascimento = ui_entry_criar(10, "DD/MM/AAAA");
    t->telefone = ui_entry_criar(15, "(11) 91234-5678 (opcional)");
    t->plano = ui_dropdown_criar();
    ui_campo(GTK_GRID(grade), 0, "Nome *", t->nome);
    ui_campo(GTK_GRID(grade), 1, "CPF *", t->cpf);
    ui_campo(GTK_GRID(grade), 2, "Data de nascimento *", t->nascimento);
    ui_campo(GTK_GRID(grade), 3, "Telefone", t->telefone);
    ui_campo(GTK_GRID(grade), 4, "Plano *", t->plano);
    gtk_box_append(GTK_BOX(caixa), grade);

    GtkWidget *botoes = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *b_novo = gtk_button_new_with_label("Novo");
    GtkWidget *b_salvar = gtk_button_new_with_label("Salvar");
    t->btn_ativo = gtk_button_new_with_label("Inativar");
    gtk_widget_add_css_class(b_salvar, "suggested-action");
    gtk_box_append(GTK_BOX(botoes), b_novo);
    gtk_box_append(GTK_BOX(botoes), b_salvar);
    gtk_box_append(GTK_BOX(botoes), t->btn_ativo);
    gtk_box_append(GTK_BOX(caixa), botoes);
    g_signal_connect(b_novo, "clicked", G_CALLBACK(ao_novo), t);
    g_signal_connect(b_salvar, "clicked", G_CALLBACK(ao_salvar), t);
    g_signal_connect(t->btn_ativo, "clicked", G_CALLBACK(ao_alternar_ativo), t);

    GtkWidget *filtros = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    t->busca = ui_entry_criar(40, "Buscar por nome ou CPF");
    gtk_widget_set_hexpand(t->busca, TRUE);
    t->chk_inativos = gtk_check_button_new_with_label("Mostrar inativos");
    gtk_box_append(GTK_BOX(filtros), t->busca);
    gtk_box_append(GTK_BOX(filtros), t->chk_inativos);
    gtk_box_append(GTK_BOX(caixa), filtros);
    g_signal_connect(t->busca, "changed", G_CALLBACK(ao_filtro_texto), t);
    g_signal_connect(t->chk_inativos, "toggled", G_CALLBACK(ao_filtro_check), t);

    gtk_box_append(GTK_BOX(caixa), ui_lista_criar(&t->lista));
    g_signal_connect(t->lista, "row-selected", G_CALLBACK(ao_selecionar), t);

    gtk_window_set_child(GTK_WINDOW(janela), caixa);
    limpar_form(t);
    recarregar(t);
    gtk_window_present(GTK_WINDOW(janela));
}
