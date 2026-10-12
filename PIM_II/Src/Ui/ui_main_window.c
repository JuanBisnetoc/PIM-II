#include "ui.h"

/* Caminhos testados (em ordem) para encontrar a logo. Se nenhum existir,
 * a janela abre normalmente, com o título em texto no lugar. */
static const char *CAMINHOS_LOGO[] = {
#ifdef ASSETS_DIR                       /* definido no CMakeLists.txt (caminho absoluto) */
    ASSETS_DIR "/Assets.jpeg",
#endif
    "Assets/Assets.jpeg", "../Assets/Assets.jpeg",
    "assets/logo.png",    "../assets/logo.png",
    NULL
};

#define LOGO_LARGURA 100
#define LOGO_ALTURA   28

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

/* Procura a logo na pasta atual e ao lado do executável (e na pasta acima dele),
 * para funcionar não importa de onde o programa for iniciado.
 * Devolve um caminho alocado (g_free) ou NULL. */
static char *localizar_logo(void) {
    char *exe = g_file_read_link("/proc/self/exe", NULL);
    char *pasta_exe = exe ? g_path_get_dirname(exe) : NULL;
    char *achado = NULL;

    for (int i = 0; CAMINHOS_LOGO[i] != NULL && !achado; i++) {
        char *c = g_strdup(CAMINHOS_LOGO[i]);
        if (g_file_test(c, G_FILE_TEST_IS_REGULAR)) achado = c;
        else g_free(c);

        if (!achado && pasta_exe) {
            c = g_build_filename(pasta_exe, CAMINHOS_LOGO[i], NULL);
            if (g_file_test(c, G_FILE_TEST_IS_REGULAR)) achado = c;
            else g_free(c);
        }
    }
    g_free(pasta_exe);
    g_free(exe);
    if (!achado) g_printerr("Aviso: logo não encontrada (esperado em assets/logo.png).\n");
    return achado;
}

/* Carrega a imagem, torna transparente o fundo (branco/cinza do quadriculado) e
 * recorta as margens vazias. Pixels coloridos da logo ficam intactos; só as
 * bordas suavizadas ganham transparência parcial. Devolve NULL em caso de erro. */
static GdkTexture *logo_sem_fundo(const char *caminho) {
    GError *erro = NULL;
    GdkPixbuf *orig = gdk_pixbuf_new_from_file(caminho, &erro);
    if (!orig) {
        g_printerr("Aviso: não foi possível abrir a logo: %s\n", erro ? erro->message : "?");
        g_clear_error(&erro);
        return NULL;
    }
    GdkPixbuf *pb = gdk_pixbuf_add_alpha(orig, FALSE, 0, 0, 0);
    g_object_unref(orig);
    if (!pb) return NULL;

    int w = gdk_pixbuf_get_width(pb), h = gdk_pixbuf_get_height(pb);
    int rs = gdk_pixbuf_get_rowstride(pb);
    guchar *px = gdk_pixbuf_get_pixels(pb);
    int x0 = w, y0 = h, x1 = -1, y1 = -1;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            guchar *p = px + (size_t)y * rs + (size_t)x * 4;
            int mx = p[0] > p[1] ? p[0] : p[1];
            if (p[2] > mx) mx = p[2];
            int mn = p[0] < p[1] ? p[0] : p[1];
            if (p[2] < mn) mn = p[2];

            int alfa = 255;
            if (mn > 150) {                       /* pixel claro */
                if (mx - mn < 28) alfa = 0;       /* neutro (branco/cinza): fundo */
                else alfa = (255 - mn) * 255 / 105; /* borda colorida suavizada */
                if (alfa > 255) alfa = 255;
            }
            p[3] = (guchar)alfa;
            if (alfa > 40) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
        }
    }

    GdkPixbuf *final = pb;
    if (x1 >= x0 && y1 >= y0) {
        x0 = x0 > 2 ? x0 - 2 : 0;
        y0 = y0 > 2 ? y0 - 2 : 0;
        x1 = x1 + 2 < w ? x1 + 2 : w - 1;
        y1 = y1 + 2 < h ? y1 + 2 : h - 1;
        final = gdk_pixbuf_new_subpixbuf(pb, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    }
    GdkTexture *tex = gdk_texture_new_for_pixbuf(final);
    if (final != pb) g_object_unref(final);
    g_object_unref(pb);
    return tex;
}

/* Cria o widget da logo, ou NULL se o arquivo não for encontrado. */
static GtkWidget *criar_logo(void) {
    char *caminho = localizar_logo();
    if (caminho) {
        GdkTexture *tex = logo_sem_fundo(caminho);
        GtkWidget *logo = tex ? gtk_picture_new_for_paintable(GDK_PAINTABLE(tex))
                              : gtk_picture_new_for_filename(caminho);
        if (tex) g_object_unref(tex);
        g_free(caminho);
        gtk_picture_set_content_fit(GTK_PICTURE(logo), GTK_CONTENT_FIT_CONTAIN);
        gtk_picture_set_can_shrink(GTK_PICTURE(logo), TRUE);
        gtk_widget_set_size_request(logo, LOGO_LARGURA, LOGO_ALTURA);
        gtk_widget_set_halign(logo, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(logo, GTK_ALIGN_CENTER);
        gtk_widget_add_css_class(logo, "logo-app");
        return logo;
    }
    return NULL;
}

/* Card do menu: ícone em "selo" colorido + título/subtítulo + seta. */
static GtkWidget *criar_card(GtkWidget *janela, const char *icone, const char *titulo,
                             const char *sub, const char *classe_cor, GCallback cb) {
    GtkWidget *botao = gtk_button_new();
    gtk_widget_add_css_class(botao, "card-menu");
    gtk_widget_set_hexpand(botao, TRUE);
    gtk_widget_set_cursor_from_name(botao, "pointer");

    GtkWidget *linha = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);

    GtkWidget *selo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(selo, "card-selo");
    gtk_widget_add_css_class(selo, classe_cor);
    gtk_widget_set_valign(selo, GTK_ALIGN_CENTER);
    GtkWidget *img = gtk_image_new_from_icon_name(icone);
    gtk_image_set_pixel_size(GTK_IMAGE(img), 24);
    gtk_widget_set_hexpand(img, TRUE);
    gtk_widget_set_halign(img, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(selo), img);

    GtkWidget *textos = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_valign(textos, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(textos, TRUE);
    GtkWidget *t = gtk_label_new(titulo);
    GtkWidget *s = gtk_label_new(sub);
    gtk_label_set_xalign(GTK_LABEL(t), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(s), 0.0f);
    gtk_widget_add_css_class(t, "card-titulo");
    gtk_widget_add_css_class(s, "card-sub");
    gtk_box_append(GTK_BOX(textos), t);
    gtk_box_append(GTK_BOX(textos), s);

    GtkWidget *seta = gtk_image_new_from_icon_name("go-next-symbolic");
    gtk_widget_add_css_class(seta, "card-seta");
    gtk_widget_set_valign(seta, GTK_ALIGN_CENTER);

    gtk_box_append(GTK_BOX(linha), selo);
    gtk_box_append(GTK_BOX(linha), textos);
    gtk_box_append(GTK_BOX(linha), seta);
    gtk_button_set_child(GTK_BUTTON(botao), linha);
    g_signal_connect(botao, "clicked", cb, janela);
    return botao;
}

GtkWidget *ui_main_window_criar(GtkApplication *app) {
    GtkWidget *janela = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(janela), "PIM II");
    gtk_window_set_default_size(GTK_WINDOW(janela), 820, 560);
    gtk_window_maximize(GTK_WINDOW(janela));   /* abre ocupando a tela toda */

    /* Barra de título limpa, sem texto (a identidade fica na logo). */
    GtkWidget *barra = gtk_header_bar_new();
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(barra), gtk_label_new(""));
    gtk_widget_add_css_class(barra, "barra-app");
    gtk_window_set_titlebar(GTK_WINDOW(janela), barra);
    g_signal_connect(janela, "close-request", G_CALLBACK(ao_fechar_principal), app);

    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    ui_margens(caixa, 22);
    gtk_widget_set_margin_top(caixa, 4);

    /* --- Cabeçalho: logo + subtítulo --- */
    GtkWidget *cabecalho = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *logo = criar_logo();
    if (logo) {
        gtk_box_append(GTK_BOX(cabecalho), logo);
    } else {
        GtkWidget *titulo = gtk_label_new("PIM II");
        gtk_label_set_xalign(GTK_LABEL(titulo), 0.5f);
        gtk_widget_add_css_class(titulo, "titulo-app");
        gtk_box_append(GTK_BOX(cabecalho), titulo);
    }
    GtkWidget *sub = gtk_label_new("Gestão de plano de saúde");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.5f);
    gtk_widget_add_css_class(sub, "subtitulo-app");
    gtk_box_append(GTK_BOX(cabecalho), sub);
    gtk_box_append(GTK_BOX(caixa), cabecalho);

    /* --- Rótulo da seção --- */
    GtkWidget *secao = gtk_label_new("MÓDULOS");
    gtk_label_set_xalign(GTK_LABEL(secao), 0.0f);
    gtk_widget_add_css_class(secao, "rotulo-secao");
    gtk_box_append(GTK_BOX(caixa), secao);

    /* --- Grade de cards --- */
    GtkWidget *grade = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grade), 14);
    gtk_grid_set_column_spacing(GTK_GRID(grade), 14);
    gtk_grid_set_column_homogeneous(GTK_GRID(grade), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(grade), TRUE);
    gtk_widget_set_vexpand(grade, FALSE);
    gtk_widget_set_valign(grade, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "system-users-symbolic", "Beneficiários",
                               "Cadastro e consulta", "cor-azul", G_CALLBACK(ao_beneficiarios)),
                    0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "contact-new-symbolic", "Médicos",
                               "CRM e especialidades", "cor-turquesa", G_CALLBACK(ao_medicos)),
                    1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "view-list-symbolic", "Planos",
                               "Valores e situação", "cor-verde", G_CALLBACK(ao_planos)),
                    0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "x-office-calendar-symbolic", "Consultas",
                               "Agendamento e histórico", "cor-azul", G_CALLBACK(ao_consultas)),
                    1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grade),
                    criar_card(janela, "text-x-generic-symbolic", "Relatórios",
                               "Resumos e exportação", "cor-turquesa", G_CALLBACK(ao_relatorios)),
                    0, 2, 2, 1);
    gtk_box_append(GTK_BOX(caixa), grade);

    /* Em tela cheia o conteúdo não estica: fica numa coluna central de largura
     * fixa, colada no topo. */
    gtk_widget_set_size_request(caixa, 760, -1);
    gtk_widget_set_halign(caixa, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(caixa, GTK_ALIGN_START);

    gtk_window_set_child(GTK_WINDOW(janela), caixa);
    return janela;
}