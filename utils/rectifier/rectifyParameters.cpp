/**
 * \file rectifyParameters.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include <vector>
#include <stddef.h>
#include "rectifyParameters.h"

/**
 *  Looks extremely unsafe because it depends on the order of static initialization.
 *  Should check standard if this is ok
 *
 *  Also it's not clear why removing "= Reflection()" breaks the code;
 **/

namespace corecvs {
template<>
Reflection BaseReflection<RectifyParameters>::reflection = Reflection();
template<>
int BaseReflection<RectifyParameters>::dummy = RectifyParameters::staticInit();
} // namespace corecvs 

SUPPRESS_OFFSET_WARNING_BEGIN

int RectifyParameters::staticInit()
{

    ReflectionNaming &nameing = naming();
    nameing = ReflectionNaming(
        "Rectify Parameters",
        "osd parameters",
        ""
    );
     

    fields().push_back(
        new EnumField
        (
          RectifyParameters::MATCHINGMETHOD_ID,
          offsetof(RectifyParameters, mMatchingMethod),
          1,
          "matchingMethod",
          "matchingMethod",
          "matchingMethod",
          new EnumReflection(2
          , new EnumOption(0,"SurfCV")
          , new EnumOption(1,"viTech")
          )
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::HESSIANTHRESHOLD_ID,
          offsetof(RectifyParameters, mHessianThreshold),
          400,
          "hessianThreshold",
          "hessianThreshold",
          "hessianThreshold"
        )
    );
    fields().push_back(
        new IntField
        (
          RectifyParameters::OCTAVES_ID,
          offsetof(RectifyParameters, mOctaves),
          3,
          "octaves",
          "octaves",
          "octaves"
        )
    );
    fields().push_back(
        new IntField
        (
          RectifyParameters::OCTAVELAYERS_ID,
          offsetof(RectifyParameters, mOctaveLayers),
          4,
          "octaveLayers",
          "octaveLayers",
          "octaveLayers"
        )
    );
    fields().push_back(
        new BoolField
        (
          RectifyParameters::EXTENDED_ID,
          offsetof(RectifyParameters, mExtended),
          false,
          "extended",
          "extended",
          "extended"
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::FILTERMINIMUMLENGTH_ID,
          offsetof(RectifyParameters, mFilterMinimumLength),
          3,
          "filterMinimumLength",
          "filterMinimumLength",
          "filterMinimumLength",
          true,
         0,
         99
        )
    );
    fields().push_back(
        new BoolField
        (
          RectifyParameters::USEKLT_ID,
          offsetof(RectifyParameters, mUseKLT),
          false,
          "useKLT",
          "useKLT",
          "useKLT"
        )
    );
    fields().push_back(
        new BoolField
        (
          RectifyParameters::COMPUTEESSENTIAL_ID,
          offsetof(RectifyParameters, mComputeEssential),
          true,
          "computeEssential",
          "computeEssential",
          "computeEssential"
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::PRIORFOCAL_ID,
          offsetof(RectifyParameters, mPriorFocal),
          820.427,
          "priorFocal",
          "priorFocal",
          "priorFocal",
          true,
         0,
         99999
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::PRIORFOCAL2_ID,
          offsetof(RectifyParameters, mPriorFocal2),
          820.427,
          "priorFocal2",
          "priorFocal2",
          "priorFocal2",
          true,
         0,
         99999
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::BASELINELENGTH_ID,
          offsetof(RectifyParameters, mBaselineLength),
          60,
          "baselineLength",
          "baselineLength",
          "baselineLength",
          true,
         -1000,
         1000
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::FOVANGLE_ID,
          offsetof(RectifyParameters, mFovAngle),
          60,
          "fovAngle",
          "fovAngle",
          "fovAngle",
          true,
         0,
         20
        )
    );
    fields().push_back(
        new EnumField
        (
          RectifyParameters::ESTIMATIONMETHOD_ID,
          offsetof(RectifyParameters, mEstimationMethod),
          1,
          "estimationMethod",
          "estimationMethod",
          "estimationMethod",
          new EnumReflection(3
          , new EnumOption(0,"RANSAC")
          , new EnumOption(1,"Iterative")
          , new EnumOption(2,"Manual")
          )
        )
    );
    fields().push_back(
        new BoolField
        (
          RectifyParameters::NORMALISE_ID,
          offsetof(RectifyParameters, mNormalise),
          true,
          "normalise",
          "normalise",
          "normalise"
        )
    );
    fields().push_back(
        new IntField
        (
          RectifyParameters::RANSACITERATIONS_ID,
          offsetof(RectifyParameters, mRansacIterations),
          4000,
          "ransacIterations",
          "ransacIterations",
          "ransacIterations",
          true,
         1,
         9999
        )
    );
    fields().push_back(
        new IntField
        (
          RectifyParameters::RANSACTESTSIZE_ID,
          offsetof(RectifyParameters, mRansacTestSize),
          15,
          "ransacTestSize",
          "ransacTestSize",
          "ransacTestSize",
          true,
         1,
         9999
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::RANSACTHRESHOLD_ID,
          offsetof(RectifyParameters, mRansacThreshold),
          1,
          "ransacThreshold",
          "ransacThreshold",
          "ransacThreshold",
          true,
         1,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::BASELINEX_ID,
          offsetof(RectifyParameters, mBaselineX),
          1,
          "baselineX",
          "baselineX",
          "baselineX",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::BASELINEY_ID,
          offsetof(RectifyParameters, mBaselineY),
          0,
          "baselineY",
          "baselineY",
          "baselineY",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::BASELINEZ_ID,
          offsetof(RectifyParameters, mBaselineZ),
          0,
          "baselineZ",
          "baselineZ",
          "baselineZ",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new EnumField
        (
          RectifyParameters::ITERATIVEMETHOD_ID,
          offsetof(RectifyParameters, mIterativeMethod),
          2,
          "iterativeMethod",
          "iterativeMethod",
          "iterativeMethod",
          new EnumReflection(5
          , new EnumOption(0,"SVD")
          , new EnumOption(1,"Gradient Descent")
          , new EnumOption(2,"Marquardt Levenberg")
          , new EnumOption(3,"Classic Kalman")
          , new EnumOption(4,"Kalman")
          )
        )
    );
    fields().push_back(
        new IntField
        (
          RectifyParameters::ITERATIVEITERATIONS_ID,
          offsetof(RectifyParameters, mIterativeIterations),
          30,
          "iterativeIterations",
          "iterativeIterations",
          "iterativeIterations",
          true,
         1,
         9999
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::ITERATIVEINITIALSIGMA_ID,
          offsetof(RectifyParameters, mIterativeInitialSigma),
          30,
          "iterativeInitialSigma",
          "iterativeInitialSigma",
          "iterativeInitialSigma",
          true,
         0,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::ITERATIVEFACTORSIGMA_ID,
          offsetof(RectifyParameters, mIterativeFactorSigma),
          0.95,
          "iterativeFactorSigma",
          "iterativeFactorSigma",
          "iterativeFactorSigma",
          true,
         0,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::MANUALX_ID,
          offsetof(RectifyParameters, mManualX),
          1,
          "manualX",
          "manualX",
          "manualX",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::MANUALY_ID,
          offsetof(RectifyParameters, mManualY),
          0,
          "manualY",
          "manualY",
          "manualY",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::MANUALZ_ID,
          offsetof(RectifyParameters, mManualZ),
          0,
          "manualZ",
          "manualZ",
          "manualZ",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::MANUALPITCH_ID,
          offsetof(RectifyParameters, mManualPitch),
          0,
          "manualPitch",
          "manualPitch",
          "manualPitch",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::MANUALYAW_ID,
          offsetof(RectifyParameters, mManualYaw),
          0,
          "manualYaw",
          "manualYaw",
          "manualYaw",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::MANUALROLL_ID,
          offsetof(RectifyParameters, mManualRoll),
          0,
          "manualRoll",
          "manualRoll",
          "manualRoll",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::ZDIRX_ID,
          offsetof(RectifyParameters, mZdirX),
          1,
          "zdirX",
          "zdirX",
          "zdirX",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::ZDIRY_ID,
          offsetof(RectifyParameters, mZdirY),
          0,
          "zdirY",
          "zdirY",
          "zdirY",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new DoubleField
        (
          RectifyParameters::ZDIRZ_ID,
          offsetof(RectifyParameters, mZdirZ),
          0,
          "zdirZ",
          "zdirZ",
          "zdirZ",
          true,
         -20,
         20
        )
    );
    fields().push_back(
        new BoolField
        (
          RectifyParameters::AUTOZ_ID,
          offsetof(RectifyParameters, mAutoZ),
          true,
          "autoZ",
          "autoZ",
          "autoZ"
        )
    );
    fields().push_back(
        new BoolField
        (
          RectifyParameters::AUTOSHIFT_ID,
          offsetof(RectifyParameters, mAutoShift),
          true,
          "autoShift",
          "autoShift",
          "autoShift"
        )
    );
    fields().push_back(
        new IntField
        (
          RectifyParameters::PRESHIFT_ID,
          offsetof(RectifyParameters, mPreShift),
          0,
          "preShift",
          "preShift",
          "preShift",
          true,
         -9999,
         9999
        )
    );
    fields().push_back(
        new IntField
        (
          RectifyParameters::GUESSSHIFTTHRESHOLD_ID,
          offsetof(RectifyParameters, mGuessShiftThreshold),
          0,
          "guessShiftThreshold",
          "guessShiftThreshold",
          "guessShiftThreshold",
          true,
         0,
         99999
        )
    );
   return 0;
}

SUPPRESS_OFFSET_WARNING_END

