#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

/* Abre (ou cria) o arquivo do banco. Use ":memory:" para testes. */
int      db_open(const char *caminho);
void     db_close(void);
sqlite3 *db_get(void);

/* Cria as tabelas se ainda não existirem. */
int      db_criar_tabelas(void);
/* Insere planos padrão na primeira execução (não duplica). */
int      db_inserir_dados_iniciais(void);

#endif
