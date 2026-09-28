add_executable(tst_core tst_core.cpp)
target_link_libraries(tst_core PRIVATE qtrar_core Qt6::Test)
target_include_directories(tst_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Las fixtures son salidas reales de unrar 7.23 capturadas con
# tools/capture-fixtures.sh. Se pasan por ruta de codigo fuente para que el
# test no dependa del directorio de trabajo.
target_compile_definitions(tst_core PRIVATE
    QTEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME tst_core COMMAND tst_core)

# Pruebas de la interfaz. Necesitan QApplication, asi que corren con
# QT_QPA_PLATFORM=offscreen: no tocan la pantalla pero instancian widgets de
# verdad.
add_executable(tst_ui tst_ui.cpp)
target_sources(tst_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../res/qtrar.qrc)
target_link_libraries(tst_ui PRIVATE qtrar_ui Qt6::Test)
target_include_directories(tst_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME tst_ui COMMAND tst_ui)
set_tests_properties(tst_ui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
