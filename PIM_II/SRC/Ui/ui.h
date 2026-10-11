#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

/* ----------------------------- Telas do sistema ---------------------------- */
GtkWidget *ui_main_window_criar(GtkApplication *app);
void ui_beneficiarios_abrir(GtkWindow *pai);   /* Sprint 2 */
void ui_medicos_abrir(GtkWindow *pai);         /* Sprint 3 */
void ui_planos_abrir(GtkWindow *pai);          /* Sprint 3 */
void ui_form_consulta_abrir(GtkWindow *pai);   /* Sprint 4 */
void ui_relatorios_abrir(GtkWindow *pai);      /* Sprint 5 */

/* ------------------------------ Funções comuns ----------------------------- */
void       ui_mensagem(GtkWindow *pai, const char *texto);
void       ui_margens(GtkWidget *w, int px);
GtkWidget *ui_janela_criar(GtkWindow *pai, const char *titulo, int largura, int altura);

/* Lista (GtkListBox dentro de um GtkScrolledWindow). */
GtkWidget *ui_lista_criar(GtkListBox **lista_out);
void       ui_lista_limpar(GtkListBox *lista);
void       ui_lista_adicionar(GtkListBox *lista, const char *texto);

/* Formulário em grade: rótulo na coluna 0 e campo na coluna 1. */
void ui_campo(GtkGrid *grid, int linha, const char *rotulo, GtkWidget *campo);
GtkWidget *ui_entry_criar(int max_caracteres, const char *dica);
const char *ui_texto(GtkWidget *entry);
void        ui_definir_texto(GtkWidget *entry, const char *texto);

/* GtkDropDown preenchido dinamicamente. */
GtkWidget *ui_dropdown_criar(void);
void       ui_dropdown_preencher(GtkWidget *dd, const char *const *textos, int n);
int        ui_dropdown_indice(GtkWidget *dd);   /* -1 se nada selecionado */

/* Converte "199,90" ou "199.90" (independe do idioma do sistema). */
int ui_parse_valor(const char *texto, double *valor);

#endif
