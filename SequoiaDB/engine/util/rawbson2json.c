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

   Source File Name = rawbson2json.c

   Descriptive Name = Raw BSON to JSON ( or CSV )

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rawbson2json.h"
#include "../client/jstobs.h"

// The caller pass the pointer for raw bson, and output buffer pointer and
// buffer length.
// the function returns true if bufferLen is good enough to hold data, otherwise
// it will return FALSE and the content in outputbuffer is not defined
BOOLEAN rawbson2json ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen )
{
   bson obj ;
   bson_init ( &obj ) ;
   bson_init_finished_data ( &obj, (char*)bsonObj ) ;
   return bsonToJson ( pOutputBuffer, bufferLen, &obj,
                       FALSE, FALSE ) ;
}

BOOLEAN rawbson2csv ( const CHAR *bsonObj,
                      CHAR *pOutputBuffer,
                      INT32 bufferLen )
{
   bson obj ;
   bson_init ( &obj ) ;
   bson_init_finished_data ( &obj, (char*)bsonObj ) ;
   return bsonToJson ( pOutputBuffer, bufferLen, &obj,
                       TRUE, FALSE ) ;
}
