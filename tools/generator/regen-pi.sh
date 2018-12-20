#!/bin/bash
mkdir -p Generated

source ./helper-regen.sh

processInstance="../../applications/processInstance/"

echo -n "Building generator... "
qmake && make
echo "done"

echo -n "Running generator on xml/processInstance.xml..."
${GENERATOR_BIN} xml/pi.xml
echo "done"


echo "Making a copy of base classes"
./copy-base.sh


echo "Copy geneated data"

copy_if_different Generated/analysisParams.cpp                      $processInstance/Generated
copy_if_different Generated/analysisParams.h                        $processInstance/Generated

copy_if_different Generated/decisionParams.cpp                      $processInstance/Generated
copy_if_different Generated/decisionParams.h                        $processInstance/Generated

copy_if_different Generated/objectDetectionParams.cpp               $processInstance/Generated
copy_if_different Generated/objectDetectionParams.h                 $processInstance/Generated


copy_if_different Generated/eventProcessingParams.cpp               $processInstance/Generated
copy_if_different Generated/eventProcessingParams.h                 $processInstance/Generated

copy_if_different Generated/dateParams.cpp                          $processInstance/Generated
copy_if_different Generated/dateParams.h                            $processInstance/Generated

copy_if_different Generated/commonPipelineParameters.cpp            $processInstance/Generated
copy_if_different Generated/commonPipelineParameters.h              $processInstance/Generated


# U  U I
# U  U I
#  UU  I

uiTargetDir=$processInstance/ui/generated

ui_list="analysisParams \
         decisionParams \
         objectDetectionParams \
         eventProcessingParams \
         dateParams \
         commonPipelineParameters"

for class in ${ui_list} ; do

    file="${class}ControlWidget"
    copy_if_different Generated/${file}.ui  $uiTargetDir
    copy_if_different Generated/${file}.cpp $uiTargetDir

    headerFile="${uiTargetDir}/${file}.h"
    if [[ ! -e $headerFile ]]; then
            ./h_stub.sh ${class}
            mv ${class}ControlWidget.h $uiTargetDir
    fi;
done;



#echo "Making a copy of processInstance classes"
#copy_if_different Generated/processingInstanceParametersControlWidget.cpp   $processInstance/Generated
#copy_if_different Generated/processingInstanceParameters.cpp                      $processInstance/Generated
#copy_if_different Generated/processingInstanceParameters.h                        $processInstance/Generated
#copy_if_different Generated/processingInstanceParametersControlWidget.ui          $processInstance/Generated
#copy_if_different Generated/algorithmSelector.h                                   $processInstance/Generated


echo "copied"
