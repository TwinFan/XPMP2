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

#define WARN_ASYNC_LOAD_ONGOING "Async load operation ongoing while object is being destroyed for %s"
#define WARN_REF_CNT            "Reference counter is %u while object is being destroyed for %s"
#define DEBUG_OBJ_LOADING       "Async load starting  for %s / %s"
#define DEBUG_OBJ_LOADED        "Async load succeeded for %s / %s"
#define DEBUG_OBJ_UNLOADED      "Object %s / %s unloaded"
#define ERR_OBJ_OBJ_NOT_FOUND   "Async load for %s: CSLObj in CSLModel object not found!"
#define ERR_OBJ_NOT_FOUND       "Async load for %s: CSLModel object not found!"
#define ERR_OBJ_NOT_LOADED      "Async load FAILED for %s from %s"

/// The file holding package information
#define XSB_AIRCRAFT_TXT        "xsb_aircraft.txt"
#define INFO_XSBACTXT_READ      "Processing %s"
#define WARN_NO_XSBACTXT_FOUND  "No xsb_aircraft.txt found"
#define WARN_DUP_PKG_NAME       "Package name (EXPORT_NAME) '%s' in folder '%s' is already in use by '%s'"
#define WARN_PKG_DPDCY_FAILED   "Line %d: Dependent package '%s' not known! May lead to follow-up issues when proessing this package."
#define ERR_TOO_FEW_PARAM       "Line %d: %s expects at least %d parameters"
#define ERR_PKG_NAME_INVALID    "Line %d: Package path invalid: %s"
#define ERR_PKG_UNKNOWN         "Line %d: Package '%s' unknown in package path %s"
#define ERR_OBJ_FILE_NOT_FOUND  "Line %d: The file '%s' could not be found at %s"
#define WARN_IGNORED_COMMANDS   "Following commands ignored: "

//
// MARK: CSLModel Implementation
//

// Destructor makes sure the XP object is removed, too
CSLObj::~CSLObj ()
{
    Unload();
}

// Load and return the underlying X-Plane object.
XPLMObjectRef CSLObj::GetAndLoadObj ()
{
    // (Try) Loading, then return what we have, even if it is NULL
    Load();
    return xpObj;
}

// Starts loading the XP object asynchronously
/// @details    Async load takes time. Maybe this object exists no longer when the
///             load operaton returns. That's why we do not just pass `this`
///             to the XPLMLoadObjectAsync() call,
///             but a separate object, here a pair of copies of the id strings, by which we can try to
///             find the object again in the callback XPObjLoadedCB().
void CSLObj::Load ()
{
    // only if not already requested or even available
    if (GetObjState() != OLS_UNAVAIL)
        return;
    
    // Prepare to load the CSL model from the .obj file
    LOG_MSG(logDEBUG, DEBUG_OBJ_LOADING,
            cslId.c_str(), path.c_str());
    XPLMLoadObjectAsync(path.c_str(),               // path to .obj
                        &XPObjLoadedCB,             // static callback function
                        new pairOfStrTy(cslId.c_str(), // _copy_ of the id string
                                        path.c_str()));
    xpObjState = OLS_LOADING;
}

// Free up the object
void CSLObj::Unload ()
{
    if (xpObj) {
        XPLMUnloadObject(xpObj);
        xpObj = NULL;
        xpObjState = OLS_UNAVAIL;
        LOG_MSG(logDEBUG, DEBUG_OBJ_UNLOADED, cslId.c_str(), path.c_str());
    }
}

// Static: callback function called when loading is done
void CSLObj::XPObjLoadedCB (XPLMObjectRef inObject,
                            void *        inRefcon)
{
    // the refcon is a pointer to a pair object created just for us
    pairOfStrTy* p = reinterpret_cast<pairOfStrTy*>(inRefcon);
    
    // try finding the CSL model object in the global map
    try {
        CSLModel& csl = glob.mapCSLModels.at(p->first);
        
        // find the object by path
        listCSLObjTy::iterator iter = std::find_if(csl.listObj.begin(),
                                                   csl.listObj.end(),
                                                   [p](const CSLObj& o)
                                                   { return o.path == p->second; });
        
        // found?
        if (iter != csl.listObj.end()) {
            // Loading succeeded -> save the object and state
            if (inObject) {
                iter->xpObj      = inObject;
                iter->xpObjState = OLS_AVAILABLE;
                LOG_MSG(logDEBUG, DEBUG_OBJ_LOADED,
                        iter->cslId.c_str(), iter->path.c_str());
            }
            // Loading failed!
            else {
                iter->Invalidate();
            }
        } else {
            LOG_MSG(logERR, ERR_OBJ_OBJ_NOT_FOUND, p->first.c_str());
            XPLMUnloadObject(inObject);
        }
    }
    // CSL model not found in global map -> release the X-Plane object right away
    catch (const std::out_of_range&) {
        LOG_MSG(logERR, ERR_OBJ_NOT_FOUND, p->first.c_str());
        XPLMUnloadObject(inObject);
    }
    
    // free up the memory for the copy of the ids
    delete p;
}

// Marks the CSL Model invalid in case object loading once failed
void CSLObj::Invalidate ()
{
    xpObj       = NULL;
    xpObjState  = OLS_INVALID;
    LOG_MSG(logERR, ERR_OBJ_NOT_LOADED, cslId.c_str(), path.c_str());

    // TODO: Remove the CLSModel entirely, or at least from matching
}

//
// MARK: CSLModel Implementation
//

// Destructor frees resources
CSLModel::~CSLModel ()
{
    // last chance to unload the objects
    Unload();
    
    // Output a warning if there is still an async load operation going on
    if (GetObjState() == OLS_LOADING) {
        LOG_MSG(logWARN, WARN_ASYNC_LOAD_ONGOING, cslId.c_str());
    }
    // Output a warning if reference counter is not zero
    if (refCnt > 0) {
        LOG_MSG(logWARN, WARN_REF_CNT, refCnt, cslId.c_str());
    }
}

// (Minimum) )State of the X-Plane objects: Is it being loaded or available?
ObjLoadStateTy CSLModel::GetObjState () const
{
    // must have some objects...
    if (listObj.empty()) return OLS_INVALID;
    
    // Otherwise find the smallest status
    return
    std::min_element(listObj.cbegin(), listObj.cend(),
                     [](const CSLObj& a, const CSLObj& b)->bool
                     { return a.GetObjState() < b.GetObjState();})
    ->GetObjState();
}

// Try get ALL object handles, only returns anything if it is the complete list
std::list<XPLMObjectRef> CSLModel::GetAllObjRefs ()
{
    bool bFullyLoaded = true;
    std::list<XPLMObjectRef> l;
    for (CSLObj& obj: listObj)                  // loop over all object
    {
        if (obj.IsInvalid())                    // if any object is considered valid
            return std::list<XPLMObjectRef>();  // return an empty list
        
        l.push_back(obj.GetAndLoadObj());       // load and save the handle
        if (l.back() == NULL)                   // not (yet) loaded?
            bFullyLoaded = false;               // return an empty list in the end
    }
    
    // return the complete list of handles if all was successful
    return bFullyLoaded ? l : std::list<XPLMObjectRef>();
}

// Start loading all objects
void CSLModel::Load ()
{
    for (CSLObj& o: listObj)
        o.Load();
}

// Unload all objects
void CSLModel::Unload ()
{
    for (CSLObj& o: listObj)
        o.Unload();
}

//
// MARK: Internal Function Definitions
//

/// @brief Recursively scans folders to find `xsb_aircraft.txt` files of CSL packages
/// @param _path The path to start the search in
/// @param[Out] paths List of paths in which an xsb_aircraft.txt file has actually been found
/// @param _maxDepth How deep into the folder hierarchy shall we search? (defaults to 5)
const char* CSLModelsFindPkgs (const std::string& _path,
                               std::vector<std::string>& paths,
                               int _maxDepth = 5);

/// Scans an `xsb_aircraft.txt` file for `EXPORT_NAME` entries to fill the list of packages
const char* CSLModelsReadPkgId (const std::string& path);

/// Process the `xsb_aircraft.txt` file for reading OBJ8 models
const char* CSLModelsProcessAcFile (const std::string& path);

/// @brief Turn a relative package path into a full path
/// @details Turns a relative package path in `xsb_aircraft.txt` (like "__Bluebell_Airbus:A306/A306_AAW.obj")
///          into a full path pointing to a concrete file and verifies the file's existence.
/// @return Empty if any validation fails, otherwise a full path to an existing .obj file
std::string CSLModelsConvPackagePath (const std::string& pkgPath, int lnNr, bool bPkgOptional = false);

/// Process an DEPENDENCY line of an `xsb_aircraft.txt` file
void AcTxtLine_DEPENDENCY (std::vector<std::string>& tokens,
                           int lnNr);
/// Process an OBJ8_AIRCRAFT line of an `xsb_aircraft.txt` file
void AcTxtLine_OBJ8_AIRCRAFT (CSLModel& csl,
                              const std::string ln,
                              int lnNr);
/// Process an OBJ8 line of an `xsb_aircraft.txt` file
void AcTxtLine_OBJ8 (CSLModel& csl,
                     std::vector<std::string>& tokens,
                     int lnNr);
/// Process an VERT_OFFSET line of an `xsb_aircraft.txt` file
void AcTxtLine_VERT_OFFSET (CSLModel& csl,
                            std::vector<std::string>& tokens,
                            int lnNr);
/// Process an ICAO, AIRLINE, LIVERY, or MATCHES line of an `xsb_aircraft.txt` file
void AcTxtLine_MATCHES (CSLModel& csl,
                        std::vector<std::string>& tokens,
                        int lnNr);

//
// MARK: Interpret individual xsb_aircraft lines
//

// Process an DEPENDENCY line of an `xsb_aircraft.txt` file
void AcTxtLine_DEPENDENCY (std::vector<std::string>& tokens,
                           int lnNr)
{
    if (tokens.size() >= 2) {
        // We try finding the package and issue a warning if we didn't...but continue anyway
        if (glob.mapCSLPkgs.find(tokens[1]) == glob.mapCSLPkgs.cend()) {
            LOG_MSG(logWARN, WARN_PKG_DPDCY_FAILED, lnNr, tokens[1].c_str());
        }
    }
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 1);
}

// Process an OBJ8_AIRCRAFT line of an `xsb_aircraft.txt` file
void AcTxtLine_OBJ8_AIRCRAFT (CSLModel& csl,
                              const std::string ln,
                              int lnNr)
{
    // First of all, save the previously read aircraft
    if (csl.IsValid()) {
        glob.mapCSLModels.emplace(csl.GetId(), std::move(csl));
        csl = CSLModel();
    }
    
    // Second parameter (actually we take all the rest of the line) is the name
    if (ln.length() >= 15) { csl.cslId = ln.substr(14); trim(csl.cslId); }
    else LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, "OBJ8_AIRCRAFT", 1);
}

// Process an OBJ8 line of an `xsb_aircraft.txt` file
void AcTxtLine_OBJ8 (CSLModel& csl,
                     std::vector<std::string>& tokens,
                     int lnNr)
{
    if (tokens.size() >= 4) {
        // we only process the SOLID part
        if (tokens[1] == "SOLID") {
            // translate the path (replace the package with the full path)
            std::string path = CSLModelsConvPackagePath(tokens[3], lnNr);
            if (!path.empty()) {
                // save the path as an additional object to the model
                csl.listObj.emplace_back(csl.GetId(), std::move(path));

                // we can already read the TEXTURE and TEXTURE_LIT paths
                if (tokens.size() >= 5) {
                    CSLObj& obj = csl.listObj.back();
                    obj.texture = CSLModelsConvPackagePath(tokens[4], lnNr, true);
                    if (tokens.size() >= 6)
                        obj.text_lit = CSLModelsConvPackagePath(tokens[5], lnNr, true);
                } // TEXTURE available
            } // Package name valid
        } // "SOLID"
    } // at least 3 params
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 3);
}

// Process an VERT_OFFSET line of an `xsb_aircraft.txt` file
void AcTxtLine_VERT_OFFSET (CSLModel& csl,
                            std::vector<std::string>& tokens,
                            int lnNr)
{
    if (tokens.size() >= 2)
        csl.vertOfs = std::stof(tokens[1]);
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 1);
}

// Process an ICAO, AIRLINE, LIVERY, or MATCHES line of an `xsb_aircraft.txt` file
void AcTxtLine_MATCHES (CSLModel& csl,
                        std::vector<std::string>& tokens,
                        int lnNr)
{
    // at least one parameter, the ICAO type code, is expected
    if (tokens.size() >= 2) {
        csl.icaoType = tokens[1];
        // the others are optional and more descriptive
        if (tokens.size() >= 3) {
            csl.icaoAirline = tokens[2];
            if (tokens.size() >= 4)
                csl.livery = tokens[3];
        }
    }
    else
        LOG_MSG(logERR, ERR_TOO_FEW_PARAM, lnNr, tokens[0].c_str(), 1);
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
    // Clear out all packages
    glob.mapCSLPkgs.clear();
}


// Scans an `xsb_aircraft.txt` file for `EXPORT_NAME` entries to fill the list of packages
const char* CSLModelsReadPkgId (const std::string& path)
{
    // Open the xsb_aircraft.txt file
    const std::string xsbName (path + "/" XSB_AIRCRAFT_TXT);
    std::ifstream fAc (xsbName);
    if (!fAc || !fAc.is_open())
        return WARN_NO_XSBACTXT_FOUND;
    
    // read the file line by line
//    LOG_MSG(logINFO, INFO_XSBACTXT_READ, xsbName.c_str());
    while (fAc)
    {
        // read a line, trim it (remove whitespace at both ends)
        std::string ln;
        safeGetline(fAc, ln);
        trim(ln);
        
        // skip over empty lines
        if (ln.empty())
            continue;
        
        // We are only looking for EXPORT_NAME here
        std::vector<std::string> tokens = str_tokenize(ln, WHITESPACE);
        if (tokens.size() == 2 &&
            tokens[0] == "EXPORT_NAME")
        {
            // Found a package id -> save an entry for a package
            auto p = glob.mapCSLPkgs.insert(std::make_pair(tokens[1], path + "/"));
            if (!p.second) {                // not inserted, ie. package name existed already?
                LOG_MSG(logWARN, WARN_DUP_PKG_NAME,
                        tokens[1].c_str(), path.c_str(),
                        p.first->second.c_str());
            } else {
                LOG_MSG(logDEBUG, "%s: Added package '%s'",
                        path.c_str(), tokens[1].c_str());
            }
        }
    }
    
    // Close the xsb_aircraft.txt file
    fAc.close();
    
    // Success
    return "";
}


// Recursively scans folders to find `xsb_aircraft.txt` files of CSL packages
const char* CSLModelsFindPkgs (const std::string& _path,
                               std::vector<std::string>& paths,
                               int _maxDepth)
{
    // Search the current given path for an xsb_aircraft.txt file
    std::list<std::string> files = GetDirContents(_path);
    if (std::find(files.cbegin(), files.cend(), XSB_AIRCRAFT_TXT) != files.cend())
    {
        // Found a "xsb_aircraft.txt"! Let's process this path then!
        paths.push_back(_path);
        return CSLModelsReadPkgId(_path);
    }
    
    // Are we still allowed to dig deeper into the folder hierarchy?
    bool bFoundAnything = false;
    if (_maxDepth > 0) {
        // The let's see if there were some directories
        for (const std::string& f: files) {
            const std::string nextPath(_path + "/" + f);
            if (IsDir(nextPath)) {
                // recuresively call myself, allow one level of hierarchy less
                const char* res = CSLModelsFindPkgs(nextPath, paths, _maxDepth-1);
                // Not the message "nothing found"?
                if (strcmp(res, WARN_NO_XSBACTXT_FOUND) != 0) {
                    // if any other error: stop here and return that error
                    if (res[0])
                        return res;
                    // ...else: We did find and process something!
                    bFoundAnything = true;
                }
            }
        }
    }
    
    // Well...did we find anything?
    return bFoundAnything ? "" : WARN_NO_XSBACTXT_FOUND;
}


// Turn a relative package path into a full path
std::string CSLModelsConvPackagePath (const std::string& pkgPath,
                                      int lnNr,
                                      bool bPkgOptional)
{
    // find the first element, which shall be the package
    std::string::size_type pos = pkgPath.find_first_of(":/\\");
    if (pos == std::string::npos ||             // not found???
        pos == 0 ||                             // or the very first char?
        pos == pkgPath.length()-1)              // or the very last char only?
    {
        if (bPkgOptional)
            // that is OK if the package is optional
            return pkgPath;
        else
        {
            LOG_MSG(logERR, ERR_PKG_NAME_INVALID, lnNr, pkgPath.c_str());
            return "";
        }
    }
    
    // Let's try finding the full path for the package
    const std::string pkg(pkgPath.substr(0,pos));
    const auto pkgIter = glob.mapCSLPkgs.find(pkg);
    if (pkgIter == glob.mapCSLPkgs.cend()) {
        LOG_MSG(logERR, ERR_PKG_UNKNOWN, lnNr, pkg.c_str(), pkgPath.c_str());
        return "";
    }
    
    // Found the package, so now let's make a proper path, all with forward slashes
    std::string path = pkgIter->second + pkgPath.substr(pos+1);
    std::replace_if(path.begin(), path.end(),
                    [](char c){return c==':' || c=='\\';},
                    '/');
    
    // We do check here already if that target really exists
    if (!ExistsFile(path)) {
        LOG_MSG(logERR, ERR_OBJ_FILE_NOT_FOUND, lnNr,
                pkgPath.c_str(), path.c_str());
        return "";
    }
    
    // Success, return the converted path
    return path;
}

// Process the `xsb_aircraft.txt` file for reading OBJ8 models
const char* CSLModelsProcessAcFile (const std::string& path)
{
    // for a good but concise message about ignored elements we keep this list
    std::map<std::string, int> ignored;
    
    // Here we collect the next CSL model
    CSLModel csl;
    
    // Open the xsb_aircraft.txt file
    const std::string xsbName (path + "/" XSB_AIRCRAFT_TXT);
    std::ifstream fAc (xsbName);
    if (!fAc || !fAc.is_open())
        return WARN_NO_XSBACTXT_FOUND;
    
    // read the file line by line
    LOG_MSG(logINFO, INFO_XSBACTXT_READ, xsbName.c_str());
    for (int lnNr = 1; fAc; ++lnNr)
    {
        // read a line, trim it (remove whitespace at both ends)
        std::string ln;
        safeGetline(fAc, ln);
        trim(ln);
        
        // skip over empty lines
        if (ln.empty())
            continue;
        
        // Break up the line into individual parameters and work on its commands
        std::vector<std::string> tokens = str_tokenize(ln, WHITESPACE);
        if (tokens.empty()) continue;
        
        // OBJ8_AIRCRAFT: Start a new aircraft specification
        if (tokens[0] == "OBJ8_AIRCRAFT")
            AcTxtLine_OBJ8_AIRCRAFT(csl, ln, lnNr);
        
        // OBJ8: Define the object file to load
        else if (tokens[0] == "OBJ8")
            AcTxtLine_OBJ8(csl, tokens, lnNr);

        // VERT_OFFSET: Defines the vertical offset of the model, so the wheels correctly touch the ground
        else if (tokens[0] == "VERT_OFFSET")
            AcTxtLine_VERT_OFFSET(csl, tokens, lnNr);
        
        // The matching parameters will be processed the same way...
        else if (tokens[0] == "ICAO" ||
                 tokens[0] == "AIRLINE" ||
                 tokens[0] == "LIVERY" ||
                 tokens[0] == "MATCHES")
            AcTxtLine_MATCHES(csl, tokens, lnNr);

        // DEPENDENCY: We warn if we don't find the package but try anyway
        else if (tokens[0] == "DEPENDENCY")
            AcTxtLine_DEPENDENCY(tokens, lnNr);
        
        // EXPORT_NAME: Already processed, just skip
        else if (tokens[0] == "EXPORT_NAME")
            ;

        // else...we just ignore it but count the commands for a proper warning later
        else
            ignored[tokens[0]]++;
    }
    
    // Close the xsb_aircraft.txt file
    fAc.close();
    
    // Don't forget to also save the last object
    if (csl.IsValid()) {
        glob.mapCSLModels.emplace(csl.GetId(), std::move(csl));
        csl = CSLModel();
    }
    
    // If there were ignored commands we list them once now
    if (!ignored.empty()) {
        std::string msg(WARN_IGNORED_COMMANDS);
        char buf[25];
        for (const auto& i: ignored) {
            snprintf(buf, sizeof(buf), "%s (%dx), ",
                     i.first.c_str(), i.second);
            msg += buf;
        }
        msg.pop_back();                 // remove the last trailing ", "
        msg.pop_back();
        LOG_MSG(logWARN, msg.c_str());
    }

    // Success
    return "";
}


// Read the CSL Models found in the given path and below
const char* CSLModelsLoad (const std::string& _path,
                           int _maxDepth)
{
    // First we identify all package, so that (theoretically)
    // package dependencies to other packages can be resolved.
    // (This might rarely be used as OBJ8 only consists of one file,
    //  but the original xsb_aircraft.txt syntax requires it.)
    std::vector<std::string> paths;
    const char* res = CSLModelsFindPkgs(_path, paths, _maxDepth);
    
    // Now we can process each folder and read in the CSL models there
    for (const std::string& p: paths)
    {
        const char* r = CSLModelsProcessAcFile(p);
        if (r[0]) {                     // error?
            res = r;                    // keep it as function result (but continue with next path anyway)
            LOG_MSG(logWARN, res);      // also report it to the log
        }
    }
    
    // return the final result
    return res;
}


// Find a model by name
CSLModel* CSLModelByName (const std::string& _mdlName)
{
    // try finding the model by name
    try {
        CSLModel& mdl = glob.mapCSLModels.at(_mdlName);
        if (mdl.IsObjInvalid())            // don't return invalid models
            return nullptr;
        return &mdl;
    }
    catch (const std::out_of_range&) {
        // Model not found
        return nullptr;
    }
}

};
