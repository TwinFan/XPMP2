/// @file       XPMPMultiplayer.cpp
/// @brief      Initialization and general control functions for xplanemp2
/// @author     Birger Hoppe
/// @copyright  (c) 2020 Birger Hoppe
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

#include "xplanemp2.h"

#include <cstring>

#define INFO_DEFAULT_ICAO       "Default ICAO aircraft type now is %s"
#define INFO_LOAD_CSL_PACKAGE   "Loading CSL package from %s"

#define DEBUG_ENABLE_AC_LABELS  "Aircraft labels %s"

// The global functions implemented here are not in our namespace for legacy reasons,
// but we use our namespace a lot:
using namespace XPMP2;

// MARK: Vertical offset stuff
// TODO: Probably to be moved elsewhere

/*
void actualVertOffsetInfo(const char *inMtl, char *outType, double *outOffset)
{}

void setUserVertOffset(const char *inMtlCode, double inOffset)
{}

void removeUserVertOffset(const char *inMtlCode)
{}
 */

//
// MARK: Initialization
//

// This routine initializes legacy portions of the multiplayer library
const char *    XPMPMultiplayerInitLegacyData(const char * inCSLFolder,
                                              const char * inRelatedPath,
                                              const char * /*inTexturePath*/,
                                              const char * inDoc8643,
                                              const char * inDefaultICAO,
                                              int (* inIntPrefsFunc)(const char *, const char *, int),
                                              float (* inFloatPrefsFunc)(const char *, const char *, float))
{
    // We just pass on the calls to the individual functions:
    
    // Internal init first
    const char* ret = XPMPMultiplayerInit (inIntPrefsFunc,
                                           inFloatPrefsFunc,
                                           NULL);
    if (ret[0])                                     // failed?
        return ret;
    
    // Then try loading the first set of CSL models
    ret = XPMPLoadCSLPackage(inCSLFolder,
                             inRelatedPath,
                             inDoc8643);
    if (ret[0])                                     // failed?
        return ret;
    
    // Define the default ICAO aircraft type
    XPMPSetDefaultPlaneICAO(inDefaultICAO);
    
    // Success
    return "";
}

// Init preference callback functions
// and storage location for user vertical offset config file
const char *    XPMPMultiplayerInit(int (* inIntPrefsFunc)(const char *, const char *, int),
                                    float (* inFloatPrefsFunc)(const char *, const char *, float),
                                    const char * resourceDir)
{
    LOG_MSG(logINFO, "XPMP2 Initializing...")
    
    // Store the pointers to the configuration callback functions
    glob.prefsFuncInt   = inIntPrefsFunc    ? inIntPrefsFunc    : PrefsFuncIntDefault;
    glob.prefsFuncFloat = inFloatPrefsFunc  ? inFloatPrefsFunc  : PrefsFuncFloatDefault;
    glob.UpdateCfgVals();
    
    // Save the resource dir, if it exists
    if (resourceDir)
    {
        if (ExistsFile(resourceDir)) {
            glob.resourceDir = resourceDir;
        } else {
            LOG_MSG(logWARN, WARN_RSRC_DIR_UNAVAIL, resourceDir);
        }
    }
    
    // Initialize all modules
    CSLModelsInit();
    AcInit();
    
    return "";
}

// Undoes above init functions
void XPMPMultiplayerCleanup()
{
    LOG_MSG(logINFO, "XPMP2 cleaning up...")

    // Cleanup all modules in revers order of initialization
    AcCleanup();
    CSLModelsCleanup();
}

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
    const char* ret = "";
    
    // If given, load the `related.txt` file
    if (inRelatedPath)
        ret = RelatedLoad(inRelatedPath);
    
    // If given, load the `doc8643.txt` file
    if (inDoc8643) {
        const char* r = Doc8643Load(inDoc8643);
        if (r[0]) ret = r;
    }
    
    // Do load the CSL Models in the given path
    if (inCSLFolder) {
        LOG_MSG(logINFO, INFO_LOAD_CSL_PACKAGE, inCSLFolder);
        const char* r = CSLModelsLoad(inCSLFolder);
        if (r[0]) ret = r;
    }
    
    return ret;
}

// checks what planes are loaded and loads any that we didn't get
void            XPMPLoadPlanesIfNecessary()
{}

// returns the number of found models
int XPMPGetNumberOfInstalledModels()
{
    return (int)glob.mapCSLModels.size();
}

// return model info
void XPMPGetModelInfo(int inIndex, const char **outModelName, const char **outIcao, const char **outAirline, const char **outLivery)
{
    // sanity check: index to high?
    if (inIndex >= XPMPGetNumberOfInstalledModels()) {
        LOG_MSG(logDEBUG, "inIndex %d too high, have only %d models",
                inIndex, (int)XPMPGetNumberOfInstalledModels());
        return;
    }
    
    // get the inIndex-th model
    mapCSLModelTy::const_iterator iterMdl = glob.mapCSLModels.cbegin();
    std::advance(iterMdl, inIndex);
    LOG_ASSERT(iterMdl != glob.mapCSLModels.cend());
    
    // Copy string pointers back. We just pass back pointers into our CSL Model object
    // as we can assume that the CSL Model object exists quite long.
    if (outModelName)   *outModelName   = iterMdl->second.GetId().data();
    if (outIcao)        *outIcao        = iterMdl->second.GetIcaoType().data();
    if (outAirline)     *outAirline     = iterMdl->second.GetIcaoAirline().data();
    if (outLivery)      *outLivery      = iterMdl->second.GetLivery().data();
}

// test model match quality for given parameters
int         XPMPModelMatchQuality(const char *              inICAO,
                                  const char *              inAirline,
                                  const char *              inLivery)
{
    CSLModel* pModel;
    return CSLModelMatching(inICAO      ? inICAO : "",
                            inAirline   ? inAirline : "",
                            inLivery    ? inLivery : "",
                            pModel);
}

// is ICAO a valid one according to our records?
bool            XPMPIsICAOValid(const char *                inICAO)
{
    if (!inICAO) return false;
    const Doc8643& doc8643 = Doc8643Get(inICAO);
    return !doc8643.empty();
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
    // forward to XPMPCreatePlaneWithModelName with no model name
    return XPMPCreatePlaneWithModelName(nullptr,        // no model name
                                        inICAOCode,
                                        inAirline,
                                        inLivery,
                                        inDataFunc,
                                        inRefcon);
}

// Create a new plane, providing a model
XPMPPlaneID XPMPCreatePlaneWithModelName(const char *       inModelName,
                                         const char *       inICAOCode,
                                         const char *       inAirline,
                                         const char *       inLivery,
                                         XPMPPlaneData_f    inDataFunc,
                                         void *             inRefcon)
{
    LegacyAircraft* pAc = new LegacyAircraft(inICAOCode,
                                             inAirline,
                                             inLivery,
                                             inDataFunc,
                                             inRefcon,
                                             inModelName);
    // This is not leaking memory, the pointer is in glob.mapAc as taken care of by the constructor
    return pAc->GetPlaneID();
}

// Destroy a plane by just deleting the object, the destructor takes care of the rest
void XPMPDestroyPlane(XPMPPlaneID _id)
{
    Aircraft* pAc = AcFindByID(_id);
    if (pAc)
        delete pAc;
}

// Change a plane's model
int     XPMPChangePlaneModel(XPMPPlaneID            _id,
                             const char *           inICAO,
                             const char *           inAirline,
                             const char *           inLivery)
{
    Aircraft* pAc = AcFindByID(_id);
    if (pAc)
        return pAc->ChangeModel(inICAO      ? inICAO : "",
                                inAirline   ? inAirline : "",
                                inLivery    ? inLivery : "");
    else
        return -1;
}

// return the name of the model in use
int     XPMPGetPlaneModelName(XPMPPlaneID             inPlaneID,
                              char *                  outTxtBuf,
                              int                     outTxtBufSize)
{
    Aircraft* pAc = AcFindByID(inPlaneID);
    if (pAc) {
        std::string mdlName = pAc->GetModelName();
        if (outTxtBuf && outTxtBufSize > 0) {
            strncpy(outTxtBuf, mdlName.c_str(), (size_t)outTxtBufSize);
            outTxtBuf[outTxtBufSize-1] = 0;         // safety measure: ensure zero-termination
        }
        return (int)mdlName.length();
    } else {
        return -1;
    }
}

// return plane's ICAO / livery
void XPMPGetPlaneICAOAndLivery(XPMPPlaneID inPlane,
                               char *      outICAOCode,    // Can be NULL
                               char *      outLivery)      // Can be NULL
{
    Aircraft* pAc = AcFindByID(inPlane);
    if (pAc) {
        // this is not a safe operation...but the way the legay interface was defined:
        if (outICAOCode) strcpy (outICAOCode, pAc->acIcaoType.c_str());
        if (outLivery)   strcpy (outLivery,   pAc->acLivery.c_str());
    }
}

// fetch plane data (unsupported in XPMP2)
XPMPPlaneCallbackResult XPMPGetPlaneData(XPMPPlaneID,
                                         XPMPPlaneDataType,
                                         void *)
{
    LOG_MSG(logERR, "Calling this function from the outside should not be needed!");
    return xpmpData_Unavailable;
}

// This function returns the quality level for the nominated plane's
int         XPMPGetPlaneModelQuality(XPMPPlaneID _inPlane)
{
    Aircraft* pAc = AcFindByID(_inPlane);
    if (pAc)
        return pAc->GetMatchQuality();
    else
        return -1;
}

// number of planes in existence
long XPMPCountPlanes()
{
    return (long)glob.mapAc.size();
}

// return nth plane
XPMPPlaneID XPMPGetNthPlane(long index)
{
    if (index < 0 || index >= XPMPCountPlanes())
        return nullptr;
    auto iter = glob.mapAc.cbegin();
    std::advance(iter, index);
    return iter->second->GetPlaneID();
}


// Set default ICAO if model couldn't be derived at all
void    XPMPSetDefaultPlaneICAO(const char * inICAO)
{
    if (!inICAO) return;
    
    // TODO: Verify against DOC8643
    glob.defaultICAO = inICAO;
    LOG_MSG(logINFO, INFO_DEFAULT_ICAO, inICAO);
}

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
{
    // Avoid duplicate entries
    XPMPPlaneNotifierTy observer (inFunc, inRefcon);
    if (std::find(glob.listObservers.begin(),
                  glob.listObservers.end(),
                  observer) == glob.listObservers.end())
        glob.listObservers.emplace_back(std::move(observer));
}

/*
 * XPMPUnregisterPlaneCreateDestroyFunc
 *
 * This function canceles a registration for a notifier functionfor obeserving
 * planes being created and destroyed.
 */
void            XPMPUnregisterPlaneNotifierFunc(XPMPPlaneNotifier_f     inFunc,
                                                void *                  inRefcon)
{
    auto iter = std::find(glob.listObservers.begin(),
                          glob.listObservers.end(),
                          XPMPPlaneNotifierTy (inFunc, inRefcon));
    if (iter != glob.listObservers.cend())
        glob.listObservers.erase(iter);
}

namespace XPMP2 {
// Send a notification to all observers
void XPMPSendNotification (const Aircraft& plane, XPMPPlaneNotification _notification)
{
    for (const XPMPPlaneNotifierTy& n: glob.listObservers)
        n.func(plane.GetPlaneID(),
               _notification,
               n.refcon);
}
}


//
// MARK: PLANE RENDERING API
//

// This function setse the plane renderer.  You can pass NULL for the function to restore defaults.
void        XPMPSetPlaneRenderer(XPMPRenderPlanes_f, void *)
{
    LOG_MSG(logERR, "XPMPSetPlaneRenderer() is NOT SUPPORTED in XPMP2");
}

// Dump debug info to the error.out for one cycle
void        XPMPDumpOneCycle(void)
{}

// Enable/Disable/Query drawing of labels
void XPMPEnableAircraftLabels (bool _enable)
{
    // Only do anything if this actually is a change to prevent log spamming
    if (glob.bDrawLabels != _enable) {
        LOG_MSG(logDEBUG, DEBUG_ENABLE_AC_LABELS, _enable ? "enabled" : "disabled");
        glob.bDrawLabels = _enable;
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
