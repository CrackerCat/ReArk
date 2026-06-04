file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qml)

qt_add_library(reark_qml_module STATIC)
qt_add_qml_module(reark_qml_module
    URI "ReArk"
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        ${QML_FILES}
)
