# PIM II — Gestão de Plano de Saúde

Sistema desktop para gerenciar um plano de saúde: beneficiários, médicos, planos, consultas e relatórios. Desenvolvido em **C**, com interface **GTK4** e banco de dados **SQLite**, organizado em 5 sprints.

## Funcionalidades

| Módulo | O que faz |
|---|---|
| **Beneficiários** | Cadastro, busca por nome/CPF, edição, inativação/reativação, vínculo com um plano |
| **Médicos** | Cadastro com CRM e especialidade, busca, edição, inativação/reativação |
| **Planos** | Cadastro com valor mensal, edição, inativação (bloqueada se houver beneficiários ativos) |
| **Consultas** | Agendamento, histórico com filtros (beneficiário e status), marcar como realizada ou cancelada |
| **Relatórios** | Beneficiários por plano, consultas por médico, consultas por status e resumo geral, com opção de salvar em `.txt` |

### Regras de negócio

- Não é possível agendar consulta no passado, nem para beneficiário ou médico inativo.
- Médico e beneficiário não podem ter duas consultas (não canceladas) no mesmo horário.
- Beneficiário ou médico com consultas agendadas não pode ser inativado.
- Plano com beneficiários ativos não pode ser inativado.
- Somente consultas agendadas podem ser marcadas como realizadas ou canceladas.
- CRM e nome de plano são únicos; CPF é único entre beneficiários.

### CPF e dados fictícios

A validação de CPF é **flexível**: aceita qualquer sequência de 11 dígitos (inclusive `111.111.111-11`), para facilitar testes e demonstrações. A tela de Beneficiários tem o botão **Preencher dados fictícios**, que gera nome, CPF, nascimento, telefone e plano aleatórios.

Para voltar à validação oficial (dígitos verificadores), troque o corpo de `srv_cpf_valido` em `Services/services.c` pelo de `srv_cpf_valido_oficial`.

## Tecnologias

- Linguagem C
- GTK4 (interface gráfica)
- SQLite3 (banco de dados, arquivo `pim_ii.db`)
- CMake (build), via `pkg-config`

## Estrutura do projeto

```
SRC/
├── main.c                  # Ponto de entrada: abre o banco e inicia o GtkApplication
├── DataBase/               # Conexão, criação das tabelas e dados iniciais
├── Models/                 # Structs (Plano, Beneficiario, Medico, Consulta) e helpers
├── Services/               # Regras de negócio, validações, consultas SQL e relatórios
└── Ui/
    ├── ui.h                # Declarações das telas e funções comuns
    ├── ui_util.c           # Estilo (CSS), componentes comuns e ciclo de vida das janelas
    ├── ui_main_window.c    # Janela principal (menu em cards)
    ├── ui_beneficiarios.c
    ├── ui_medicos.c
    ├── ui_planos.c
    ├── ui_form_consulta.c
    └── ui_relatorios.c
```

## Banco de dados

O arquivo `pim_ii.db` é criado na pasta de trabalho do executável, na primeira execução.

Tabelas: `planos`, `beneficiarios`, `medicos` e `consultas`, com chaves estrangeiras ativadas. Três planos iniciais são inseridos automaticamente (Básico, Intermediário e Premium).

## Como compilar

### Dependências

- Compilador C (GCC)
- CMake
- GTK4 e SQLite3 com `pkg-config`

No Windows, o projeto usa o **MSYS2 (ambiente UCRT64)**:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-gtk4 \
          mingw-w64-ucrt-x86_64-sqlite3 \
          mingw-w64-ucrt-x86_64-pkg-config
```

No Linux (Debian/Ubuntu):

```bash
sudo apt install build-essential cmake pkg-config libgtk-4-dev libsqlite3-dev
```

### Build

```bash
cmake -S . -B build
cmake --build build
./build/PIM_II
```

Execute o programa a partir de uma pasta onde ele possa criar o arquivo `pim_ii.db` e os relatórios `.txt`.

## Interface

- Janela principal com menu em cards e barra de título moderna.
- Estilo global em CSS (cantos arredondados, listas com seleção suave).
- Fechar uma janela filha fecha somente ela; fechar a janela principal encerra o aplicativo.

## Sprints

1. Banco de dados e estrutura do projeto
2. Beneficiários (cadastro, consulta, alteração, inativação e validações)
3. Médicos e planos
4. Consultas (agendamento, histórico e alteração de status)
5. Relatórios e testes