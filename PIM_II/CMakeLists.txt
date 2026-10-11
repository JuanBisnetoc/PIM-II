cmake_minimum_required(VERSION 4.3)
project(PIM_II C)

set(CMAKE_C_STANDARD 11)

# pkg-config instalado pelo MSYS2 no ambiente UCRT64
if(WIN32)
    set(PKG_CONFIG_EXECUTABLE "C:/msys64/ucrt64/bin/pkg-config.exe")
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

set(INCLUDES SRC SRC/DataBase SRC/Models SRC/Services SRC/Ui)

# Código sem interface (banco + regras de negócio): reaproveitado nos testes
add_library(pim_core STATIC
        SRC/DataBase/DataBase.c
        SRC/Models/models.c
        SRC/Services/services.c
)
target_include_directories(pim_core PUBLIC ${INCLUDES})
target_link_libraries(pim_core PUBLIC PkgConfig::SQLITE3)
target_compile_options(pim_core PRIVATE -Wall -Wextra)

# Aplicativo
add_executable(PIM_II
        SRC/main.c
        SRC/Ui/ui_util.c
        SRC/Ui/ui_main_window.c
        SRC/Ui/ui_beneficiarios.c
        SRC/Ui/ui_medicos.c
        SRC/Ui/ui_planos.c
        SRC/Ui/ui_form_consulta.c
        SRC/Ui/ui_relatorios.c
)
target_include_directories(PIM_II PRIVATE ${INCLUDES})
target_link_libraries(PIM_II PRIVATE pim_core PkgConfig::GTK4)
target_compile_options(PIM_II PRIVATE -Wall -Wextra)

# Testes dos services (Sprint 5): rode com `ctest` ou execute test_services
enable_testing()
add_executable(test_services tests/test_services.c)
target_link_libraries(test_services PRIVATE pim_core)
add_test(NAME services COMMAND test_services)
