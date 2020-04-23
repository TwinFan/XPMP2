/// @file       XPMPMultiplayer.cpp
/// @brief      Initialization and general control functions for XPMP2
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

#include "XPMP2.h"

#include <cstring>

#define ERR_RSRC_DIR_INVALID  "Resource directory too short: %s"
#define ERR_RSRC_DIR_UNAVAIL  "Resource directory '%s' does not exist!"
#define ERR_RSRC_FILE_UNAVAIL "File '%s' is missing in resource directory '%s'"
#define INFO_DEFAULT_ICAO       "Default ICAO aircraft type now is %s"
#define INFO_LOAD_CSL_PACKAGE   "Loading CSL package from %s"

// The global functions implemented here are not in our namespace for legacy reasons,
// but we use our namespace a lot:
using namespace XPMP2;

//
// MARK: Initialization
//

namespace XPMP2 {

/// Validates existence of a given file and saves its full path location
static bool FindResFile (const char* fName, std::string& outPath)
{
    std::string path = glob.resourceDir + fName;
    if (ExistsFile(path)) {
        outPath = std::move(path);
        return true;
    } else {
        LOG_MSG(logFATAL, ERR_RSRC_FILE_UNAVAIL,
                fName, glob.resourceDir.c_str());
        return false;
    }
}

/// Validate all required files are available in the resource directory
const char* XPMPValidateResourceFiles (const char* resourceDir)
{
    // Store the resource dir
    if (!resourceDir || strlen(resourceDir) < 5) {
        LOG_MSG(logERR, ERR_RSRC_DIR_INVALID, resourceDir ? resourceDir : "<nullptr>");
        return "resourceDir too short / nullptr";
    }
    glob.resourceDir = TOPOSIX(resourceDir);
    
    // for path validation it must not end in the path separator
    if (glob.resourceDir.back() == PATH_DELIM_STD)
        glob.resourceDir.pop_back();    
    // Check for its existence
    if (!IsDir(glob.resourceDir)) {
        LOG_MSG(logERR, ERR_RSRC_DIR_UNAVAIL, glob.resourceDir.c_str());
        return "Resource directory unavailable";
    }
    // now add a separator to the path for ease of future use
    glob.resourceDir += PATH_DELIM_STD;
    
    // Validate and save a few files
    if (!FindResFile(RSRC_RELATED, glob.pathRelated))
        return "related.txt not found in resource directory";
    if (!FindResFile(RSRC_DOC8643, glob.pathDoc8643))
        return "Doc8643.txt not found in resource directory";
    if (!FindResFile(RSRC_MAP_ICONS, glob.pathMapIcons))
        return "MapIcons.png not found in resource directory";
    
    // NoPlane.acf is special
    // It must be available in resource directory as a basis
    if (!FindResFile(RSRC_NO_PLANE, glob.pathNoPlane))
        return "NoPlane.acf not found in resource directory";
    // But ideally we copy it to and open it from an Aircraft subdirectory
    char sysPath[512];
    XPLMGetSystemPath(sysPath);
    std::string pathAcf = TOPOSIX(sysPath) + RSRC_AIRCRAFT;
    if (!glob.pluginName.empty() && glob.pluginName != UNKNOWN_PLUGIN_NAME) {
        pathAcf += glob.pluginName;
        pathAcf += PATH_DELIM_STD;
    }

    // Shall we copy the file to an Aircraft target directory?
    if (glob.bCopyNoPlane) {
        // If necessary, copy a new file into the Aircraft/<plugin> folder
        if (!CopyFileIfNewer(glob.pathNoPlane, pathAcf)) {
            LOG_MSG(logWARN, "Could not access 'NoPlane.acf' in '%s' and could not copy it there either",
                pathAcf.c_str());
        }
    }

    // Try accessing NoPlane.acf in Aircraft/<plugin> subdirectory
    pathAcf += RSRC_NO_PLANE;
    if (ExistsFile(pathAcf)) {
        // Yea, found in there
        glob.pathNoPlane = pathAcf;
    }
    else
    {
        // So we are using NoPlane.acf from Resource directory.
        // That will work for a while, but as soon as user opens flight menu
        // X-Plane will forget some or all AI Aircraft
        LOG_MSG(logWARN, "NoPlane.acf is not in /Aircraft/... directory! This will likely lead to X-Plane forgetting AI Aircraft if user opens Flight Configuration!");
    }
    
    // This path we might need in HFS form
    glob.pathNoPlane = FROMPOSIX(glob.pathNoPlane);
    LOG_MSG(logINFO, "Using %s", glob.pathNoPlane.c_str());

    // Success
    return "";
}

};

// This routine initializes legacy portions of the multiplayer library
const char *    XPMPMultiplayerInitLegacyData(const char* inCSLFolder,
                                              const char* inPluginName,
                                              const char* resourceDir,
                                              XPMPIntPrefsFuncTy inIntPrefsFunc,
                                              const char* inDefaultICAO)
{
    // We just pass on the calls to the individual functions:
    
    // Internal init first
    const char* ret = XPMPMultiplayerInit (inPluginName,
                                           resourceDir,
                                           inIntPrefsFunc,
                                           inDefaultICAO);
    if (ret[0])                                     // failed?
        return ret;
    
    // Then try loading the first set of CSL models
    return XPMPLoadCSLPackage(inCSLFolder);
}

// Init preference callback functions
// and storage location for user vertical offset config file
const char *    XPMPMultiplayerInit(const char* inPluginName,
                                    const char* resourceDir,
                                    XPMPIntPrefsFuncTy inIntPrefsFunc,
                                    const char* inDefaultICAO)
{
    // Initialize random number generator
    std::srand(unsigned(std::time(nullptr)));
    
    // Store the pointers to the configuration callback functions
    glob.prefsFuncInt   = inIntPrefsFunc    ? inIntPrefsFunc    : PrefsFuncIntDefault;
    
    // Get the plugin's name and store it for later reference
    XPMPSetPluginName(inPluginName);
    if (glob.pluginName == UNKNOWN_PLUGIN_NAME) {
        char szPluginName[256];
        XPLMGetPluginInfo(XPLMGetMyID(), szPluginName, nullptr, nullptr, nullptr);
        glob.pluginName = szPluginName;
    }

    // Get X-Plane's version numbers
    glob.ReadVersions();    
    LOG_MSG(logINFO, "XPMP2 Initializing under X-Plane version %d/%s and XPLM version %d",
            glob.verXPlane, GetGraphicsDriverTxt(), glob.verXPLM);

    // And get initial config values (defines, e.g., log level, which we'll need soon)
    glob.UpdateCfgVals();

    // Look for all supplemental files
    const char* ret = XPMPValidateResourceFiles(resourceDir);
    if (ret[0]) return ret;
    
    // Define the default ICAO aircraft type
    XPMPSetDefaultPlaneICAO(inDefaultICAO);
    
    // Initialize all modules
    CSLModelsInit();
    AcInit();
    TwoDInit();
    AIMultiInit();
    MapInit();
    
    // Load related.txt
    ret = RelatedLoad(glob.pathRelated);
    if (ret[0]) return ret;

    // Load Doc8643.txt
    ret = Doc8643Load(glob.pathDoc8643);
    if (ret[0]) return ret;

    // Success
    return "";
}

// Overrides the plugin's name to be used in Log output
void XPMPSetPluginName (const char* inPluginName)
{
    if (inPluginName)
        glob.pluginName = inPluginName;
}

// Undoes above init functions
void XPMPMultiplayerCleanup()
{
    LOG_MSG(logINFO, "XPMP2 cleaning up...")

    // Cleanup all modules in revers order of initialization
    MapCleanup();
    AIMultiCleanup();
    TwoDCleanup();
    AcCleanup();
    CSLModelsCleanup();
}

// OBJ7 is not supported
const char * XPMPMultiplayerOBJ7SupportEnable(const char *)
{
    return "OBJ7 format is no longer supported";
}

//
// MARK: CSL Package Handling
//

// Loads a collection of planes models
const char *    XPMPLoadCSLPackage(const char * inCSLFolder)
{
    // Do load the CSL Models in the given path
    if (inCSLFolder) {
        LOG_MSG(logINFO, INFO_LOAD_CSL_PACKAGE, inCSLFolder);
        return CSLModelsLoad(inCSLFolder);
    }
    else
        return "<nullptr> provided";
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
#if IBM
#pragma warning(push)
    // I don't know why, but in this function, and only in this, MS warns about throwing an exception
#pragma warning(disable: 4297)
#endif
    LOG_ASSERT(iterMdl != glob.mapCSLModels.cend());
#if IBM
#pragma warning(pop)
#endif

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
    return Doc8643IsTypeValid(inICAO);
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
    try {
#ifndef __clang_analyzer__
        LegacyAircraft* pAc = new LegacyAircraft(inICAOCode,
                                                 inAirline,
                                                 inLivery,
                                                 inDataFunc,
                                                 inRefcon,
                                                 inModelName);
        // This is not leaking memory, the pointer is in glob.mapAc as taken care of by the constructor
        return pAc->GetPlaneID();
#endif
    }
    catch (const XPMP2Error&) {
        // This might be thrown in case of problems creating the object
        LOG_MSG(logERR, "Could not create plane object for %s/%s/%s/%s!",
                inICAOCode  ? inICAOCode    : "<null>",
                inAirline   ? inAirline     : "<null>",
                inLivery    ? inLivery      : "<null>",
                inModelName ? inModelName   : "<null>");
        return nullptr;
    }
}

// Destroy a plane by just deleting the object, the destructor takes care of the rest
void XPMPDestroyPlane(XPMPPlaneID _id)
{
    Aircraft* pAc = AcFindByID(_id);
    if (pAc)
        delete pAc;
}

// Show/Hide the aircraft temporarily without destroying the object
void XPMPSetPlaneVisibility(XPMPPlaneID _id, bool _bVisible)
{
    Aircraft* pAc = AcFindByID(_id);
    if (pAc)
        pAc->SetVisible(_bVisible);
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
                  observer) == glob.listObservers.end()) {
        glob.listObservers.emplace_back(std::move(observer));
        LOG_MSG(logDEBUG, "%lu observers registered",
                (unsigned long)glob.listObservers.size());
    }
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
    if (iter != glob.listObservers.cend()) {
        glob.listObservers.erase(iter);
        LOG_MSG(logDEBUG, "%lu observers registered",
                (unsigned long)glob.listObservers.size());
    }
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

