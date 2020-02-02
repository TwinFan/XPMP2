/// @file       CSLModels.h
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

#ifndef _CSLModels_h_
#define _CSLModels_h_

namespace XPMP2 {

/// Map of CSLPackages: Maps an id to the base path (path ends on forward slash)
typedef std::map<std::string,std::string> mapCSLPackageTy;

/// State of the X-Plane object: Is it being loaded or available?
enum ObjLoadStateTy {
    OLS_INVALID = -1,       ///< loading once failed -> invalid!
    OLS_UNAVAIL = 0,        ///< Not yet tried loading the CSL object
    OLS_LOADING,            ///< async load underway
    OLS_AVAILABLE,          ///< X-Plane object available in `xpObj`
};

/// One `.obj` file of a CSL model (of which it can have multiple)
class CSLObj
{
public:
    std::string cslId;      ///< id of the CSL model this belongs to
    std::string path;       ///< full path to the `.obj` file
    std::string texture;    ///< texture file if defined, to be used in the TEXTURE line of the .obj file
    std::string text_lit;   ///< texture_lit file if defined, to be used in the TEXTURE_LIT line of the .obj file

protected:
    /// The X-Plane object reference to the loaded model as requested with XPLMLoadObjectAsync
    XPLMObjectRef       xpObj = NULL;
    /// State of the X-Plane object: Is it being loaded or available?
    ObjLoadStateTy xpObjState = OLS_UNAVAIL;

public:
    /// Constructor doesn't do much
    CSLObj (const std::string& _id,
            const std::string& _path) : cslId(_id), path(_path) {}
    /// Generate standard move constructor
    CSLObj (CSLObj&& o) = default;
    CSLObj& operator = (CSLObj&& o) = default;
    /// Destructor makes sure the XP object is removed, too
    ~CSLObj ();
    
    /// State of the X-Plane object: Is it being loaded or available?
    ObjLoadStateTy GetObjState () const         { return xpObjState; }
    /// Is invalid?
    bool IsInvalid () const                     { return xpObjState == OLS_INVALID; }

    /// @brief Load and return the underlying X-Plane objects.
    /// @note Can return NULL while async load is underway!
    XPLMObjectRef GetAndLoadObj ();
    /// Starts loading the XP object
    void Load ();
    /// Free up the object
    void Unload ();

protected:
    /// callback function called when loading is done
    static void XPObjLoadedCB (XPLMObjectRef inObject,
                               void *        inRefcon);
    /// Marks the CSL Model invalid in case object loading once failed
    void Invalidate ();
};

/// List of objects
typedef std::list<CSLObj> listCSLObjTy;

typedef std::pair<std::string,std::string> pairOfStrTy;

/// Represetns a CSL model as it is saved on disk
class CSLModel
{
public:
    /// id, just an arbitrary label read from `xsb_aircraft.txt::OBJ8_AIRCRAFT`
    std::string         cslId;
    /// ICAO Airline code this model represents: `xsb_aircraft.txt::AIRLINE`
    std::string         icaoAirline;
    /// Livery code this model represents: `xsb_aircraft.txt::LIVERY`
    std::string         livery;
    /// list of objects representing this model
    listCSLObjTy        listObj;
    /// Vertical offset to be applied [m]
    float               vertOfs = 3.0f;

protected:
    /// ICAO aircraft type this model represents: `xsb_aircraft.txt::ICAO`
    std::string         icaoType;
    /// Proper Doc8643 entry for this model
    const Doc8643*      doc8643 = nullptr;
    /// "related" group for this model (a group of alike plane models), or 0
    int                 related = 0;
    /// Reference counter: Number of Aircraft actively using this model
    unsigned refCnt = 0;
    
public:
    /// Constructor
    CSLModel () {}
    /// Generate standard move constructor
    CSLModel (CSLModel&& o) = default;
    CSLModel& operator = (CSLModel&& o) = default;

    /// Destructor frees resources
    virtual ~CSLModel ();
    
    /// Set the a/c type model, which also fills `doc8643` and `related`
    void SetIcaoType (const std::string& _type);
    
    /// Minimum requirement for using this object is: id, type, path
    bool IsValid () const { return !cslId.empty() && !icaoType.empty() && !listObj.empty(); }
    
    const std::string& GetId () const           { return cslId; }       ///< id, just an arbitrary label read from `xsb_aircraft.txt::OBJ8_AIRCRAFT`
    const std::string& GetIcaoType () const     { return icaoType; }    ///< ICAO aircraft type this model represents: `xsb_aircraft.txt::ICAO`
    const std::string& GetIcaoAirline () const  { return icaoAirline; } ///< ICAO Airline code this model represents: `xsb_aircraft.txt::AIRLINE`
    const std::string& GetLivery () const       { return livery; }      ///< Livery code this model represents: `xsb_aircraft.txt::LIVERY`
    int GetRelatedGrp () const                  { return related; }     ///< "related" group for this model (a group of alike plane models), or 0

    // Data from Doc8643
    const Doc8643& GetDoc8643 () const          { return *doc8643; }    ///< Classification (like "L2P" or "L4J") and WTC (like "H" or "L/M")
    const char* GetWTC () const                 { return doc8643->wtc; }///< Wake turbulence category
    char GetClassSize () const                  { return doc8643->classification[0]; }
    char GetClassNumEng () const                { return doc8643->classification[1]; }
    char GetClassEngType () const               { return doc8643->classification[2]; }

    /// Vertical Offset to be applied to aircraft model
    float GetVertOfs () const                   { return vertOfs; }
        
    /// (Minimum) )State of the X-Plane objects: Is it being loaded or available?
    ObjLoadStateTy GetObjState () const;
    /// (Minimum) )State of the X-Plane object: Is it invalid?
    bool IsObjInvalid () const                  { return GetObjState() == OLS_INVALID; }
    
    /// Try get ALL object handles, only returns anything if it is the complete list
    std::list<XPLMObjectRef> GetAllObjRefs ();
    
    /// Increase the reference counter for Aircraft usage
    void IncRefCnt () { ++refCnt; }
    /// Decrease the reference counter for Aircraft usage
    void DecRefCnt () { if (refCnt>0) --refCnt; }
    /// Current reference counter
    unsigned GetRefCnt () const { return refCnt; }

protected:
    /// Start loading all objects
    void Load ();
    /// Unload all objects
    void Unload ();
};

/// Map of CSLModels (owning the object)
typedef std::map<std::string,CSLModel> mapCSLModelTy;

/// Map of pointers to CSLModels (for lookup by different sorting keys)
typedef std::map<std::string,CSLModel*> mapCSLModelPTy;

/// List of pointers to CSLModels
typedef std::list<CSLModel*> listCSLModelPTy;

//
// MARK: Global Functions
//

/// Initialization
void CSLModelsInit ();

/// Grace cleanup
void CSLModelsCleanup ();

/// @brief Read the CSL Models found in the given path and below
/// @param _path Path to a folder, which will be searched hierarchically for `xsb_aircraft.txt` files
/// @param _maxDepth Search shall go how many folders deep at max?
/// @return An empty string on success, otherwise a human-readable error message
const char* CSLModelsLoad (const std::string& _path,
                           int _maxDepth = 5);

/// Find a model by name
CSLModel* CSLModelByName (const std::string& _mdlName);

/// @brief Find a matching model
/// @param _type ICAO aircraft type like "A319"
/// @param _airline ICAO airline code like "DLH"
/// @param _livery Any specific livery code, in LiveTraffic e.g. the tail number
/// @param[out] pModel Receives the pointer to the matching CSL model, or NULL if nothing found
/// @return The number of passes needed to find a match, the lower the better the quality
int CSLModelMatching (const std::string& _type,
                      const std::string& _airline,
                      const std::string& _livery,
                      CSLModel* &pModel);

}       // namespace XPMP2

#endif
