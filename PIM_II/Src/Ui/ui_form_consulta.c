/* Sprint 4: cadastro de consultas, consulta de histórico, alteração de status e exclusão. */
#include "ui.h"

#include <stdlib.h>
#include "services.h"

typedef struct {
    GtkWindow    *janela;
    GtkWidget    *dd_benef, *dd_medico, *data_hora, *obs;
    GtkWidget    *dd_filtro_benef, *dd_filtro_status;
    GtkWidget    *lbl_sel, *btn_realizada, *btn_cancelada, *btn_excluir;
    GtkListBox   *lista;

    Beneficiario *benef_ativos;  int n_benef_ativos;   /* para agendar */
    Beneficiario *benef_todos;   int n_benef_todos;    /* para o filtro do histórico */
    Medico       *medicos;       int n_medicos;
    Consulta     *consultas;     int n_consultas;

    int  sel_id;
    int  atualizando;            /* evita recarregar durante o preenchimento dos dropdowns */
} TelaConsultas;

static TelaConsultas *g_tela = NULL;

static const char *const ROTULOS_STATUS[] = {"Todos os status", "Agendadas", "Realizadas", "Canceladas"};
static const char *const VALORES_STATUS[] = {"", STATUS_AGENDADA, STATUS_REALIZADA, STATUS_CANCELADA};

static void atualizar_selecao(TelaConsultas *t, const Consulta *c) {
    if (!c) {
        t->sel_id = 0;
        gtk_label_set_text(GTK_LABEL(t->lbl_sel), "Nenhuma consulta selecionada.");
        gtk_widget_set_sensitive(t->btn_realizada, FALSE);
        gtk_widget_set_sensitive(t->btn_cancelada, FALSE);
        gtk_widget_set_sensitive(t->btn_excluir, FALSE);
        return;
    }
    t->sel_id = c->id;
    char *txt = g_strdup_printf("Consulta #%d selecionada (%s).", c->id, c->status);
    gtk_label_set_text(GTK_LABEL(t->lbl_sel), txt);
    g_free(txt);
    gboolean agendada = g_strcmp0(c->status, STATUS_AGENDADA) == 0;
    gtk_widget_set_sensitive(t->btn_realizada, agendada);
    gtk_widget_set_sensitive(t->btn_cancelada, agendada);
    gtk_widget_set_sensitive(t->btn_excluir, TRUE);   /* qualquer status pode ser excluído */
}

/* Recarrega o histórico conforme os filtros. */
static void recarregar_historico(TelaConsultas *t) {
    ui_lista_limpar(t->lista);
    free(t->consultas);
    t->consultas = NULL;
    t->n_consultas = 0;
    atualizar_selecao(t, NULL);

    int idx_b = ui_dropdown_indice(t->dd_filtro_benef);
    int benef_id = (idx_b > 0 && idx_b - 1 < t->n_benef_todos) ? t->benef_todos[idx_b - 1].id : 0;

    int idx_s = ui_dropdown_indice(t->dd_filtro_status);
    const char *status = (idx_s >= 0 && idx_s < 4) ? VALORES_STATUS[idx_s] : "";

    SrvResultado r = srv_consulta_listar(benef_id, status, &t->consultas, &t->n_consultas);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    for (int i = 0; i < t->n_consultas; i++) {
        const Consulta *c = &t->consultas[i];
        char data[24];
        srv_datahora_iso_para_br(c->data_hora, data, sizeof data);
        char *texto = g_strdup_printf("#%d  |  %s  |  %s  |  Dr(a). %s  |  %s%s%s", c->id, data,
                                      c->beneficiario_nome, c->medico_nome, c->status,
                                      c->observacao[0] ? "  |  Obs.: " : "", c->observacao);
        ui_lista_adicionar(t->lista, texto);
        g_free(texto);
    }
}

/* Recarrega as listas usadas nos dropdowns (beneficiários e médicos). */
static void carregar_listas(TelaConsultas *t) {
    t->atualizando = 1;

    free(t->benef_ativos); t->benef_ativos = NULL; t->n_benef_ativos = 0;
    free(t->benef_todos);  t->benef_todos = NULL;  t->n_benef_todos = 0;
    free(t->medicos);      t->medicos = NULL;      t->n_medicos = 0;

    srv_beneficiario_listar("", 1, &t->benef_ativos, &t->n_benef_ativos);
    srv_beneficiario_listar("", 0, &t->benef_todos, &t->n_benef_todos);
    srv_medico_listar("", 1, &t->medicos, &t->n_medicos);

    /* beneficiário (agendamento) */
    char **textos = g_new0(char *, t->n_benef_ativos + 1);
    for (int i = 0; i < t->n_benef_ativos; i++) {
        char cpf[20];
        srv_formatar_cpf(t->benef_ativos[i].cpf, cpf, sizeof cpf);
        textos[i] = g_strdup_printf("%s (%s)", t->benef_ativos[i].nome, cpf);
    }
    ui_dropdown_preencher(t->dd_benef, (const char *const *)textos, t->n_benef_ativos);
    g_strfreev(textos);

    /* médico (agendamento) */
    textos = g_new0(char *, t->n_medicos + 1);
    for (int i = 0; i < t->n_medicos; i++)
        textos[i] = g_strdup_printf("%s — %s (%s)", t->medicos[i].nome,
                                    t->medicos[i].especialidade, t->medicos[i].crm);
    ui_dropdown_preencher(t->dd_medico, (const char *const *)textos, t->n_medicos);
    g_strfreev(textos);

    /* filtro do histórico: "Todos" + todos os beneficiários */
    textos = g_new0(char *, t->n_benef_todos + 2);
    textos[0] = g_strdup("Todos os beneficiários");
    for (int i = 0; i < t->n_benef_todos; i++)
        textos[i + 1] = g_strdup_printf("%s%s", t->benef_todos[i].nome,
                                        t->benef_todos[i].ativo ? "" : " [inativo]");
    ui_dropdown_preencher(t->dd_filtro_benef, (const char *const *)textos, t->n_benef_todos + 1);
    g_strfreev(textos);

    t->atualizando = 0;
}

static void ao_atualizar(GtkButton *b, gpointer dados) {
    (void)b;
    TelaConsultas *t = dados;
    carregar_listas(t);
    recarregar_historico(t);
}

static void ao_filtro(GObject *obj, GParamSpec *pspec, gpointer dados) {
    (void)obj;
    (void)pspec;
    TelaConsultas *t = dados;
    if (!t->atualizando) recarregar_historico(t);
}

static void ao_selecionar(GtkListBox *lista, GtkListBoxRow *linha, gpointer dados) {
    (void)lista;
    TelaConsultas *t = dados;
    if (!linha) return;
    int i = gtk_list_box_row_get_index(linha);
    if (i < 0 || i >= t->n_consultas) return;
    atualizar_selecao(t, &t->consultas[i]);
}

static void ao_agendar(GtkButton *botao, gpointer dados) {
    (void)botao;
    TelaConsultas *t = dados;

    int ib = ui_dropdown_indice(t->dd_benef);
    int im = ui_dropdown_indice(t->dd_medico);
    if (ib < 0 || ib >= t->n_benef_ativos) {
        ui_mensagem(t->janela, "Selecione um beneficiário ativo (cadastre um, se necessário).");
        return;
    }
    if (im < 0 || im >= t->n_medicos) {
        ui_mensagem(t->janela, "Selecione um médico ativo (cadastre um, se necessário).");
        return;
    }

    Consulta c = {0};
    c.beneficiario_id = t->benef_ativos[ib].id;
    c.medico_id = t->medicos[im].id;

    char *dh = g_strstrip(g_strdup(ui_texto(t->data_hora)));
    int ok = srv_datahora_br_para_iso(dh, c.data_hora, sizeof c.data_hora);
    g_free(dh);
    if (!ok) {
        ui_mensagem(t->janela, srv_mensagem(SRV_ERRO_DATAHORA_INVALIDA));
        return;
    }
    g_strlcpy(c.observacao, ui_texto(t->obs), sizeof c.observacao);

    SrvResultado r = srv_consulta_agendar(&c);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    ui_definir_texto(t->data_hora, "");
    ui_definir_texto(t->obs, "");
    recarregar_historico(t);
    ui_mensagem(t->janela, "Consulta agendada com sucesso.");
}

static void mudar_status(TelaConsultas *t, const char *novo_status) {
    if (!t->sel_id) {
        ui_mensagem(t->janela, "Selecione uma consulta na lista.");
        return;
    }
    SrvResultado r = srv_consulta_alterar_status(t->sel_id, novo_status, ui_texto(t->obs));
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    ui_definir_texto(t->obs, "");
    recarregar_historico(t);
}

static void ao_realizada(GtkButton *b, gpointer dados) { (void)b; mudar_status(dados, STATUS_REALIZADA); }
static void ao_cancelada(GtkButton *b, gpointer dados) { (void)b; mudar_status(dados, STATUS_CANCELADA); }

/* --- Exclusão (com confirmação) --- */

/* Resposta do diálogo de confirmação. 'dados' carrega o id da consulta; a tela é
 * obtida de g_tela (se a janela já foi fechada, não há nada a fazer). */
static void ao_confirmar_exclusao(GObject *fonte, GAsyncResult *res, gpointer dados) {
    int id = GPOINTER_TO_INT(dados);
    GError *erro = NULL;
    int botao = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(fonte), res, &erro);
    if (erro) {                       /* diálogo dispensado (Esc) */
        g_error_free(erro);
        return;
    }
    TelaConsultas *t = g_tela;
    if (!t || botao != 1) return;     /* janela fechada ou "Cancelar" */

    SrvResultado r = srv_consulta_excluir(id);
    if (r != SRV_OK) {
        ui_mensagem(t->janela, srv_mensagem(r));
        return;
    }
    recarregar_historico(t);
    ui_mensagem(t->janela, "Consulta excluída com sucesso.");
}

static void ao_excluir(GtkButton *b, gpointer dados) {
    (void)b;
    TelaConsultas *t = dados;
    if (!t->sel_id) {
        ui_mensagem(t->janela, "Selecione uma consulta na lista.");
        return;
    }
    char *msg = g_strdup_printf("Excluir a consulta #%d?", t->sel_id);
    GtkAlertDialog *dlg = gtk_alert_dialog_new("%s", msg);
    gtk_alert_dialog_set_detail(dlg, "Esta ação apaga o registro definitivamente e não pode ser desfeita.");
    static const char *const botoes[] = {"Cancelar", "Excluir", NULL};
    gtk_alert_dialog_set_buttons(dlg, botoes);
    gtk_alert_dialog_set_cancel_button(dlg, 0);
    gtk_alert_dialog_set_default_button(dlg, 0);
    gtk_alert_dialog_choose(dlg, t->janela, NULL, ao_confirmar_exclusao, GINT_TO_POINTER(t->sel_id));
    g_object_unref(dlg);
    g_free(msg);
}

static void ao_destruir(GtkWidget *w, gpointer dados) {
    ui_desconectar(w, dados);   /* nenhum callback roda após o free */
    TelaConsultas *t = dados;
    free(t->benef_ativos);
    free(t->benef_todos);
    free(t->medicos);
    free(t->consultas);
    g_free(t);
    g_tela = NULL;
}

void ui_form_consulta_abrir(GtkWindow *pai) {
    if (g_tela) {
        gtk_window_present(g_tela->janela);
        return;
    }
    TelaConsultas *t = g_new0(TelaConsultas, 1);
    g_tela = t;

    GtkWidget *janela = ui_janela_criar(pai, "Consultas", 900, 740);
    t->janela = GTK_WINDOW(janela);
    g_signal_connect(janela, "destroy", G_CALLBACK(ao_destruir), t);

    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    ui_margens(caixa, 18);

    /* --- Agendamento --- */
    GtkWidget *titulo1 = gtk_label_new("Agendar consulta");
    gtk_label_set_xalign(GTK_LABEL(titulo1), 0.0f);
    gtk_widget_add_css_class(titulo1, "secao-titulo");
    gtk_box_append(GTK_BOX(caixa), titulo1);

    GtkWidget *grade = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grade), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grade), 10);
    t->dd_benef = ui_dropdown_criar();
    t->dd_medico = ui_dropdown_criar();
    t->data_hora = ui_entry_criar(16, "DD/MM/AAAA HH:MM");
    t->obs = ui_entry_criar(150, "Opcional (também usada ao realizar/cancelar)");
    ui_campo(GTK_GRID(grade), 0, "Beneficiário *", t->dd_benef);
    ui_campo(GTK_GRID(grade), 1, "Médico *", t->dd_medico);
    ui_campo(GTK_GRID(grade), 2, "Data e hora *", t->data_hora);
    ui_campo(GTK_GRID(grade), 3, "Observação", t->obs);
    gtk_box_append(GTK_BOX(caixa), grade);

    GtkWidget *botoes = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *b_agendar = gtk_button_new_with_label("Agendar");
    GtkWidget *b_atualizar = gtk_button_new_with_label("Atualizar listas");
    gtk_widget_add_css_class(b_agendar, "suggested-action");
    gtk_box_append(GTK_BOX(botoes), b_agendar);
    gtk_box_append(GTK_BOX(botoes), b_atualizar);
    gtk_box_append(GTK_BOX(caixa), botoes);
    g_signal_connect(b_agendar, "clicked", G_CALLBACK(ao_agendar), t);
    g_signal_connect(b_atualizar, "clicked", G_CALLBACK(ao_atualizar), t);

    gtk_box_append(GTK_BOX(caixa), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* --- Histórico --- */
    GtkWidget *titulo2 = gtk_label_new("Histórico de consultas");
    gtk_label_set_xalign(GTK_LABEL(titulo2), 0.0f);
    gtk_widget_add_css_class(titulo2, "secao-titulo");
    gtk_box_append(GTK_BOX(caixa), titulo2);

    GtkWidget *filtros = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    t->dd_filtro_benef = ui_dropdown_criar();
    t->dd_filtro_status = ui_dropdown_criar();
    gtk_widget_set_hexpand(t->dd_filtro_benef, TRUE);
    ui_dropdown_preencher(t->dd_filtro_status, ROTULOS_STATUS, 4);
    gtk_box_append(GTK_BOX(filtros), t->dd_filtro_benef);
    gtk_box_append(GTK_BOX(filtros), t->dd_filtro_status);
    gtk_box_append(GTK_BOX(caixa), filtros);
    g_signal_connect(t->dd_filtro_benef, "notify::selected", G_CALLBACK(ao_filtro), t);
    g_signal_connect(t->dd_filtro_status, "notify::selected", G_CALLBACK(ao_filtro), t);

    gtk_box_append(GTK_BOX(caixa), ui_lista_criar(&t->lista));
    g_signal_connect(t->lista, "row-selected", G_CALLBACK(ao_selecionar), t);

    GtkWidget *acoes = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    t->lbl_sel = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(t->lbl_sel), 0.0f);
    gtk_widget_set_hexpand(t->lbl_sel, TRUE);
    t->btn_realizada = gtk_button_new_with_label("Marcar como realizada");
    t->btn_cancelada = gtk_button_new_with_label("Cancelar consulta");
    t->btn_excluir = gtk_button_new_with_label("Excluir consulta");
    gtk_widget_add_css_class(t->btn_cancelada, "destructive-action");
    gtk_widget_add_css_class(t->btn_excluir, "destructive-action");
    gtk_box_append(GTK_BOX(acoes), t->lbl_sel);
    gtk_box_append(GTK_BOX(acoes), t->btn_realizada);
    gtk_box_append(GTK_BOX(acoes), t->btn_cancelada);
    gtk_box_append(GTK_BOX(acoes), t->btn_excluir);
    gtk_box_append(GTK_BOX(caixa), acoes);
    g_signal_connect(t->btn_realizada, "clicked", G_CALLBACK(ao_realizada), t);
    g_signal_connect(t->btn_cancelada, "clicked", G_CALLBACK(ao_cancelada), t);
    g_signal_connect(t->btn_excluir, "clicked", G_CALLBACK(ao_excluir), t);

    gtk_window_set_child(GTK_WINDOW(janela), caixa);
    carregar_listas(t);
    recarregar_historico(t);
    gtk_window_present(GTK_WINDOW(janela));
}