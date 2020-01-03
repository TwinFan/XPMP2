/// @file       XPMPMultiplayer.cpp
/// @brief      Initialization and general control functions for xplanemp2
/// @author     Birger Hoppe
/// @copyright  (c) 2018-2020 Birger Hoppe
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

#include "XPMPMultiplayer.h"



// MARK: Vertical offset stuff
// TODO: Probably to be moved elsewhere

void actualVertOffsetInfo(const char *inMtl, char *outType, double *outOffset)
{}

void setUserVertOffset(const char *inMtlCode, double inOffset)
{}

void removeUserVertOffset(const char *inMtlCode)
{}

// This routine initializes legacy portions of the multiplayer library
const char *    XPMPMultiplayerInitLegacyData(const char * inCSLFolder,
                                              const char * inRelatedPath,
                                              const char * inTexturePath,
                                              const char * inDoc8643,
                                              const char * inDefaltICAO,
                                              int (* inIntPrefsFunc)(const char *, const char *, int),
                                              float (* inFloatPrefsFunc)(const char *, const char *, float))
{
    return "";
}

// Init preference callback functions
// and storage location for user vertical offset config file
const char *    XPMPMultiplayerInit(int (* inIntPrefsFunc)(const char *, const char *, int),
                                    float (* inFloatPrefsFunc)(const char *, const char *, float),
                                    const char * resourceDir)
{
    return "";
}

// Undoes above init functions
void XPMPMultiplayerCleanup()
{}

// OBJ7 is not supported
const char * XPMPMultiplayerOBJ7SupportEnable(const char *)
{
    return "OBJ7 format is no longer supported";
}

//
// MARK: AI/Multiplayer control
//

// Acquire control of multiplayer aircraft
const char *    XPMPMultiplayerEnable()
{
    return "";
}

// Release control of multiplayer aircraft
void XPMPMultiplayerDisable()
{}

// Do we control AI planes?
bool XPMPHasControlOfAIAircraft()
{
    return false;
}

//
// MARK: CSL Package Handling
//

// Loads a collection of planes models
const char *    XPMPLoadCSLPackage(const char * inCSLFolder,
                                   const char * inRelatedPath,
                                   const char * inDoc8643)
{
    return "";
}

// checks what planes are loaded and loads any that we didn't get
void            XPMPLoadPlanesIfNecessary()
{}

// returns the number of found models
int XPMPGetNumberOfInstalledModels()
{
    return 0;
}

// return model info
void XPMPGetModelInfo(int inIndex, const char **outModelName, const char **outIcao, const char **outAirline, const char **outLivery)
{}

// test model match quality for given parameters
int         XPMPModelMatchQuality(
                                  const char *              inICAO,
                                  const char *              inAirline,
                                  const char *              inLivery)
{
    return -1;
}

// is ICAO a valid one according to our records?
bool            XPMPIsICAOValid(const char *                inICAO)
{
    return false;
}

//
// MARK: Create and Manage Planes
//

// Create a new plane
XPMPPlaneID XPMPCreatePlane(const char *            inICAOCode,
                            const char *            inAirline,
                            const char *            inLivery,
                            XPMPPlaneData_f         inDataFunc,
                            void *                  inRefcon)
{
    return nullptr;
}

// Create a new plane, providing a model
XPMPPlaneID XPMPCreatePlaneWithModelName(const char *           inModelName,
                                         const char *           inICAOCode,
                                         const char *           inAirline,
                                         const char *           inLivery,
                                         XPMPPlaneData_f            inDataFunc,
                                         void *                  inRefcon)
{
    return nullptr;
}

// Destroy a plane
void XPMPDestroyPlane(XPMPPlaneID)
{}

// Change a plane's model
int     XPMPChangePlaneModel(XPMPPlaneID                inPlaneID,
                             const char *           inICAOCode,
                             const char *           inAirline,
                             const char *           inLivery)
{
    return -1;
}

// return the name of the model in use
int     XPMPGetPlaneModelName(XPMPPlaneID             inPlaneID,
                              char *                  outTxtBuf,
                              int                     outTxtBufSize)
{
    return 0;
}

// return plane's ICAO / livery
void            XPMPGetPlaneICAOAndLivery(XPMPPlaneID               inPlane,
                                          char *                    outICAOCode,    // Can be NULL
                                          char *                    outLivery)      // Can be NULL
{}

// fetch plane data
XPMPPlaneCallbackResult XPMPGetPlaneData(
                                         XPMPPlaneID                    inPlane,
                                         XPMPPlaneDataType          inDataType,
                                         void *                     outData)
{
    return xpmpData_Unavailable;
}

// This function returns the quality level for the nominated plane's
int         XPMPGetPlaneModelQuality(XPMPPlaneID                inPlane)
{
    return -1;
}

// number of planes in existence
long            XPMPCountPlanes()
{
    return 0;
}

// return nth plane
XPMPPlaneID XPMPGetNthPlane(long index)
{
    return nullptr;
}


// Set default ICAO if model couldn't be derived at all
void    XPMPSetDefaultPlaneICAO(const char *            inICAO)
{}

//
// MARK: Plane Observation
//

/*
 * XPMPRegisterPlaneCreateDestroyFunc
 *
 * This function registers a notifier functionfor obeserving planes being created and destroyed.
 *
 */
void            XPMPRegisterPlaneNotifierFunc(XPMPPlaneNotifier_f       inFunc,
                                              void *                    inRefcon)
{}

/*
 * XPMPUnregisterPlaneCreateDestroyFunc
 *
 * This function canceles a registration for a notifier functionfor obeserving
 * planes being created and destroyed.
 */
void            XPMPUnregisterPlaneNotifierFunc(XPMPPlaneNotifier_f     inFunc,
                                                void *                  inRefcon)
{}

//
// MARK: PLANE RENDERING API
//

// This function setse the plane renderer.  You can pass NULL for the function to restore defaults.
void        XPMPSetPlaneRenderer(XPMPRenderPlanes_f         inRenderer,
                                 void *                         inRef)
{}

// Dump debug info to the error.out for one cycle
void        XPMPDumpOneCycle(void)
{}

// Enable/Disable/Query drawing of labels
void                  XPMPEnableAircraftLabels(void)
{}

void                  XPMPDisableAircraftLabels(void)
{}

bool                  XPMPDrawingAircraftLabels(void)
{
    return false;
}
