#include "ui.h"

static void ao_beneficiarios(GtkButton *b, gpointer janela) { (void)b; ui_beneficiarios_abrir(GTK_WINDOW(janela)); }
static void ao_medicos(GtkButton *b, gpointer janela)       { (void)b; ui_medicos_abrir(GTK_WINDOW(janela)); }
static void ao_planos(GtkButton *b, gpointer janela)        { (void)b; ui_planos_abrir(GTK_WINDOW(janela)); }
static void ao_consultas(GtkButton *b, gpointer janela)     { (void)b; ui_form_consulta_abrir(GTK_WINDOW(janela)); }
static void ao_relatorios(GtkButton *b, gpointer janela)    { (void)b; ui_relatorios_abrir(GTK_WINDOW(janela)); }

static void adicionar_botao(GtkWidget *caixa, GtkWidget *janela, const char *rotulo,
                            GCallback cb) {
    GtkWidget *botao = gtk_button_new_with_label(rotulo);
    g_signal_connect(botao, "clicked", cb, janela);
    gtk_box_append(GTK_BOX(caixa), botao);
}

GtkWidget *ui_main_window_criar(GtkApplication *app) {
    GtkWidget *janela = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(janela), "PIM II");
    gtk_window_set_default_size(GTK_WINDOW(janela), 380, 380);

    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    ui_margens(caixa, 20);

    GtkWidget *titulo = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(titulo),
                         "<span size='x-large' weight='bold'>PIM II</span>\n"
                         "<span>Gestão de plano de saúde</span>");
    gtk_label_set_justify(GTK_LABEL(titulo), GTK_JUSTIFY_CENTER);
    gtk_widget_set_margin_bottom(titulo, 10);
    gtk_box_append(GTK_BOX(caixa), titulo);

    adicionar_botao(caixa, janela, "Beneficiários", G_CALLBACK(ao_beneficiarios));
    adicionar_botao(caixa, janela, "Médicos", G_CALLBACK(ao_medicos));
    adicionar_botao(caixa, janela, "Planos", G_CALLBACK(ao_planos));
    adicionar_botao(caixa, janela, "Consultas", G_CALLBACK(ao_consultas));
    adicionar_botao(caixa, janela, "Relatórios", G_CALLBACK(ao_relatorios));

    gtk_window_set_child(GTK_WINDOW(janela), caixa);
    return janela;
}
