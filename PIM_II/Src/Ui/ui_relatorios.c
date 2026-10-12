/* Sprint 5: relatórios em texto, com opção de salvar em arquivo. */
#include "ui.h"

#include <stdlib.h>
#include "services.h"

typedef struct {
    GtkWindow *janela;
    GtkWidget *dd_tipo;
    GtkTextBuffer *buffer;
    char *ultimo_texto;   /* texto exibido, usado ao salvar */
    int   ultimo_tipo;
} TelaRelatorios;

static TelaRelatorios *g_tela = NULL;

static void ao_gerar(GtkButton *b, gpointer dados) {
    (void)b;
    TelaRelatorios *t = dados;
    int tipo = ui_dropdown_indice(t->dd_tipo);
    if (tipo < 0 || tipo >= REL_TOTAL) return;

    free(t->ultimo_texto);
    t->ultimo_texto = srv_relatorio_gerar(tipo);
    t->ultimo_tipo = tipo;
    gtk_text_buffer_set_text(t->buffer, t->ultimo_texto ? t->ultimo_texto : "", -1);
}

static void ao_salvar(GtkButton *b, gpointer dados) {
    (void)b;
    TelaRelatorios *t = dados;
    if (!t->ultimo_texto) {
        ui_mensagem(t->janela, "Gere um relatório antes de salvar.");
        return;
    }
    char nome[64];
    g_snprintf(nome, sizeof nome, "relatorio_%d.txt", t->ultimo_tipo + 1);

    if (srv_salvar_texto(nome, t->ultimo_texto)) {
        char *pasta = g_get_current_dir();
        char *caminho = g_build_filename(pasta, nome, NULL);
        char *msg = g_strdup_printf("Relatório salvo em:\n%s", caminho);
        ui_mensagem(t->janela, msg);
        g_free(msg);
        g_free(caminho);
        g_free(pasta);
    } else {
        ui_mensagem(t->janela, "Não foi possível salvar o arquivo.");
    }
}

static void ao_destruir(GtkWidget *w, gpointer dados) {
    ui_desconectar(w, dados);   /* nenhum callback roda após o free */
    TelaRelatorios *t = dados;
    free(t->ultimo_texto);
    g_free(t);
    g_tela = NULL;
}

void ui_relatorios_abrir(GtkWindow *pai) {
    if (g_tela) {
        gtk_window_present(g_tela->janela);
        return;
    }
    TelaRelatorios *t = g_new0(TelaRelatorios, 1);
    g_tela = t;

    GtkWidget *janela = ui_janela_criar(pai, "Relatórios", 820, 580);
    t->janela = GTK_WINDOW(janela);
    g_signal_connect(janela, "destroy", G_CALLBACK(ao_destruir), t);

    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    ui_margens(caixa, 18);

    GtkWidget *titulo = gtk_label_new("Relatórios");
    gtk_label_set_xalign(GTK_LABEL(titulo), 0.0f);
    gtk_widget_add_css_class(titulo, "secao-titulo");
    gtk_box_append(GTK_BOX(caixa), titulo);

    GtkWidget *barra = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    const char *titulos[REL_TOTAL];
    for (int i = 0; i < REL_TOTAL; i++) titulos[i] = srv_relatorio_titulo(i);
    t->dd_tipo = ui_dropdown_criar();
    ui_dropdown_preencher(t->dd_tipo, titulos, REL_TOTAL);
    gtk_widget_set_hexpand(t->dd_tipo, TRUE);

    GtkWidget *b_gerar = gtk_button_new_with_label("Gerar");
    GtkWidget *b_salvar = gtk_button_new_with_label("Salvar em .txt");
    gtk_widget_add_css_class(b_gerar, "suggested-action");
    gtk_box_append(GTK_BOX(barra), t->dd_tipo);
    gtk_box_append(GTK_BOX(barra), b_gerar);
    gtk_box_append(GTK_BOX(barra), b_salvar);
    gtk_box_append(GTK_BOX(caixa), barra);
    g_signal_connect(b_gerar, "clicked", G_CALLBACK(ao_gerar), t);
    g_signal_connect(b_salvar, "clicked", G_CALLBACK(ao_salvar), t);

    GtkWidget *texto = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(texto), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(texto), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(texto), TRUE);
    ui_margens(texto, 10);
    t->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(texto));

    GtkWidget *rolagem = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rolagem), texto);
    gtk_widget_set_vexpand(rolagem, TRUE);
    gtk_widget_add_css_class(rolagem, "frame");
    gtk_box_append(GTK_BOX(caixa), rolagem);

    gtk_window_set_child(GTK_WINDOW(janela), caixa);
    ao_gerar(NULL, t);   /* mostra o primeiro relatório ao abrir */
    gtk_window_present(GTK_WINDOW(janela));
}