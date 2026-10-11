#include "ui.h"

#include <string.h>

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
    ui_margens(rotulo, 6);
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
