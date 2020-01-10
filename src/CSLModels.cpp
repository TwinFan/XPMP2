/// @file       CSLModels.cpp
/// @brief      Managing CSL Models, including matching and loading
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

namespace XPMP2 {

#define WARN_ASYNC_LOAD_ONGOING "Async load operation ongoing while object is destroyed for %s"
#define DEBUG_OBJ_LOADING       "Async load starting for %s from %s"
#define DEBUG_OBJ_LOADED        "Async load succeeded for %s"
#define DEBUG_OBJ_UNLOADED      "Object %s unloaded"
#define ERR_OBJ_NOT_FOUND       "Async load for %s: CSLModel object not found!"
#define ERR_OBJ_NOT_LOADED      "Async load FAILED for %s from %s"

//
// MARK: CSLModel Implementation
//

// Constructor
CSLModel::CSLModel (const std::string& _id,
                    const std::string& _type,
                    const std::string& _airline,
                    const std::string& _livery,
                    const std::string& _path) :
cslId(_id),
icaoType(_type),
icaoAirline(_airline),
livery(_livery),
path(_path)
{}

// Destructor frees resources
CSLModel::~CSLModel ()
{
    // last change to unload the model
    XPObjUnload();
    
    // Output a warning if there is still an async load operation going on
    // This is bound to crash as the CSLModel object is going invalid
    if (GetObjState() == OLS_LOADING) {
        LOG_MSG(logWARN, WARN_ASYNC_LOAD_ONGOING, cslId.c_str());
    }
}

//
// MARK: X-Plane Object
//

// Load and return the underlying X-Plane object.
XPLMObjectRef CSLModel::GetAndLoadXPObj ()
{
    // if not yet tried to load, then initiate loading
    if (GetObjState() == OLS_UNAVAIL)
        XPObjLoad();
    
    // return what we have, even if it is NULL
    return xpObj;
}

// Starts loading the XP object asynchronously
/// @details    Async load takes time. Maybe this object exists no longer when the
///             load operaton returns. That's why we do not just pass `this`
///             to the XPLMLoadObjectAsync() call,
///             but a separate object, here a copy of the id string, by which we can try to
///             find the object again in the callback XPObjLoadedCB().
void CSLModel::XPObjLoad ()
{
    // only if not already requested or even available
    if (GetObjState() != OLS_UNAVAIL)
        return;
    
    // Prepare to load the CSL model from the .obj file
    LOG_MSG(logDEBUG, DEBUG_OBJ_LOADING,
            cslId.c_str(), path.c_str());
    XPLMLoadObjectAsync(path.c_str(),               // path to .obj
                        &XPObjLoadedCB,             // static callback function
                        new std::string(cslId));    // _copy_ of the id string
    xpObjState = OLS_LOADING;
}

// Static: callback function called when loading is done
void CSLModel::XPObjLoadedCB (XPLMObjectRef inObject,
                              void *        inRefcon)
{
    // the refcon is a pointer to a std::string object created just for us
    std::string* pCslId = reinterpret_cast<std::string*>(inRefcon);
    
    // try finding the CSL model object in the global map
    try {
        CSLModel& csl = glob.mapCSLModels.at(*pCslId);
        
        // Loading succeeded -> save the object and state
        if (inObject) {
            csl.xpObj       = inObject;
            csl.xpObjState  = OLS_AVAILABLE;
            LOG_MSG(logDEBUG, DEBUG_OBJ_LOADED, pCslId->c_str());
        }
        // Loading failed!
        else {
            csl.XPObjInvalidate();
        }

    }
    // CSL model not found in global map -> release the X-Plane object right away
    catch (const std::out_of_range&) {
        LOG_MSG(logERR, ERR_OBJ_NOT_FOUND, pCslId->c_str());
        XPLMUnloadObject(inObject);
    }
    
    // free up the memory for the copy of the id
    delete pCslId;
}

// Marks the CSL Model invalid in case object loading once failed
void CSLModel::XPObjInvalidate ()
{
    xpObj       = NULL;
    xpObjState  = OLS_INVALID;
    LOG_MSG(logERR, ERR_OBJ_NOT_LOADED, cslId.c_str(), path.c_str());

    // TODO: Remove the CLSModel entirely, or at least from matching
}


// Free up the object
void CSLModel::XPObjUnload ()
{
    if (xpObj) {
        XPLMUnloadObject(xpObj);
        xpObj = NULL;
        xpObjState = OLS_UNAVAIL;
        LOG_MSG(logDEBUG, DEBUG_OBJ_UNLOADED, cslId.c_str());
    }
}

//
// MARK: Global Functions
//

// Initialization
void CSLModelsInit ()
{}

// Grace cleanup
void CSLModelsCleanup ()
{
    // Clear out all model objects, will in turn unload all X-Plane objects
    glob.mapCSLModels.clear();
}

// Read the CSL Models from one given path
const char* CSLModelsLoad (const std::string& _path)
{
    // TODO: Implement
    
    // For now: One constant object to a known object
    CSLModel mdl ("A320_DLH",
                  "A320",
                  "DLH",
                  "",
                  "/Users/birger/Applications/X-Plane 11/Resources/plugins/LiveTraffic/Resources/CSL/BB_Airbus/A320/A320_DLH.obj");
    glob.mapCSLModels.emplace("A320_DLH", std::move(mdl));
    
    // success
    return "";
}

// Find a model by name
CSLModel* CSLModelByName (const std::string& _mdlName)
{
    // try finding the model by name
    try {
        CSLModel& mdl = glob.mapCSLModels.at(_mdlName);
        if (mdl.IsInvalid())            // don't return invalid models
            return nullptr;
        return &mdl;
    }
    catch (const std::out_of_range&) {
        // Model not found
        return nullptr;
    }
}

};
