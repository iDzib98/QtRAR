cmake_minimum_required(VERSION 3.21)

project(QtRAR
    VERSION 0.1.0
    DESCRIPTION "WinRAR-style GUI for Linux on top of the official RAR binaries"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# La licencia del nucleo es GPL-3.0-or-later; los binarios de RAR tienen su
# propia licencia y NO se distribuyen con la aplicacion (ver .gitignore).
option(QTRAR_BUNDLE_UNRAR
    "Instalar unrar junto a QtRAR. Es la unica excepcion permitida por la EULA \
de rarlab (clausula 3.a: los componentes UnRAR si pueden distribuirse por separado)." OFF)
option(QTRAR_FETCH_RAR
    "Descargar automaticamente los binarios oficiales de RAR si faltan (solo desarrollo)." ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
endif()

qt_standard_project_setup()

if(QTRAR_FETCH_RAR AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bin/unrar")
    message(STATUS "QtRAR: los binarios de RAR no estan en bin/, descargando...")
    execute_process(
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/fetch-rar.sh"
        RESULT_VARIABLE _fetch_result
        OUTPUT_VARIABLE _fetch_out
        ERROR_VARIABLE  _fetch_err)
    if(_fetch_result EQUAL 0)
        message(STATUS "QtRAR: binarios descargados\n${_fetch_out}")
    else()
        message(WARNING "QtRAR: no se pudieron descargar los binarios de RAR.\n"
                        "${_fetch_err}\n"
                        "Se compilara igualmente; ejecuta tools/fetch-rar.sh antes de usarlo.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Nucleo: sin dependencias de Qt Widgets, se puede testear por separado.
# ---------------------------------------------------------------------------
add_library(qtrar_core STATIC
    src/core/ArchiveService.cpp
    src/core/ArchiveService.h
    src/core/BinaryLocator.cpp
    src/core/BinaryLocator.h
    src/core/Diagnostics.cpp
    src/core/Diagnostics.h
    src/core/HFormat.cpp
    src/core/HFormat.h
    src/core/LicenseProbe.cpp
    src/core/LicenseProbe.h
    src/core/ListParser.cpp
    src/core/ListParser.h
    src/core/ProcessRunner.cpp
    src/core/ProcessRunner.h
    src/core/Types.cpp
    src/core/TranslationCatalog.cpp
    src/core/Types.h
)
target_include_directories(qtrar_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(qtrar_core PUBLIC Qt6::Core Qt6::Gui)

# ---------------------------------------------------------------------------
# Aplicacion
# ---------------------------------------------------------------------------
# La interfaz va en una biblioteca aparte, no dentro del ejecutable, para que
# los tests puedan enlazarla: el filtro de busqueda y la validacion del dialogo
# de creacion son logica de verdad, no maquetacion, y merece la pena probarlos
# sin montar una ventana.
qt_add_library(qtrar_ui STATIC
    src/model/ArchiveTreeModel.cpp
    src/model/ArchiveTreeModel.h
    src/ui/AddressBar.cpp
    src/ui/AddressBar.h
    src/ui/CommandRegistry.cpp
    src/ui/CommandRegistry.h
    src/ui/FindProxyModel.cpp
    src/ui/FindProxyModel.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/StatusPanel.cpp
    src/ui/StatusPanel.h
    src/ui/VirusScan.cpp
    src/ui/VirusScan.h
    src/ui/ThemeManager.cpp
    src/ui/ThemeManager.h
    src/ui/dialogs/CreateDialog.cpp
    src/ui/dialogs/CreateDialog.h
    src/ui/dialogs/OptionsDialog.cpp
    src/ui/dialogs/OptionsDialog.h
    src/ui/dialogs/ToolbarDialog.cpp
    src/ui/dialogs/ToolbarDialog.h
    src/ui/dialogs/ExtractDialog.cpp
    src/ui/dialogs/ExtractDialog.h
    src/ui/dialogs/LicenseDialog.cpp
    src/ui/dialogs/LicenseDialog.h
    src/ui/dialogs/RarSetupDialog.cpp
    src/ui/dialogs/RarSetupDialog.h
    src/ui/dialogs/PasswordDialog.cpp
    src/ui/dialogs/PasswordDialog.h
    src/ui/dialogs/ProgressDialog.cpp
    src/ui/dialogs/ProgressDialog.h
)
target_include_directories(qtrar_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(qtrar_ui PUBLIC qtrar_core Qt6::Widgets)

qt_add_executable(QtRAR
    src/main.cpp
    src/app/Application.cpp
    src/app/HeadlessProbe.cpp
    src/app/HeadlessProbe.h
    src/app/Application.h
)
target_link_libraries(QtRAR PRIVATE qtrar_ui)

# Recursos: los SVG originales y la hoja de estilo viajan dentro del binario,
# para que la aplicacion funcione sin instalar ficheros sueltos.
qt_add_resources(QtRAR "qtrar_res"
    PREFIX "/"
    BASE   "res"
    FILES
        res/themes/original.qss
        res/icons/about.svg
        res/icons/add.svg
        res/icons/app.svg
        res/icons/archive-locked.svg
        res/icons/archive.svg
        res/icons/back.svg
        res/icons/close.svg
        res/icons/code-locked.svg
        res/icons/code.svg
        res/icons/comment.svg
        res/icons/delete.svg
        res/icons/deselectall.svg
        res/icons/document-locked.svg
        res/icons/document.svg
        res/icons/encrypt.svg
        res/icons/extract.svg
        res/icons/file-locked.svg
        res/icons/file.svg
        res/icons/find.svg
        res/icons/folder-locked.svg
        res/icons/folder.svg
        res/icons/goto.svg
        res/icons/help.svg
        res/icons/image-locked.svg
        res/icons/image.svg
        res/icons/info.svg
        res/icons/invert.svg
        res/icons/media-locked.svg
        res/icons/media.svg
        res/icons/newarchive.svg
        res/icons/open.svg
        res/icons/options.svg
        res/icons/password.svg
        res/icons/protect.svg
        res/icons/qtrar.svg
        res/icons/refresh.svg
        res/icons/rename.svg
        res/icons/repair.svg
        res/icons/selectall.svg
        res/icons/sfx.svg
        res/icons/shield.svg
        res/icons/test.svg
        res/icons/up.svg
        res/icons/view.svg
        res/icons/wizard.svg
)

# Version visible en el "Acerca de" y en --version. Va en la biblioteca de
# interfaz porque el dialogo "Acerca de" esta ahi, no en el ejecutable.
target_compile_definitions(qtrar_ui PUBLIC
    QTRAR_VERSION="${PROJECT_VERSION}")

target_link_libraries(QtRAR PRIVATE
    qtrar_core
    Qt6::Widgets
    Qt6::Network
)

# Solo unrar puede distribuirse junto a la aplicacion (EULA rarlab 3.a).
# `rar` nunca se empaqueta: el script tools/fetch-rar.sh lo deja en bin/ para
# desarrollo y el usuario lo instala por su cuenta.
if(QTRAR_BUNDLE_UNRAR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bin/unrar")
        install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/bin/unrar"
                DESTINATION ${CMAKE_INSTALL_BINDIR})
    else()
        # Sin este aviso el fallo es invisible: el paquete sale sin unrar y nada
        # falla hasta que alguien lo nota al abrir un RAR. Suele passar en CI,
        # donde `bin/` no existe y QTRAR_FETCH_RAR va en OFF.
        message(FATAL_ERROR
            "QTRAR_BUNDLE_UNRAR=ON pero no hay bin/unrar. "
            "Ejecuta tools/fetch-rar.sh, o quita la opcion.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Instalacion
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS QtRAR
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES res/icons/qtrar.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES packaging/qtrar.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/qtrar-archives.desktop packaging/qtrar-add.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/kio/servicemenus)
install(PROGRAMS
    "packaging/nautilus-scripts/QtRAR/Extraer aquí"
    "packaging/nautilus-scripts/QtRAR/Extraer en..."
    "packaging/nautilus-scripts/QtRAR/Comprobar archivo"
    "packaging/nautilus-scripts/QtRAR/Añadir a un archivo..."
    DESTINATION ${CMAKE_INSTALL_DATADIR}/nautilus/scripts/QtRAR)
install(FILES
    packaging/README-context-menu.md
    packaging/README-deb.md
    licenses/license.txt
    licenses/acknow.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/qtrar)
# Documentacion del proyecto. `licenses/license.txt` y `acknow.txt` NO son la
# licencia de QtRAR: son los textos de RARLAB, que se instalan arriba porque su
# EULA obliga a acompanar a sus componentes. La licencia de QtRAR esta en la
# raiz del repositorio.
#
# Ojo con los nombres: todo va al mismo directorio, asi que dos ficheros
# llamados README.md se pisarian en silencio y el paquete acabaria con la
# licencia de terceros en lugar del README.
install(FILES
    README.md
    CONTRIBUTING.md
    CHANGELOG.md
    docs/ARCHITECTURE.md
    docs/TRANSLATING.md
    licenses/THIRD-PARTY-NOTICES.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/qtrar)
# Debian busca el aviso de derechos de autor en `copyright`, no en `LICENSE`.
install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/qtrar
    RENAME copyright)

# ---------------------------------------------------------------------------
# Traducciones: es + en desde el primer commit
# ---------------------------------------------------------------------------
set(QTRAR_TS_FILES
    res/i18n/qtrar_es.ts
    res/i18n/qtrar_en.ts
)
# `qtrar_ui` tiene que estar en la lista: si no, `lupdate` deja de mirar sus
# ficheros, los da por obsoletos y el .qm se queda sin la mitad de los textos.
qt_add_lupdate(QtRAR
    TS_FILES ${QTRAR_TS_FILES}
    SOURCE_TARGETS QtRAR qtrar_core qtrar_ui)
qt_add_lrelease(QtRAR
    TS_FILES ${QTRAR_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QTRAR_QM_FILES)
add_custom_target(translations ALL DEPENDS ${QTRAR_QM_FILES})
add_dependencies(QtRAR translations)
install(FILES ${QTRAR_QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/qtrar/i18n)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
# Las traducciones compiladas se dejan junto al ejecutable para que
# `applicationDirPath()/i18n` las encuentre tanto en el arbol de fuentes
# como en una instalacion normal.
#
# No vale un POST_BUILD sobre QtRAR: si solo cambian los .ts, el ejecutable no
# se relinga y la copia de los .qm se queda obsoleta. Se registra un comando
# propio que depende de los .qm, y `translations` lo fuerza siempre.
set(QTRAR_QM_STAMP "${CMAKE_CURRENT_BINARY_DIR}/i18n.stamp")
add_custom_command(
    OUTPUT "${QTRAR_QM_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:QtRAR>/i18n"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${QTRAR_QM_FILES}
            "$<TARGET_FILE_DIR:QtRAR>/i18n"
    COMMAND ${CMAKE_COMMAND} -E touch "${QTRAR_QM_STAMP}"
    DEPENDS ${QTRAR_QM_FILES}
    COMMENT "Copiando traducciones junto al ejecutable"
    VERBATIM
)
add_custom_target(copy_translations ALL DEPENDS "${QTRAR_QM_STAMP}")
add_dependencies(copy_translations translations)

# ---------------------------------------------------------------------------
# Paquete Debian
# ---------------------------------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "qtrar")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "QtRAR")
set(CPACK_PACKAGE_CONTACT "QtRAR maintainers <maintainers@qtrar.local>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Gestor de archivos RAR y ZIP con interfaz WinRAR")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "QtRAR maintainers <maintainers@qtrar.local>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
# Las dependencias de Qt se calculan desde los ELF instalados. 7-Zip es
# opcional para RAR y se recomienda para leer y extraer ZIP.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "7zip")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "rar")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGE_FILE_NAME "qtrar_${PROJECT_VERSION}_${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
