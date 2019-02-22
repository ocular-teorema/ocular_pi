#ifndef VIMOUSE_3D_STEREO_STYLE_H_
#define VIMOUSE_3D_STEREO_STYLE_H_
/**
 * \file viMouse3dStereoStyle.h
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

/**
 * Helper namespace to hide ViMouse3dStereoStyle enum from global context.
 */

namespace ViMouse3dStereoStyle {

/** 
 * \brief ViMouse 3d Stereo Style 
 * ViMouse 3d Stereo Style 
 */
enum ViMouse3dStereoStyle {
    /** 
     * \brief Grey value 
     * Grey value 
     */
    GREY_VALUE = 0,
    /** 
     * \brief Z Coordinate 
     * Z Coordinate 
     */
    Z_COORDINATE = 1,
    /** 
     * \brief Y Coordinate 
     * Y Coordinate 
     */
    Y_COORDINATE = 2,
    /** 
     * \brief Distance 
     * Distance 
     */
    DISTANCE = 3,
    /** 
     * \brief None 
     * None 
     */
    NONE = 4,
    /** 
     * \brief By Flag 
     * By Flag 
     */
    BY_FLAG = 5,
    /** 
     * \brief By Cluster 
     * By Cluster 
     */
    BY_CLUSTER = 6,
    /** 
     * \brief Last virtual option to run cycles to
     */
    VIMOUSE_3D_STEREO_STYLE_LAST
};


static inline const char *getName(const ViMouse3dStereoStyle &value)
{
    switch (value) 
    {
     case GREY_VALUE : return "GREY_VALUE"; break ;
     case Z_COORDINATE : return "Z_COORDINATE"; break ;
     case Y_COORDINATE : return "Y_COORDINATE"; break ;
     case DISTANCE : return "DISTANCE"; break ;
     case NONE : return "NONE"; break ;
     case BY_FLAG : return "BY_FLAG"; break ;
     case BY_CLUSTER : return "BY_CLUSTER"; break ;
     default : return "Not in range"; break ;
     
    }
    return "Not in range";
}

} //namespace ViMouse3dStereoStyle

#endif  //VIMOUSE_3D_STEREO_STYLE_H_