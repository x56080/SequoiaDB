/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rawbson2json.h

   Descriptive Name = Raw BSON To JSON ( or csv ) Header

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/21/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RAWBSON2JSON_H__
#define RAWBSON2JSON_H__

#include "core.h"
SDB_EXTERN_C_START
BOOLEAN rawbson2json ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen ) ;
BOOLEAN rawbson2csv ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen ) ;
SDB_EXTERN_C_END
#endif
