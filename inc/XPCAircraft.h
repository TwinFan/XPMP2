/// @file       XPCAircraft.h
/// @brief      XPCAircraft represents an aircraft as managed by xplanemp2
/// @note       This class bases on and is compatible to the XPCAircraft wrapper
///             class provided with the original libxplanemp.
///             In xplanemp2, however, this class is not a wrapper but the actual
///             means of managing aircraft. Hence, it includes a lot more members.
/// @author     Birger Hoppe
/// @copyright  The original XPCAircraft.h file in libxplanemp had no copyright note.
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

#ifndef _XPCAircraft_h_
#define _XPCAircraft_h_

#include "XPMPMultiplayer.h"

class	XPCAircraft {
public:
	
	XPCAircraft(
			const char *			inICAOCode,
			const char *			inAirline,
			const char *			inLivery);
	virtual							~XPCAircraft();
	
	virtual	XPMPPlaneCallbackResult	GetPlanePosition(
			XPMPPlanePosition_t *	outPosition)=0;

	virtual	XPMPPlaneCallbackResult	GetPlaneSurfaces(
			XPMPPlaneSurfaces_t *	outSurfaces)=0;

	virtual	XPMPPlaneCallbackResult	GetPlaneRadar(
			XPMPPlaneRadar_t *	outRadar)=0;

    // overriding info text callback is optional
    virtual XPMPPlaneCallbackResult GetInfoTexts(
            XPMPInfoTexts_t *   /*outInfoTexts*/)
    {  return xpmpData_Unavailable; }
protected:

	XPMPPlaneID			mPlane;

	static	XPMPPlaneCallbackResult	AircraftCB(
			XPMPPlaneID			inPlane,
			XPMPPlaneDataType	inDataType,
			void *				ioData,
			void *				inRefcon);

};	

#endif
