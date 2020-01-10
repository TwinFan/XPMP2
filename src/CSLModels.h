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

/// Represetns a CSL model as it is saved on disk
class CSLModel
{
public:
    /// State of the X-Plane object: Is it being loaded or available?
    enum ObjLoadStateTy {
        OLS_UNAVAIL = 0,        ///< Not yet tried loading the CSL object
        OLS_LOADING,            ///< async load underway
        OLS_AVAILABLE,          ///< X-Plane object available in `xpObj`
        OLS_INVALID,            ///< loading once failed -> invalid!
    };

protected:
    /// id, just an arbitrary label read from `xsb_aircraft.txt::OBJ8_AIRCRAFT`
    std::string         cslId;
    /// ICAO aircraft type this model represents: `xsb_aircraft.txt::ICAO`
    std::string         icaoType;
    /// ICAO Airline code this model represents: `xsb_aircraft.txt::AIRLINE`
    std::string         icaoAirline;
    /// Livery code this model represents: `xsb_aircraft.txt::LIVERY`
    std::string         livery;
    
    /// Full path to the `.obj` file representing this model
    std::string         path;
    
    /// Vertical offset to be applied [m]
    float               vertOfs = 3.0f;
    
    /// The X-Plane object reference to the loaded model as requested with XPLMLoadObjectAsync
    XPLMObjectRef       xpObj = NULL;
    /// State of the X-Plane object: Is it being loaded or available?
    ObjLoadStateTy xpObjState = OLS_UNAVAIL;
    
public:
    /// Constructor
    CSLModel (const std::string& _id,
              const std::string& _type,
              const std::string& _airline,
              const std::string& _livery,
              const std::string& _path);
    
    /// Destructor frees resources
    virtual ~CSLModel ();
    
    const std::string& GetId () const           { return cslId; }
    const std::string& GetIcaoType () const     { return icaoType; }
    const std::string& GetIcaoAirline () const  { return icaoAirline; }
    const std::string& GetLivery () const       { return livery; }
    const std::string& GetPath () const         { return path; }
    
    /// Vertical Offset to be applied to aircraft model
    float GetVertOfs () const                   { return vertOfs; }
    
    /// @brief Load and return the underlying X-Plane object.
    /// @note Can return NULL while async load is underway!
    XPLMObjectRef GetAndLoadXPObj ();
    
    ObjLoadStateTy GetObjState () const         { return xpObjState; }
    bool IsInvalid () const                     { return xpObjState == OLS_INVALID; }
    
protected:
    /// Starts loading the XP object
    void XPObjLoad ();
    /// callback function called when loading is done
    static void XPObjLoadedCB (XPLMObjectRef inObject,
                               void *        inRefcon);
    /// Marks the CSL Model invalid in case object loading once failed
    void XPObjInvalidate ();
    /// Free up the object
    void XPObjUnload ();
    
};

/// Map of CSLModels (owning the object(
typedef std::map<std::string,CSLModel> mapCSLModelTy;

/// Map of pointers to CSLModels (for lookup by different sorting keys)
typedef std::map<std::string,CSLModel*> mapCSLModelPTy;

//
// MARK: Global Functions
//

/// Initialization
void CSLModelsInit ();

/// Grace cleanup
void CSLModelsCleanup ();

/// @brief Read the CSL Models from one given path
/// @param _path Path to a folder, which will be searched hierarchically for `xsb_aircraft.txt` files
/// @return An empty string on success, otherwise a human-readable error message
const char* CSLModelsLoad (const std::string& _path);

/// Find a model by name
CSLModel* CSLModelByName (const std::string& _mdlName);

};

#endif
