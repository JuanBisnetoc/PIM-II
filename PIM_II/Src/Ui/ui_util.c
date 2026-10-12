#include "ui.h"

#include <string.h>

/* ========================================================================== */
/*  Estilo global                                                             */
/*  Paleta extraída da logo NexaSaúde: azul, turquesa e verde.                */
/* ========================================================================== */

void ui_aplicar_estilo(void) {
    /* O visual da marca é claro: ignora o tema escuro do sistema. */
    GtkSettings *cfg = gtk_settings_get_default();
    if (cfg) g_object_set(cfg, "gtk-application-prefer-dark-theme", FALSE, NULL);

    static const char *css =
        /* --- paleta --- */
        "@define-color marca_azul #0b63c5;"
        "@define-color marca_turquesa #12a3a0;"
        "@define-color marca_verde #6cbf4a;"
        "@define-color marca_escuro #0b4f5e;"
        "@define-color fundo_app #f3f8fb;"
        "@define-color borda_app #dce7ee;"
        "@define-color texto_app #1f3a47;"

        /* --- base --- */
        "window { background-color: @fundo_app; color: @texto_app; font-size: 11pt; }"
        "headerbar.barra-app { background: white; box-shadow: none; border: none;"
        "  border-bottom: 3px solid @marca_turquesa; min-height: 44px; }"
        "headerbar.barra-app .title { font-weight: 600; color: @marca_escuro; }"

        /* --- cabeçalho da tela principal --- */
        ".titulo-app { font-size: 30px; font-weight: 800; color: @marca_azul; }"
        ".subtitulo-app { color: #5b7280; font-size: 105%; }"
        ".rotulo-secao { font-size: 80%; font-weight: 700; letter-spacing: 2px;"
        "  color: #6b8290; margin-top: 6px; }"

        /* --- botões --- */
        "button { background-image: none; background-color: white;"
        "  border: 1px solid @borda_app; border-radius: 12px; min-height: 38px;"
        "  padding: 0 16px; color: @texto_app; box-shadow: none;"
        "  transition: all 150ms ease; }"
        "button:hover { border-color: @marca_turquesa; background-color: #f0fafa; }"
        "button:disabled { opacity: 0.5; }"
        "button.suggested-action { background-image: linear-gradient(to right,"
        "  @marca_azul, @marca_turquesa); color: white; border: none; font-weight: 600;"
        "  box-shadow: 0 2px 6px alpha(@marca_azul, 0.35); }"
        "button.suggested-action:hover { box-shadow: 0 4px 12px alpha(@marca_azul, 0.45); }"
        "button.destructive-action { background-color: #fdecec; color: #b3261e;"
        "  border-color: #f5c6c3; }"
        "button.destructive-action:hover { background-color: #fadada;"
        "  border-color: #e9a29d; }"

        /* --- cards do menu principal --- */
        "button.card-menu { padding: 18px 20px; border-radius: 18px;"
        "  background-color: white; border: 1px solid @borda_app;"
        "  box-shadow: 0 1px 2px alpha(@marca_escuro, 0.06); }"
        "button.card-menu:hover { background-color: white;"
        "  border-color: @marca_turquesa;"
        "  box-shadow: 0 8px 20px alpha(@marca_azul, 0.16); }"
        "button.card-menu:active { background-color: #f0fafa; }"
        ".card-titulo { font-weight: 700; font-size: 112%; color: @texto_app; }"
        ".card-sub { color: #5b7280; font-size: 90%; }"
        ".card-seta { color: #9db0bb; transition: all 150ms ease; }"
        "button.card-menu:hover .card-seta { color: @marca_turquesa; }"
        ".card-selo { min-width: 52px; min-height: 52px; border-radius: 16px; }"
        ".card-selo.cor-azul { background: #dbeafa; color: @marca_azul; }"
        ".card-selo.cor-turquesa { background: #d4f0ef; color: #0e8a87; }"
        ".card-selo.cor-verde { background: #e3f4d9; color: #3f8a24; }"

        /* --- janelas filhas --- */
        ".secao-titulo { font-weight: 700; font-size: 115%; padding-left: 10px;"
        "  border-left: 4px solid @marca_turquesa; color: @marca_escuro; }"
        "entry, dropdown > button { background-color: white; background-image: none;"
        "  color: @texto_app; border: 1px solid @borda_app; border-radius: 12px;"
        "  min-height: 38px; box-shadow: none; }"
        "entry:focus-within { border-color: @marca_turquesa;"
        "  box-shadow: 0 0 0 3px alpha(@marca_turquesa, 0.22); }"
        "scrolledwindow.frame { background-color: white; border: 1px solid @borda_app;"
        "  border-radius: 14px; }"
        "list { background: transparent; }"
        "list row { border-radius: 10px; margin: 3px 6px; transition: all 120ms ease; }"
        "list row:hover { background: alpha(@marca_turquesa, 0.08); }"
        "list row:selected { background: alpha(@marca_turquesa, 0.16);"
        "  color: @marca_escuro; box-shadow: inset 3px 0 0 @marca_turquesa; }"
        "textview, textview text { background-color: white; color: @texto_app; }"
        "separator { background-color: @borda_app; min-height: 1px; }"
        "checkbutton check { border: 1px solid #b6c8d3; border-radius: 5px;"
        "  background-color: white; background-image: none; }"
        "checkbutton:checked check { background-color: @marca_turquesa;"
        "  border-color: @marca_turquesa; color: white; }";

    GtkCssProvider *p = gtk_css_provider_new();
    gtk_css_provider_load_from_data(p, css, -1);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(p),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(p);
}

/* ========================================================================== */
/*  Ciclo de vida das janelas                                                 */
/* ========================================================================== */

/* Desconecta todos os handlers que usam 'dados' em w e em toda a sua árvore de
 * filhos. Chamado no início de ao_destruir, antes do g_free(dados): evita que
 * sinais emitidos durante o descarte (changed, row-selected, notify::selected)
 * acessem memória já liberada. */
void ui_desconectar(GtkWidget *w, gpointer dados) {
    g_signal_handlers_disconnect_by_data(w, dados);
    for (GtkWidget *c = gtk_widget_get_first_child(w); c; c = gtk_widget_get_next_sibling(c))
        ui_desconectar(c, dados);
}

/* ========================================================================== */
/*  Componentes                                                               */
/* ========================================================================== */

void ui_mensagem(GtkWindow *pai, const char *texto) {
    GtkAlertDialog *dlg = gtk_alert_dialog_new("%s", texto);
    gtk_alert_dialog_show(dlg, pai);
    g_object_unref(dlg);
}

void ui_margens(GtkWidget *w, int px) {
    gtk_widget_set_margin_top(w, px);
    gtk_widget_set_margin_bottom(w, px);
    gtk_widget_set_margin_start(w, px);
    gtk_widget_set_margin_end(w, px);
}

GtkWidget *ui_janela_criar(GtkWindow *pai, const char *titulo, int largura, int altura) {
    GtkWidget *janela = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(janela), titulo);
    gtk_window_set_default_size(GTK_WINDOW(janela), largura, altura);

    /* Barra de título clara com o filete turquesa da marca (igual à principal). */
    GtkWidget *barra = gtk_header_bar_new();
    gtk_widget_add_css_class(barra, "barra-app");
    gtk_window_set_titlebar(GTK_WINDOW(janela), barra);

    if (pai) {
        gtk_window_set_transient_for(GTK_WINDOW(janela), pai);
        GtkApplication *app = gtk_window_get_application(pai);
        if (app) gtk_window_set_application(GTK_WINDOW(janela), app);
    }
    return janela;
}

GtkWidget *ui_lista_criar(GtkListBox **lista_out) {
    GtkWidget *lista = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(lista), GTK_SELECTION_SINGLE);

    GtkWidget *rolagem = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rolagem), lista);
    gtk_widget_set_vexpand(rolagem, TRUE);
    gtk_widget_set_hexpand(rolagem, TRUE);
    gtk_widget_add_css_class(rolagem, "frame");

    *lista_out = GTK_LIST_BOX(lista);
    return rolagem;
}

void ui_lista_limpar(GtkListBox *lista) {
    GtkListBoxRow *linha;
    while ((linha = gtk_list_box_get_row_at_index(lista, 0)) != NULL)
        gtk_list_box_remove(lista, GTK_WIDGET(linha));
}

void ui_lista_adicionar(GtkListBox *lista, const char *texto) {
    GtkWidget *rotulo = gtk_label_new(texto);
    gtk_label_set_xalign(GTK_LABEL(rotulo), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(rotulo), TRUE);
    ui_margens(rotulo, 8);
    gtk_list_box_append(lista, rotulo);
}

void ui_campo(GtkGrid *grid, int linha, const char *rotulo, GtkWidget *campo) {
    GtkWidget *l = gtk_label_new(rotulo);
    gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
    gtk_grid_attach(grid, l, 0, linha, 1, 1);
    gtk_widget_set_hexpand(campo, TRUE);
    gtk_grid_attach(grid, campo, 1, linha, 1, 1);
}

GtkWidget *ui_entry_criar(int max_caracteres, const char *dica) {
    GtkWidget *e = gtk_entry_new();
    if (max_caracteres > 0) gtk_entry_set_max_length(GTK_ENTRY(e), max_caracteres);
    if (dica) gtk_entry_set_placeholder_text(GTK_ENTRY(e), dica);
    return e;
}

const char *ui_texto(GtkWidget *entry) {
    return gtk_editable_get_text(GTK_EDITABLE(entry));
}

void ui_definir_texto(GtkWidget *entry, const char *texto) {
    gtk_editable_set_text(GTK_EDITABLE(entry), texto ? texto : "");
}

GtkWidget *ui_dropdown_criar(void) {
    static const char *vazio[] = {NULL};
    return gtk_drop_down_new_from_strings(vazio);
}

void ui_dropdown_preencher(GtkWidget *dd, const char *const *textos, int n) {
    GtkStringList *modelo = gtk_string_list_new(NULL);
    for (int i = 0; i < n; i++) gtk_string_list_append(modelo, textos[i]);
    gtk_drop_down_set_model(GTK_DROP_DOWN(dd), G_LIST_MODEL(modelo));
    g_object_unref(modelo);
}

int ui_dropdown_indice(GtkWidget *dd) {
    guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(dd));
    return sel == GTK_INVALID_LIST_POSITION ? -1 : (int)sel;
}

int ui_parse_valor(const char *texto, double *valor) {
    char tmp[64];
    g_strlcpy(tmp, texto ? texto : "", sizeof tmp);
    g_strstrip(tmp);
    if (!tmp[0]) return 0;
    for (char *p = tmp; *p; p++)
        if (*p == ',') *p = '.';
    char *fim = NULL;
    double v = g_ascii_strtod(tmp, &fim);
    if (fim == tmp || *fim != '\0') return 0;
    *valor = v;
    return 1;
}