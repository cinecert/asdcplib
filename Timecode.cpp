/*
Copyright (c) 2005, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*! \file    Timecode.cpp
    \version $Id$
    \brief   Miscellaneous timecode functions
*/

#include <Timecode.h>
  
// constants for 30fps drop-frame timecode
const ui32_t DF_FRM_IN_1  = 1798UL;   // number of frames in 1 min of DF code
const ui32_t DF_FRM_IN_10 = 17982UL;  // number of frames in 10 mins of DF code
const ui32_t DF_FRM_HOUR  = 107892UL; // number of frames in 1 hour of DF code


// convert hh:mm:ss:ff at fps to frame count
ui32_t
ASDCP::tc_to_frames(ui16_t fps, ui16_t hh, ui16_t mm, ui16_t ss, ui16_t ff, bool df)
{
  if ( df )
    {
      if ( fps != 30 )
	{
	  DefaultLogSink().Error("Drop Frame Timecode is not supported at %hu fps\n", fps);
	  return 0;
	}

      return ( (hh * DF_FRM_HOUR)
	       + ((mm / 10) * DF_FRM_IN_10)
	       + ((mm % 10) * DF_FRM_IN_1)
	       + (ss * fps )
	       + ff );
    }
  else
    {
      return ((((((hh * 60UL) + mm) * 60UL) + ss ) * fps ) + ff);
    }
}


//
// end Timecode.cpp
//
