#include <gtk/gtk.h>

#include "DataBase.h"
#include "ui.h"

static void ao_ativar(GtkApplication *app, gpointer dados) {
    (void)dados;
    ui_aplicar_estilo();
    GtkWidget *janela = ui_main_window_criar(app);
    gtk_window_present(GTK_WINDOW(janela));
}

int main(int argc, char **argv) {
    /* O arquivo do banco fica na pasta de trabalho do executável. */
    if (!db_open("pim_ii.db")) return 1;
    if (!db_criar_tabelas() || !db_inserir_dados_iniciais()) {
        db_close();
        return 1;
    }

    GtkApplication *app = gtk_application_new("br.pim.ii", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(ao_ativar), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    db_close();
    return status;
}