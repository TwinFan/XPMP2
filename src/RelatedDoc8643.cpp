/// @file       RelatedDoc8643.cpp
/// @brief      Handling the `related.txt` file for creating groups of similar looking aircraft types,
///             and Doc8643, the official list of ICAO aircraft type codes.
/// @details    A related group is declared simply by a line of ICAO a/c type codes read from the file.
///             Internally, the group is just identified by its line number in `related.txt`.
///             So the group "44" might be "A306 A30B A310", the Airbus A300 series.
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

namespace XPMP2 {

#define DEBUG_READ_RELATED      "related.txt: Trying to read from '%s'"
#define ERR_RELATED_NOT_FOUND   "related.txt: Could not open the file for reading"
#define WARN_DUP_RELATED_ENTRY  "related.txt: Duplicate entry for '%s' in line %d"

#define DEBUG_READ_DOC8643      "doc8643.txt: Reading from '%s'"
#define ERR_DOC8643_NOT_FOUND   "doc8643.txt: Could not open the file for reading"
#define ERR_DOC8643_READ_ERR    "doc8643.txt: Line %d did not match expectations: %s"

#define ERR_CFG_LINE_READ       "Error '%s' while reading line %d of %s"
#define ERR_CFG_FILE_TOOMANY    "Too many errors while trying to read file"

/// Maximum number of warnings during file read before bailing
constexpr int ERR_CFG_FILE_MAXWARN = 5;
/// Maximum length of OS error message
constexpr size_t SERR_LEN = 255;

//
// MARK: related.txt
//

// Read the `related.txt` file, full path passed in
const char* RelatedLoad (const std::string& _path)
{
    // No need to read more than once
    if (!glob.mapRelated.empty())
        return "";
    
    // Open the related.txt file
    LOG_MSG(logDEBUG, DEBUG_READ_RELATED, _path.c_str());
    std::ifstream fRelated (TOPOSIX(_path));
    if (!fRelated || !fRelated.is_open())
        return ERR_RELATED_NOT_FOUND;
    
    // read the file line by line and keep track of the line number as the internal id
    for (int lnNr = 1; fRelated; ++lnNr)
    {
        // read a line, trim it (remove whitespace at both ends)
        std::string ln;
        safeGetline(fRelated, ln);
        trim(ln);
        
        // skip over empty lines and over comments starting with a semicolon
        if (ln.empty() || ln[0] == ';')
            continue;
        
        // The remainder are expected to be arrays of ICAO type codes,
        std::vector<std::string> tokens = str_tokenize(ln, WHITESPACE);
        // which we just take as is and add them into the map
        // (no validation against Doc8643 here for good reasons.
        //  it increases flexibility if we allow to group non-official codes,
        //  e.g. one could group MD81 (non-official but offen mistakenly used)
        //  with MD80 (the officiel code) and both would be found)
        for (const std::string& icao: tokens) {
            // We warn about duplicate entries
            if (glob.logLvl <= logWARN) {
                const auto it = glob.mapRelated.find(icao);
                if (it != glob.mapRelated.cend()) {
                    LOG_MSG(logWARN, WARN_DUP_RELATED_ENTRY,
                            icao.c_str(), lnNr);
                }
            }
            // But we use all entries - may the last one win
            glob.mapRelated[icao] = lnNr;
        }
    }
    
    // Close the related.txt file
    fRelated.close();
    
    // Success
    return "";
}

// Find the related group for an ICAO a/c type, 0 if none
int RelatedGet (const std::string& _acType)
{
    try { return glob.mapRelated.at(_acType); }
    catch (const std::out_of_range&) { return 0; }
}

//
// MARK: Doc843.txt
//

const Doc8643 DOC8643_EMPTY;    // objet returned if no other matches

// constructor setting all elements
Doc8643::Doc8643 (const std::string& _classification,
                  const std::string& _wtc)
{
    strncpy(classification, _classification.c_str(), sizeof(classification));
    classification[sizeof(classification)-1] = 0;
    strncpy(wtc, _wtc.c_str(), sizeof(wtc));
    wtc[sizeof(wtc)-1] = 0;
}

// reads the Doc8643 file into mapDoc8643
const char* Doc8643Load (const std::string& _path)
{
    // must not read more than once!
    // CSLModels might already refer to these objects if already loaded.
    if (!glob.mapDoc8643.empty())
        return "";
    
    // open the file for reading
    std::ifstream fIn (TOPOSIX(_path));
    if (!fIn || !fIn.is_open())
        return ERR_DOC8643_NOT_FOUND;
    LOG_MSG(logDEBUG, DEBUG_READ_DOC8643, _path.c_str());

    // regular expression to extract individual values, separated by TABs
    enum { DOC_MANU=1, DOC_MODEL, DOC_TYPE, DOC_CLASS, DOC_WTC, DOC_EXPECTED };
    const std::regex re("^([^\\t]+)\\t"                   // manufacturer
                        "([^\\t]+)\\t"                    // model
                        "([[:alnum:]]{2,4})\\t"           // type designator
                        "(-|[AGHLST][C1-8][EJPRT])\\t"    // classification
                        "(-|[HLM]|L/M)");                 // wtc

    // loop over lines of the file
    std::string text;
    int errCnt = 0;
    for (int ln=1; fIn && errCnt <= ERR_CFG_FILE_MAXWARN; ln++) {
        // read entire line
        safeGetline(fIn, text);
        if (text.empty())           // skip empty lines silently
            continue;
        
        // apply the regex to extract values
        std::smatch m;
        std::regex_search(text, m, re);
        
        // add to map (if matched)
        if (m.size() == DOC_EXPECTED) {
            glob.mapDoc8643.emplace(m[DOC_TYPE],
                                    Doc8643(m[DOC_CLASS],
                                            m[DOC_WTC]));
        } else if (fIn) {
            // I/O was good, but line didn't match
            LOG_MSG(logWARN, ERR_DOC8643_READ_ERR, ln, text.c_str());
            errCnt++;
        } else if (!fIn && !fIn.eof()) {
            // I/O error
            char sErr[SERR_LEN];
            strerror_s(sErr, sizeof(sErr), errno);
            LOG_MSG(logERR, ERR_CFG_LINE_READ, sErr, ln, _path.c_str());
            errCnt++;
        }
    }
    
    // close file
    fIn.close();
    
    // too many warnings?
    if (errCnt > ERR_CFG_FILE_MAXWARN)
        return ERR_CFG_FILE_TOOMANY;
    
    // looks like success
    return "";
}

// return the matching Doc8643 object from the global map
const Doc8643& Doc8643Get (const std::string& _type)
{
    try { return glob.mapDoc8643.at(_type); }
    catch (const std::out_of_range&) { return DOC8643_EMPTY; }
}

// Is the given aircraft type a valid ICAO type as per Doc8643?
bool Doc8643IsTypeValid (const std::string& _type)
{
    return glob.mapDoc8643.count(_type) > 0;
}


}   // namespace XPMP2
