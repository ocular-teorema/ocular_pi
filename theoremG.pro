TEMPLATE = subdirs
CONFIG  += ordered

SUBDIRS +=                   \
    core                     \
    utils                    \
    processInstance          \

utils.depends            += core
processInstance.depends  += core

core.file                     = core/core.pro
utils.file                    = utils/utils.pro
processInstance.file          = applications/processInstance/processInstance.pro

