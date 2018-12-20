# try use global config 
exists(../../../config.pri) {
    #message(Using global config)
    ROOT_DIR=../../../
} else { 
    message(Using local config)
    ROOT_DIR=../    
}
include($$ROOT_DIR/config.pri)
    

CONFIG  += staticlib
TARGET   = cvs_core
TEMPLATE = lib

COREDIR = .
include(core.pri)                                   # it uses COREDIR, TARGET and detects COREBINDIR!

OBJECTS_DIR = $$ROOT_DIR/.obj/cvs_core$$BUILD_CFG_NAME
MOC_DIR     = $$OBJECTS_DIR
UI_DIR      = $$OBJECTS_DIR

TARGET  = $$join(TARGET,,,$$BUILD_CFG_SFX)          # add 'd' at the end for debug versions

DESTDIR = $$COREBINDIR

# to delete also target lib by 'clean' make command (distclean does this)
win32 {
    QMAKE_CLEAN += "$(DESTDIR_TARGET)"              # for Linux qmake doesn't generate DESTDIR_TARGET :(
} else {
    QMAKE_CLEAN += "$(DESTDIR)$(TARGET)"            # for win such cmd exists with inserted space :(
}

#
# include sources and headers for each subdir
#
for (MODULE, CORE_SUBMODULES) {
    message (Adding module $${MODULE} )
    include($${MODULE}/$${MODULE}.pri)
}

include(xml/generated/generated.pri)

#CONFIG += with_simplecore


OTHER_FILES +=            \
    xml/parameters.xml    \
    xml/bufferFilters.xml \
    xml/clustering1.xml   \
    xml/filterBlock.xml   \
    xml/precise.xml       \
    xml/distortion.xml    \

OTHER_FILES +=            \
    ../tools/generator/regen-core.sh \
    ../tools/generator/h_stub.sh \


