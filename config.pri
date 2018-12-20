
CONFIG +=       \
        debug  \
#        release \

#CONFIG += with_piui

CONFIG +=       \
   with_sse     \
   with_sse3    \
   with_sse4

# include standard part for any project that tunes some specific parameters that depend of the config been set above

ROOT_PATH=$$PWD
include(common.pri)
