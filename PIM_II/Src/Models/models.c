#include "models.h"

#include <ctype.h>
#include <string.h>

void models_copiar(char *dest, size_t tam, const char *origem) {
    if (!dest || tam == 0) return;
    if (!origem) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, origem, tam - 1);
    dest[tam - 1] = '\0';
}

void models_trim(char *s) {
    if (!s) return;
    size_t ini = 0;
    while (s[ini] && isspace((unsigned char)s[ini])) ini++;
    size_t fim = strlen(s);
    while (fim > ini && isspace((unsigned char)s[fim - 1])) fim--;
    memmove(s, s + ini, fim - ini);
    s[fim - ini] = '\0';
}
