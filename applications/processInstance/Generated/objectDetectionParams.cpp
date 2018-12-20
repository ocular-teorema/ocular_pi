/**
 * \file objectDetectionParams.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include <vector>
#include <stddef.h>
#include "objectDetectionParams.h"

/**
 *  Looks extremely unsafe because it depends on the order of static initialization.
 *  Should check standard if this is ok
 *
 *  Also it's not clear why removing "= Reflection()" breaks the code;
 **/

namespace corecvs {
template<>
Reflection BaseReflection<ObjectDetectionParams>::reflection = Reflection();
template<>
int BaseReflection<ObjectDetectionParams>::dummy = ObjectDetectionParams::staticInit();
} // namespace corecvs 

SUPPRESS_OFFSET_WARNING_BEGIN

int ObjectDetectionParams::staticInit()
{

    ReflectionNaming &nameing = naming();
    nameing = ReflectionNaming(
        "Object Detection Params",
        "Object Detection Params",
        ""
    );
     

    fields().push_back(
        new BoolField
        (
          ObjectDetectionParams::DEBUG_OBJECTS_ID,
          offsetof(ObjectDetectionParams, mDebugObjects),
          false,
          "Debug objects",
          "Debug objects",
          "Debug objects"
        )
    );
    fields().push_back(
        new BoolField
        (
          ObjectDetectionParams::EXPERIMENTAL_ID,
          offsetof(ObjectDetectionParams, mExperimental),
          false,
          "Experimental",
          "Experimental",
          "Experimental"
        )
    );
    fields().push_back(
        new IntField
        (
          ObjectDetectionParams::MINIMUM_CLUSTER_ID,
          offsetof(ObjectDetectionParams, mMinimumCluster),
          50,
          "Minimum Cluster",
          "Minimum Cluster",
          "Minimum Cluster",
          true,
         1,
         999999
        )
    );
    fields().push_back(
        new IntField
        (
          ObjectDetectionParams::DILATE_SIZE_ID,
          offsetof(ObjectDetectionParams, mDilateSize),
          10,
          "Dilate Size",
          "Dilate Size",
          "Dilate Size",
          true,
         1,
         999999
        )
    );
    fields().push_back(
        new IntField
        (
          ObjectDetectionParams::BG_THRESHOLD_ID,
          offsetof(ObjectDetectionParams, mBgThreshold),
          15,
          "Bg threshold",
          "Bg threshold",
          "Bg threshold",
          true,
         1,
         99999
        )
    );
    fields().push_back(
        new DoubleField
        (
          ObjectDetectionParams::FG_THRESHOLD_ID,
          offsetof(ObjectDetectionParams, mFgThreshold),
          40,
          "Fg Threshold",
          "Fg Threshold",
          "Fg Threshold",
          true,
         1,
         100
        )
    );
   return 0;
}

SUPPRESS_OFFSET_WARNING_END


