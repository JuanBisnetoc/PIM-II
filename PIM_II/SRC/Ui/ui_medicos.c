/* Sprint 3: cadastro, consulta e alteração de médicos. */
#include "ui.h"

#include <stdlib.h>
#include "services.h"

typedef struct {
    GtkWindow  *janela;
    GtkWidget  *nome, *crm, *especialidade, *busca, *btn_ativo, *chk_inativos;
    GtkListBox *lista;
    Medico     *itens;
    int         qtd;
    int         sel_id;     /* 0 = novo registro */
    int         sel_ativo;
} TelaMedicos;

static TelaMedicos *g_tela = NULL;

static void limpar_form(TelaMedicos *t) {
    ui_definir_texto(t->nome, "");
    ui_definir_texto(t->crm, "");
    ui_definir_texto(t->especialidade, "");
    t->sel_id = 0;
    t->sel_ativo = 1;
    gtk_button_set_label(GTK_BUTTON(t->btn_ativo), "Inativar");
    gtk_widget_set_sensitive(t->btn_ativo, FALSE);
}

static void recarregar(TelaMedicos *t) {
    int mostrar_inativos = gtk_check_button_get_active(GTK_CHECK_BUTTON(t->chk_inativos));

    ui_lista_limpar(t->lista);
    free(t->itens);
    t->itens = NULL;
    t->qtd = 0;

    SrvResultado r = srv_medico_listar(ui_texto(t->busca), !mostrar_inativos, &t->itens, &t->qtd);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    for (int i = 0; i < t->qtd; i++) {
        const Medico *m = &t->itens[i];
        char *texto = g_strdup_printf("%s  |  %s  |  %s%s", m->nome, m->crm, m->especialidade,
                                      m->ativo ? "" : "  [INATIVO]");
        ui_lista_adicionar(t->lista, texto);
        g_free(texto);
    }
}

static void ao_selecionar(GtkListBox *lista, GtkListBoxRow *linha, gpointer dados) {
    (void)lista;
    TelaMedicos *t = dados;
    if (!linha) return;
    int i = gtk_list_box_row_get_index(linha);
    if (i < 0 || i >= t->qtd) return;

    const Medico *m = &t->itens[i];
    ui_definir_texto(t->nome, m->nome);
    ui_definir_texto(t->crm, m->crm);
    ui_definir_texto(t->especialidade, m->especialidade);
    t->sel_id = m->id;
    t->sel_ativo = m->ativo;
    gtk_button_set_label(GTK_BUTTON(t->btn_ativo), m->ativo ? "Inativar" : "Reativar");
    gtk_widget_set_sensitive(t->btn_ativo, TRUE);
}

static void ao_novo(GtkButton *b, gpointer dados) {
    (void)b;
    TelaMedicos *t = dados;
    gtk_list_box_unselect_all(t->lista);
    limpar_form(t);
    gtk_widget_grab_focus(t->nome);
}

static void ao_salvar(GtkButton *b, gpointer dados) {
    (void)b;
    TelaMedicos *t = dados;

    Medico m = {0};
    m.id = t->sel_id;
    g_strlcpy(m.nome, ui_texto(t->nome), sizeof m.nome);
    g_strlcpy(m.crm, ui_texto(t->crm), sizeof m.crm);
    g_strlcpy(m.especialidade, ui_texto(t->especialidade), sizeof m.especialidade);

    SrvResultado r = m.id ? srv_medico_atualizar(&m) : srv_medico_inserir(&m);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    limpar_form(t);
    recarregar(t);
    ui_mensagem(t->janela, "Médico salvo com sucesso.");
}

static void ao_alternar_ativo(GtkButton *b, gpointer dados) {
    (void)b;
    TelaMedicos *t = dados;
    if (!t->sel_id) return;

    SrvResultado r = srv_medico_alterar_ativo(t->sel_id, !t->sel_ativo);
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
    ui_desconectar(w, dados);   /* nenhum callback roda após o free */
    TelaMedicos *t = dados;
    free(t->itens);
    g_free(t);
    g_tela = NULL;
}

void ui_medicos_abrir(GtkWindow *pai) {
    if (g_tela) {
        gtk_window_present(g_tela->janela);
        return;
    }
    TelaMedicos *t = g_new0(TelaMedicos, 1);
    g_tela = t;

    GtkWidget *janela = ui_janela_criar(pai, "Médicos", 680, 620);
    t->janela = GTK_WINDOW(janela);
    g_signal_connect(janela, "destroy", G_CALLBACK(ao_destruir), t);

    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    ui_margens(caixa, 18);

    GtkWidget *titulo = gtk_label_new("Dados do médico");
    gtk_label_set_xalign(GTK_LABEL(titulo), 0.0f);
    gtk_widget_add_css_class(titulo, "secao-titulo");
    gtk_box_append(GTK_BOX(caixa), titulo);

    GtkWidget *grade = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grade), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grade), 10);
    t->nome = ui_entry_criar(45, NULL);
    t->crm = ui_entry_criar(19, "Ex.: CRM-SP 123456");
    t->especialidade = ui_entry_criar(30, "Ex.: Cardiologia");
    ui_campo(GTK_GRID(grade), 0, "Nome *", t->nome);
    ui_campo(GTK_GRID(grade), 1, "CRM *", t->crm);
    ui_campo(GTK_GRID(grade), 2, "Especialidade *", t->especialidade);
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

    gtk_box_append(GTK_BOX(caixa), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *filtros = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    t->busca = ui_entry_criar(40, "Buscar por nome, CRM ou especialidade");
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