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

   Source File Name = qgmHashTable.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef qgmHASHTABLE_HPP_
#define qgmHASHTABLE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{
typedef  INT64 QGM_HT_CONTEXT ;
#define QGM_HT_INVALID_CONTEXT -1

   class _qgmHashTable : public SDBObject
   {
   public:
      _qgmHashTable() ;
      virtual ~_qgmHashTable() ;

   public:
      INT32 init( UINT64 bufSize ) ;

      INT32 push( const CHAR *fieldName,
                  const BSONObj &value ) ;

      INT32 find( const BSONElement &key,
                  QGM_HT_CONTEXT &context ) ;

      INT32 getMore( const BSONElement &key,
                     QGM_HT_CONTEXT &context,
                     BSONObj &value ) ;

      void clear() ;

      /// release data and mem.
      void release() ;
   private:

#pragma pack(1)
      struct hashTuple
      {
         // BSONElement start
         const CHAR *data ;
         INT32 fieldNameSize ;
         INT32 totalSize ;
         // BSONElement end

         const CHAR *value ; // bosnobj rawdata.
         hashTuple *next ;

         void init()
         {
            data = NULL ;
            fieldNameSize = -1 ;
            totalSize = -1 ;
            value = NULL ;
            next = NULL ;
         }
      } ;
#pragma pack()

   private:
      CHAR *_buf ;
      UINT64 _bufSize ;
      UINT64 _written ;
      UINT32 _buckets ;
   } ;
   typedef class _qgmHashTable qgmHashTable ;
}

#endif

