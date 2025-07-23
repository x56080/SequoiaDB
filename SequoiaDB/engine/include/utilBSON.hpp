/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilBSON.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_BSON_HPP__
#define UTIL_BSON_HPP__

#include "ossUtil.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bsontypes.h"

using namespace bson ;

namespace engine
{
   class _utilBSONRawBuilder : public utilPooledObject
   {
      public:
         _utilBSONRawBuilder() ;
         ~_utilBSONRawBuilder() ;

         // Note: This builder will not extend the buffer. So be sure to allocate enough buffer at
         //       the beginning, or the building will fail.
         INT32 start( CHAR *buffer, INT32 buffSize, const BSONObj *baseObj = NULL ) ;

         // nameLen is used to avoid scanning the name to get the length. Null teminating is not
         // included in nameLen.
         INT32 appendElement( BSONType type, const CHAR *name, INT32 nameLen,
                              const CHAR *value, INT32 valueLen ) ;

         INT32 done( BOOLEAN &isEmpty ) ;

         void reset() ;

         INT32 dataSize() const ;
         BOOLEAN isOutOfBuff() const ;

         OSS_INLINE static BOOLEAN  emptyValType( BSONType type ) ;
         OSS_INLINE static INT32    getEleValSize( CHAR type, const CHAR *value ) ;

      private:
         // Validate the generated record.
         BOOLEAN _validate() const ;

      private:
         CHAR     *_buffer ;
         CHAR     *_writePtr ;
         INT32    _buffSize ;
         INT32    _remainSize ;
         BOOLEAN  _done ;
         BOOLEAN  _isOutOfBuff ;
   } ;
   typedef _utilBSONRawBuilder utilBSONRawBuilder ;

   OSS_INLINE BOOLEAN _utilBSONRawBuilder::emptyValType( BSONType type )
   {
      switch ( (BSONType)type )
      {
         case EOO:
         case Undefined:
         case jstNULL:
         case MaxKey:
         case MinKey:
            return TRUE ;
         default:
            return FALSE ;
      }
   }

   OSS_INLINE INT32 _utilBSONRawBuilder::getEleValSize( CHAR type, const CHAR *value )
   {
      INT32 size = 0 ;

      switch ( (BSONType)type )
      {
        case EOO:
        case Undefined:
        case jstNULL:
        case MaxKey:
        case MinKey:
            break;
        case bson::Bool:
            size = 1;
            break;
        case NumberInt:
            size = 4;
            break;
        case Timestamp:
        case bson::Date:
        case NumberDouble:
        case NumberLong:
            size = 8;
            break;
        case NumberDecimal:
            size = *reinterpret_cast< const int *>( value ) ;
            break ;
        case jstOID:
            size = 12;
            break;
        case Symbol:
        case Code:
        case bson::String:
            size = *reinterpret_cast< const int *>( value ) + 4 ;   // 4 is for the size of the string
            break ;
        case DBRef:
            // 4 is for the size of the string, 12 is for the oid
            size = *reinterpret_cast< const int *>( value ) + 4 + 12 ;
            break ;
        case CodeWScope:
        case Object:
        case bson::Array:
            size = *reinterpret_cast< const int *>( value ) ;
            break ;
        case BinData:
            // BinData format: type|fieldName|binsize(4 bytes)|subtype(1 byte)|data
            size = *reinterpret_cast< const int *>( value ) + 4 + 1 ;
            break;
        case RegEx:
            {
               // RegEx format: type|fieldName|regexStr|optionStr
               const CHAR *p = value ;
               INT32 subLen = ossStrlen( p ) ;
               p = p + subLen + 1 ;
               size = subLen + 1 + ossStrlen( p ) + 1 ;
            }
            break ;
        default:
            size = -1 ;
            break ;
      }

      return size ;
   }
}

#endif /* UTIL_BSON_HPP__ */
