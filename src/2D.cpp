/// @file       2D.cpp
/// @brief      Implementation of 2-D routines, like drawing aircraft labels
/// @details    2-D drawing is a bit "unnatural" as the aircraft are put into a 3-D world.
///             These functions work only after setting up a 2-D OpenGL projection.
/// @note       This means the code depends on OpenGL. This should be OK also in the long
///             run when X-Plane moves to Vulkan/Metal as I understood that OpenGL would still
///             be available for 2-D drawing, just not for 3-D. But this part might need review then.\n
///             Originally, I had hoped not to need all these matrixes and
///             their complex arithmetic any longer in XPMP2.\n
///             But there is just one single usage left: Determine, if a text label would be visible.
///             X-Plane's instancing deicdes itself if it needs to draw the plane, which is handled
///             as an object planed using [XPLMInstanceSetPosition](https://developer.x-plane.com/sdk/XPLMInstance/#XPLMInstanceSetPosition).
///             But we have no information, if and where (in terms of 2-D display coordinates) the object is drawn.
///             Hence we still need to calculate ourselves, if the object is visible or not.
/// @warning    Label drawing not only uses OpenGL calls, but also the deprecated
///             drawing callback functionality of X-Plane via
///             [XPLMRegisterDrawCallback](https://developer.x-plane.com/sdk/XPLMDisplay/#XPLMRegisterDrawCallback).
///             which is "likely [to] be removed during the X-Plane 11 run as part of the transition to Vulkan/Metal/etc."
///             So label drawing will need to be revisited then...or is finally gone for good.
/// @author     Birger Hoppe
/// @copyright  (c) 2020 Birger Hoppe
/// @author     Ben Supnik and Chris Serio (for large parts, indicated below per function)
/// @copyright  Copyright (c) 2004, Ben Supnik and Chris Serio (for parts, indicated below per function)
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

#include "XPMP2.h"

// This is the only file using OpenGL
#if IBM
#include <GL/gl.h>
#elif APL
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif


#define ERR_REGISTER_DRAW_CB    "Could not register drawing callback. Labels will not draw!"
#define DEBUG_ENABLE_AC_LABELS  "Aircraft labels %s"

namespace XPMP2 {

//
// MARK: 2-D projection calculations
//

/// Copies a standard C array into a valarray
template<class T>
std::valarray<T>& copyToValarray (std::valarray<T>& dest, const T* src, size_t size)
{
    // asserten correct valarray size
    if (dest.size() != size)
        dest.resize(size);
    // do the copy
    for (size_t i = 0; i < size; ++i)
        dest[i] = src[i];
    return dest;
}

/// This structure keeps the required OpenGL matrix definitions (parts taken over from otiginal libxplanemp's `cull_info_t`)
struct matrixesTy {
    /// The model view matrix, to get from local OpenGL to eye coordinates.
    std::valarray<float> modelView;
    /// Projection matrix
    std::valarray<float> proj;
    
    ///< Four clip planes in the form of Ax + By + Cz + D = 0 (ABCD are in the array.
    ///< They are oriented so the positive side of the clip plane is INSIDE the view volume.
    std::valarray<float> neaClip;
    std::valarray<float> farClip;
    std::valarray<float> lftClip;
    std::valarray<float> rgtClip;
    std::valarray<float> botClip;
    std::valarray<float> topClip;
    
    /// viewport
    std::valarray<GLint> viewport;
    
    /// Constructor ensure correct valarray sizes
    matrixesTy () :
    modelView(0.0f, 16), proj(0.0f, 16),
    neaClip(0.0f, 4), farClip(0.0f, 4),
    lftClip(0.0f, 4), rgtClip(0.0f, 4),
    botClip(0.0f, 4), topClip(0.0f, 4),
    viewport(0, 4)
    {}

};

// a few dataRefs we need:
XPLMDataRef drModelviewMatrix   = nullptr;  ///< sim/graphics/view/modelview_matrix
XPLMDataRef drProjectionMatrix  = nullptr;  ///< sim/graphics/view/projection_matrix
XPLMDataRef drViewport          = nullptr;  ///< sim/graphics/view/viewport
XPLMDataRef drMSAAXRatio        = nullptr;  ///< sim/private/controls/hdr/fsaa_ratio_x
XPLMDataRef drMSAAYRatio        = nullptr;  ///< sim/private/controls/hdr/fsaa_ratio_y
XPLMDataRef drHDROnRef          = nullptr;  ///< sim/graphics/settings/HDR_on
XPLMDataRef drVisibility        = nullptr;  ///< sim/graphics/view/visibility_effective_m or sim/weather/visibility_effective_m
XPLMDataRef drWorldRenderTy     = nullptr;  ///< sim/graphics/view/world_render_type
XPLMDataRef drPlaneRenderTy     = nullptr;  ///< sim/graphics/view/plane_render_type

/// @brief Sets up model-view and projection matrix for OpenGL drawing
/// @see `setup_cull_info` and `XPMPDefaultPlaneRenderer` of original libxplanemp, of which this is an adapted version
/// @author Ben Supnik, Chris Serio, and Chris Collins
void SetupCullInfo(matrixesTy& m)
{
    // First, just read out the current OpenGL matrices...do this once at setup
    // because it's not the fastest thing to do.
    //
    // if our X-Plane version supports it, pull it from the daatrefs to avoid a
    // potential driver stall.
    float af[16];
    if (!drModelviewMatrix || !drProjectionMatrix) {
        glGetFloatv(GL_MODELVIEW_MATRIX,  af);
        copyToValarray(m.modelView, af, 16);
        glGetFloatv(GL_PROJECTION_MATRIX, af);
        copyToValarray(m.proj,      af, 16);
    } else {
        XPLMGetDatavf(drModelviewMatrix,  af, 0, 16);
        copyToValarray(m.modelView, af, 16);
        XPLMGetDatavf(drProjectionMatrix, af, 0, 16);
        copyToValarray(m.proj,      af, 16);
    }
    
    // Now...what the heck is this?  Here's the deal: the clip planes have values in "clip" coordinates of: Left = (1,0,0,1)
    // Right = (-1,0,0,1), Bottom = (0,1,0,1), etc.  (Clip coordinates are coordinates from -1 to 1 in XYZ that the driver
    // uses.  The projection matrix converts from eye to clip coordinates.)
    //
    // How do we convert a plane backward from clip to eye coordinates?  Well, we need the transpose of the inverse of the
    // inverse of the projection matrix.  (Transpose of the inverse is needed to transform a plane, and the inverse of the
    // projection is the matrix that goes clip -> eye.)  Well, that cancels out to the transpose of the projection matrix,
    // which is nice because it means we don't need a matrix inversion in this bit of sample code.
    
    // So this nightmare down here is simply:
    // clip plane * transpose (proj_matrix)
    // worked out for all six clip planes.  If you squint you can see the patterns:
    // L:  1  0 0 1
    // R: -1  0 0 1
    // B:  0  1 0 1
    // T:  0 -1 0 1
    // etc.
    m.lftClip[0] = m.proj[0]+m.proj[3];    m.lftClip[1] = m.proj[4]+m.proj[7];    m.lftClip[2] = m.proj[8]+m.proj[11];    m.lftClip[3] = m.proj[12]+m.proj[15];
    m.rgtClip[0] =-m.proj[0]+m.proj[3];    m.rgtClip[1] =-m.proj[4]+m.proj[7];    m.rgtClip[2] =-m.proj[8]+m.proj[11];    m.rgtClip[3] =-m.proj[12]+m.proj[15];
    
    m.botClip[0] = m.proj[1]+m.proj[3];    m.botClip[1] = m.proj[5]+m.proj[7];    m.botClip[2] = m.proj[9]+m.proj[11];    m.botClip[3] = m.proj[13]+m.proj[15];
    m.topClip[0] =-m.proj[1]+m.proj[3];    m.topClip[1] =-m.proj[5]+m.proj[7];    m.topClip[2] =-m.proj[9]+m.proj[11];    m.topClip[3] =-m.proj[13]+m.proj[15];

    m.neaClip[0] = m.proj[2]+m.proj[3];    m.neaClip[1] = m.proj[6]+m.proj[7];    m.neaClip[2] = m.proj[10]+m.proj[11];   m.neaClip[3] = m.proj[14]+m.proj[15];
    m.farClip[0] =-m.proj[2]+m.proj[3];    m.farClip[1] =-m.proj[6]+m.proj[7];    m.farClip[2] =-m.proj[10]+m.proj[11];   m.farClip[3] =-m.proj[14]+m.proj[15];

    // Determine the viewport
    int vpInt[4] = {0,0,0,0};
    if (drViewport != nullptr) {
        // sim/graphics/view/viewport    int[4]    n    Pixels    Current OpenGL viewport in device window coordinates.
        // Note that this is left, bottom, right top, NOT left, bottom, width, height!!
        XPLMGetDatavi(drViewport, vpInt, 0, 4);
        m.viewport[0] = vpInt[0];
        m.viewport[1] = vpInt[1];
        m.viewport[2] = vpInt[2] - vpInt[0];
        m.viewport[3] = vpInt[3] - vpInt[1];
    } else {
        glGetIntegerv(GL_VIEWPORT, vpInt);
        copyToValarray(m.viewport, vpInt, 4);
    }

}

/// @brief Determines if a given sphere is visible in the user's current view
/// @see `sphere_is_visible` of original libxplanemp, of which this is an adapted copy
/// @author Ben Supnik, Chris Serio, and Chris Collins
bool IsSphereVisible (const matrixesTy& m, std::valarray<float>& pt_mv, float r)
{
    // First: we transform our coordinate into eye coordinates from model-view.

    // The following is already pre-computed in 'pt_mv'
    // float xp = x * m.modelView[0] + y * m.modelView[4] + z * m.modelView[ 8] + m.modelView[12];
    // float yp = x * m.modelView[1] + y * m.modelView[5] + z * m.modelView[ 9] + m.modelView[13];
    // float zp = x * m.modelView[2] + y * m.modelView[6] + z * m.modelView[10] + m.modelView[14];
    pt_mv[3] = 1.0f;

    // Now - we apply the "plane equation" of each clip plane to see how far from the clip plane our point is.
    // The clip planes are directed: positive number distances mean we are INSIDE our viewing area by some distance;
    // negative means outside.  So ... if we are outside by less than -r, the ENTIRE sphere is out of bounds.
    // We are not visible!  We do the near clip plane, then sides, then far, in an attempt to try the planes
    // that will eliminate the most geometry first...half the world is behind the near clip plane, but not much is
    // behind the far clip plane on sunny day.
    // if ((pt_mv[0] * m.neaClip[0] + pt_mv[1] * m.neaClip[1] + pt_mv[2] * m.neaClip[2] + pt_mv[3] * m.neaClip[3]) < -r)    return false;
    // if ((pt_mv[0] * m.botClip[0] + pt_mv[1] * m.botClip[1] + pt_mv[2] * m.botClip[2] + pt_mv[3] * m.botClip[3]) < -r)    return false;
    // if ((pt_mv[0] * m.topClip[0] + pt_mv[1] * m.topClip[1] + pt_mv[2] * m.topClip[2] + pt_mv[3] * m.topClip[3]) < -r)    return false;
    // if ((pt_mv[0] * m.lftClip[0] + pt_mv[1] * m.lftClip[1] + pt_mv[2] * m.lftClip[2] + pt_mv[3] * m.lftClip[3]) < -r)    return false;
    // if ((pt_mv[0] * m.rgtClip[0] + pt_mv[1] * m.rgtClip[1] + pt_mv[2] * m.rgtClip[2] + pt_mv[3] * m.rgtClip[3]) < -r)    return false;
    // if ((pt_mv[0] * m.farClip[0] + pt_mv[1] * m.farClip[1] + pt_mv[2] * m.farClip[2] + pt_mv[3] * m.farClip[3]) < -r)    return false;
    if ((pt_mv * m.neaClip).sum() < -r)    return false;
    if ((pt_mv * m.botClip).sum() < -r)    return false;
    if ((pt_mv * m.topClip).sum() < -r)    return false;
    if ((pt_mv * m.lftClip).sum() < -r)    return false;
    if ((pt_mv * m.rgtClip).sum() < -r)    return false;
    if ((pt_mv * m.farClip).sum() < -r)    return false;
    return true;
}

/// @brief Calculates the 2D location, which maps to the given 3D location (imagine placing a label at that position: it shall be close to the 3D plane drawn there)
/// @see `convert_to_2d` of original libxplanemp, of which this is an adapted copy
/// @author Ben Supnik, Chris Serio, and Chris Collins
void ConvertTo2d (const matrixesTy& m, std::valarray<float>& pt_mv, float& out_x, float& out_y)
{
    // The following is already pre-computed in 'pt_mv'
    // float xe = x * m.modelView[0] + y * m.modelView[4] + z * m.modelView[ 8] + w * m.modelView[12];
    // float ye = x * m.modelView[1] + y * m.modelView[5] + z * m.modelView[ 9] + w * m.modelView[13];
    // float ze = x * m.modelView[2] + y * m.modelView[6] + z * m.modelView[10] + w * m.modelView[14];
    // float we = x * m.modelView[3] + y * m.modelView[7] + z * m.modelView[11] + w * m.modelView[15];
    

    // float xc = xe * m.proj[0] + ye * m.proj[4] + ze * m.proj[ 8] + we * m.proj[12];
    // float yc = xe * m.proj[1] + ye * m.proj[5] + ze * m.proj[ 9] + we * m.proj[13];
    // float zc = xe * m.proj[2] + ye * m.proj[6] + ze * m.proj[10] + we * m.proj[14];
    // float wc = xe * m.proj[3] + ye * m.proj[7] + ze * m.proj[11] + we * m.proj[15];
    const float wc = (pt_mv * m.proj[std::slice(3,4,4)]).sum();
    const float xc = (pt_mv * m.proj[std::slice(0,4,4)]).sum() / wc;
    const float yc = (pt_mv * m.proj[std::slice(1,4,4)]).sum() / wc;
    //    float zc = (pt_mv * m.proj[std::slice(2,4,4)]).sum() / wc;

    out_x = float(m.viewport[0]) + (1.0f + xc) * float(m.viewport[2]) / 2.0f;
    out_y = float(m.viewport[1]) + (1.0f + yc) * float(m.viewport[3]) / 2.0f;
}

//
// MARK: Drawing Control
//

/// @brief Write the labels of all aircraft
/// @see This code bases on the last part of `XPMPDefaultPlaneRenderer` of the original libxplanemp
/// @author Ben Supnik, Chris Serio, Chris Collins, Birger Hoppe
void TwoDDrawLabels ()
{
    matrixesTy m;
    XPLMCameraPosition_t posCamera;

    // short-cut if label-writing is completely switched off
    if (!glob.bDrawLabels) return;
    
    // Determine the maximum distance for label drawing.
    // Depends on current actual visibility as well as a configurable maximum
    XPLMReadCameraPosition(&posCamera);
    const float maxLabelDist = std::min(glob.maxLabelDist,
                                        drVisibility ? XPLMGetDataf(drVisibility) : glob.maxLabelDist)
                               * posCamera.zoom;    // Labels get easier to see when users zooms.
    
    // determine x/y scaling, which changes with SSAA rendering settings
    float x_scale = 1.0f;
    float y_scale = 1.0f;
    if (drHDROnRef && XPLMGetDatai(drHDROnRef)) {   // SSAA hack only if HDR enabled
        if (drMSAAXRatio)
            x_scale = XPLMGetDataf(drMSAAXRatio);
        if (drMSAAYRatio)
            y_scale = XPLMGetDataf(drMSAAYRatio);
    }
    if (x_scale < 1.0f && y_scale < 1.0f) {
        x_scale = 1.0f;
        y_scale = 1.0f;
    }

    // Set up the OpenGL matrix for 2-D drawing
    SetupCullInfo(m);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, m.viewport[2], 0, m.viewport[3], -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    if (x_scale > 1.0f || y_scale > 1.0f)
        glScalef(x_scale, y_scale, 1.0f);
    
    // Loop over all aircraft and draw their labels
    for (const auto& p: glob.mapAc)
    {
        // skip if a/c is invisible
        const Aircraft& ac = *p.second;
        if (!ac.IsVisible())
            continue;
        
        // The aircraft's location
        const std::valarray<float> acPos = {ac.drawInfo.x, ac.drawInfo.y, ac.drawInfo.z, 1.0f};
        
        // This vector is used in all 3 calculations (DistToCamera, IsSphereVisible, ConvertTo2d)
        // float xp = x * m.modelView[0] + y * m.modelView[4] + z * m.modelView[ 8] + m.modelView[12];
        // float yp = x * m.modelView[1] + y * m.modelView[5] + z * m.modelView[ 9] + m.modelView[13];
        // float zp = x * m.modelView[2] + y * m.modelView[6] + z * m.modelView[10] + m.modelView[14];
        std::valarray<float> pt_mv = {
            (acPos * std::valarray<float>(m.modelView[std::slice(0, 4, 4)])).sum(),
            (acPos * std::valarray<float>(m.modelView[std::slice(1, 4, 4)])).sum(),
            (acPos * std::valarray<float>(m.modelView[std::slice(2, 4, 4)])).sum(),
            1.0f                // this is overwritten for the call to ConvertTo2d() only later
        };
        
        // Exit if aircraft is father away from camera than we would draw labels for
        if (ac.GetDistToCamera() > maxLabelDist)
            continue;
        
        // Calculate the heading from the camera to the target (hor, vert).
        // Calculate the angles between the camera angles and the real angles.
        // Cull (ie. skip) if we exceed half the FOV.
        if (!IsSphereVisible(m, pt_mv, 50.0f))
            continue;
        
        // Map the 3D coordinates of the aircraft to 2D coordinates of the flat screen
        float x = 0.0f, y = 0.0f;
        pt_mv[3] = (acPos * std::valarray<float>(m.modelView[std::slice(3, 4, 4)])).sum();
        ConvertTo2d(m, pt_mv, x, y);

        // Determine text color:
        // It stays as defined by application for half the way to maxLabelDist.
        // For the other half, it gradually fades to gray.
        // `rat` determines how much it faded already (factor from 0..1)
        const float rat = ac.GetDistToCamera() < maxLabelDist / 2.0f ? 0.0f :     // first half: no fading
            (ac.GetDistToCamera() - maxLabelDist/2.0f) / (maxLabelDist/2.0f);     // Second hald: fade to gray (remember: acDist <= maxLabelDist!)
        constexpr float gray[4] = {0.6f, 0.6f, 0.6f, 1.0f};
        float c[4] = {
            (1.0f-rat) * ac.colLabel[0] + rat * gray[0],     // red
            (1.0f-rat) * ac.colLabel[1] + rat * gray[1],     // green
            (1.0f-rat) * ac.colLabel[2] + rat * gray[2],     // blue
            (1.0f-rat) * ac.colLabel[3] + rat * gray[3]      // alpha? (not used for text anyway)
        };

        // Finally: Draw the label
        XPLMDrawString(c, int(x/x_scale), int(y/y_scale)+10,
                       (char*)ac.label.c_str(), NULL, xplmFont_Basic);

    }

    // Undo OpenGL matrix setup
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


/// Drawing callback, called by X-Plane in every drawing cycle
int CPLabelDrawing (XPLMDrawingPhase     /*inPhase*/,
                    int                  /*inIsBefore*/,
                    void *               /*inRefcon*/)
{
    const bool bShadow = XPLMGetDatai(drWorldRenderTy) != 0;
    const bool bBlend  = XPLMGetDatai(drPlaneRenderTy) == 2;
    
    // just do it...
    if (!bShadow && bBlend)
        TwoDDrawLabels();
    return 1;
}


/// Activate actual label drawing, esp. set up drawing callback
void TwoDActivate ()
{
    // Register the actual drawing func.
    if (!XPLMRegisterDrawCallback(CPLabelDrawing,
                                  xplm_Phase_Airplanes,
                                  0,                        // after
                                  nullptr))
        LOG_MSG(logERR, ERR_REGISTER_DRAW_CB);
}


/// Deactivate actual label drawing, esp. stop drawing callback
void TwoDDeactivate ()
{
    // Unregister the drawing callback
    XPLMUnregisterDrawCallback(CPLabelDrawing, xplm_Phase_Airplanes, 0, nullptr);
}


// Initialize the module
void TwoDInit ()
{
    // get all necessary dataRef handles (might return NULL if not available!)
    drModelviewMatrix   = XPLMFindDataRef("sim/graphics/view/modelview_matrix");
    drProjectionMatrix  = XPLMFindDataRef("sim/graphics/view/projection_matrix");
    drViewport          = XPLMFindDataRef("sim/graphics/view/viewport");
    drMSAAXRatio        = XPLMFindDataRef("sim/private/controls/hdr/fsaa_ratio_x");
    drMSAAYRatio        = XPLMFindDataRef("sim/private/controls/hdr/fsaa_ratio_y");
    drHDROnRef          = XPLMFindDataRef("sim/graphics/settings/HDR_on");
    drVisibility        = XPLMFindDataRef("sim/graphics/view/visibility_effective_m");
    if (!drVisibility)
        drVisibility    = XPLMFindDataRef("sim/weather/visibility_effective_m");
    drWorldRenderTy     = XPLMFindDataRef("sim/graphics/view/world_render_type");
    drPlaneRenderTy     = XPLMFindDataRef("sim/graphics/view/plane_render_type");

    // Register the drawing callback if need be
    if (glob.bDrawLabels)
        TwoDActivate();
}

// Grace cleanup
void TwoDCleanup ()
{
    // Remove drawing callbacks
    TwoDDeactivate();
}


}  // namespace XPMP2

//
// MARK: General API functions outside XPMP2 namespace
//

using namespace XPMP2;

// Enable/Disable/Query drawing of labels
void XPMPEnableAircraftLabels (bool _enable)
{
    // Only do anything if this actually is a change to prevent log spamming
    if (glob.bDrawLabels != _enable) {
        LOG_MSG(logDEBUG, DEBUG_ENABLE_AC_LABELS, _enable ? "enabled" : "disabled");
        glob.bDrawLabels = _enable;
        
        // Start/stop drawing as requested
        if (glob.bDrawLabels)
            TwoDActivate();
        else
            TwoDDeactivate();
    }
}

void XPMPDisableAircraftLabels()
{
    XPMPEnableAircraftLabels(false);
}

bool XPMPDrawingAircraftLabels()
{
    return glob.bDrawLabels;
}

