#!/bin/bash

source ./helper-regen.sh

analytic="../../applications/videoAnalytic/"

echo -n "Building generator... "
qmake-qt4 && make
echo "done"

echo -n "Running generator on xml/analytic.xml..."
${GENERATOR_BIN} xml/analytic.xml 
echo "done"


echo "Making a copy of base classes"
./copy-base.sh


echo "Making a copy of analytic classes"
copy_if_different Generated/analyticParametersControlWidget.cpp             $analytic/
copy_if_different Generated/homography4PointParametersControlWidget.cpp     $analytic/

copy_if_different Generated/analyticParameters.cpp                      $analytic/generatedParameters
copy_if_different Generated/analyticParameters.h                        $analytic/generatedParameters
copy_if_different Generated/homography4PointParameters.cpp              $analytic/generatedParameters
copy_if_different Generated/homography4PointParameters.h                $analytic/generatedParameters
copy_if_different Generated/firstOrderModelParameters.cpp               $analytic/generatedParameters
copy_if_different Generated/firstOrderModelParameters.h                 $analytic/generatedParameters

copy_if_different Generated/backgroundDetector.h                        $analytic/generatedParameters
copy_if_different Generated/forceDatabaseOverride.h                     $analytic/generatedParameters
copy_if_different Generated/analyticParametersControlWidget.ui          $analytic/ui
copy_if_different Generated/homography4PointParametersControlWidget.ui  $analytic/ui



copy_if_different Generated/parametersMapperAnalytic.cpp            $analytic/parametersMapper
copy_if_different Generated/parametersMapperAnalytic.h              $analytic/parametersMapper


echo "copied"
