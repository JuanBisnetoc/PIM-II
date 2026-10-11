#include "ui.h"

static void ao_beneficiarios(GtkButton *b, gpointer janela) { (void)b; ui_beneficiarios_abrir(GTK_WINDOW(janela)); }
static void ao_medicos(GtkButton *b, gpointer janela)       { (void)b; ui_medicos_abrir(GTK_WINDOW(janela)); }
static void ao_planos(GtkButton *b, gpointer janela)        { (void)b; ui_planos_abrir(GTK_WINDOW(janela)); }
static void ao_consultas(GtkButton *b, gpointer janela)     { (void)b; ui_form_consulta_abrir(GTK_WINDOW(janela)); }
static void ao_relatorios(GtkButton *b, gpointer janela)    { (void)b; ui_relatorios_abrir(GTK_WINDOW(janela)); }

/* Fechar a janela principal encerra o aplicativo (e, com ele, as filhas).
 * Fechar uma janela filha fecha somente ela. */
static gboolean ao_fechar_principal(GtkWindow *j, gpointer app) {
    (void)j;
    g_application_quit(G_APPLICATION(app));
    return FALSE;
}

static GtkWidget *criar_card(GtkWidget *janela, const char *icone, const char *titulo,
                             const char *sub, GCallback cb) {
    GtkWidget *botao = gtk_button_new();
    gtk_widget_add_css_class(botao, "card-menu");
    gtk_widget_set_hexpand(botao, TRUE);

    GtkWidget *linha = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
    GtkWidget *img = gtk_image_new_from_icon_name(icone);
    gtk_image_set_pixel_size(GTK_IMAGE(img), 32);

    GtkWidget *textos = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_valign(textos, GTK_ALIGN_CENTER);
    GtkWidget *t = gtk_label_new(titulo);
    GtkWidget *s = gtk_label_new(sub);
    gtk_label_set_xalign(GTK_LABEL(t), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(s), 0.0f);
    gtk_widget_add_css_class(t, "card-titulo");
    gtk_widget_add_css_class(s, "card-sub");
    gtk_box_append(GTK_BOX(textos), t);
    gtk_box_append(GTK_BOX(textos), s);

    gtk_box_append(GTK_BOX(linha), img);
    gtk_box_append(GTK_BOX(linha), textos);
    gtk_button_set_child(GTK_BUTTON(botao), linha);
    g_signal_connect(botao, "clicked", cb, janela);
    return botao;
}

GtkWidget *ui_main_window_criar(GtkApplication *app) {
    GtkWidget *janela = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(janela), "PIM II");
    gtk_window_set_default_size(GTK_WINDOW(janela), 620, 460);
    gtk_window_set_titlebar(GTK_WINDOW(janela), gtk_header_bar_new());
    g_signal_connect(janela, "close-request", G_CALLBACK(ao_fechar_principal), app);

    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    ui_margens(caixa, 24);

    GtkWidget *titulo = gtk_label_new("PIM II");
    GtkWidget *sub = gtk_label_new("Gestão de plano de saúde");
    gtk_label_set_xalign(GTK_LABEL(titulo), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0f);
    gtk_widget_add_css_class(titulo, "titulo-app");
    gtk_widget_add_css_class(sub, "subtitulo-app");
    gtk_box_append(GTK_BOX(caixa), titulo);
    gtk_box_append(GTK_BOX(caixa), sub);

    GtkWidget *grade = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grade), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grade), 12);
    gtk_grid_set_column_homogeneous(GTK_GRID(grade), TRUE);
    gtk_widget_set_vexpand(grade, TRUE);

    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "system-users-symbolic", "Beneficiários",
                               "Cadastro e consulta", G_CALLBACK(ao_beneficiarios)), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "contact-new-symbolic", "Médicos",
                               "CRM e especialidades", G_CALLBACK(ao_medicos)), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "view-list-symbolic", "Planos",
                               "Valores e situação", G_CALLBACK(ao_planos)), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "x-office-calendar-symbolic", "Consultas",
                               "Agendamento e histórico", G_CALLBACK(ao_consultas)), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "text-x-generic-symbolic", "Relatórios",
                               "Resumos e exportação", G_CALLBACK(ao_relatorios)), 0, 2, 2, 1);
    gtk_box_append(GTK_BOX(caixa), grade);

    gtk_window_set_child(GTK_WINDOW(janela), caixa);
    return janela;
}