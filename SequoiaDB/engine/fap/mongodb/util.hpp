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

   Source File Name = util.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MSG_CONVERTER_UTIL_HPP_
#define _SDB_MSG_CONVERTER_UTIL_HPP_

#include "oss.hpp"
#include "iostream"
#include "msgBuffer.hpp"
#include "../../bson/bson.hpp"

class _baseConverter : public SDBObject
{
public:
   _baseConverter() : _msglen( 0 ), _msgdata( NULL )
   {}

   virtual ~_baseConverter()
   {
      if ( NULL != _msgdata )
      {
         _msgdata = NULL ;
      }
   }

   void loadFrom( CHAR *msg, const INT32 len )
   {
      if ( NULL == msg )
      {
         return ;
      }

      _msgdata = msg ;
      _msglen  = len ;
   }

   virtual INT32 convert( msgBuffer &out )
   {
      return SDB_OK ;
   }

//    virtual INT32 reConvert( msgBuffer &out, const CHAR *cmdName )
//    {
//       return SDB_OK ;
//    }

protected:
   INT32   _msglen ;
   CHAR   *_msgdata ;
} ;
typedef _baseConverter baseConverter ;

///< check big endian machine
inline BOOLEAN checkBigEndian()
{
   BOOLEAN bigEndian = FALSE ;
   union
   {
      unsigned int i ;
      unsigned char s[4] ;
   } c ;

   c.i = 0x12345678 ;
   if ( 0x12 == c.s[0] )
   {
      bigEndian = TRUE ;
   }

   return bigEndian ;
}

#endif
