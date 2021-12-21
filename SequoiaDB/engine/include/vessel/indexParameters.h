/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = indexParameters.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_PARAMETERS_H_
#define VESSEL_INDEX_PARAMETERS_H_

#include "vessel/indexDef.h"
#include "vessel/strSlice.h"
#include "../bson/bson.hpp"

namespace engine
{
namespace vessel
{
   class indexParameters : public SDBObject
   {
      public:
         indexParameters(){}
         ~indexParameters(){}
         indexParameters &operator=(const indexParameters &o)
         {
            type = o.type;
            isUnique = o.isUnique;
            enforeced = o.enforeced;
            notNull = o.notNull;
            notArray = o.notArray;
            btreeMaxPrefixFields = o.btreeMaxPrefixFields;
            columnFamily = o.columnFamily;
            return *this;
         }

      public:
         BOOLEAN isValid()const;

         void exportToBson(bson::BSONObjBuilder &builder)const;
         BOOLEAN extractFromBson(const bson::BSONObj &obj);

         BOOLEAN isPrefixCompressionEnabled()const;

      public:
         OSS_INLINE BOOLEAN isBtreeIndex()const
         {
            return INDEX_TYPE_BTREE == type;
         }
         OSS_INLINE BOOLEAN isLsmIndex()const
         {
            return INDEX_TYPE_LSM == type;
         }
      public:
         INDEX_TYPE type = INVALID_INDEX_TYPE;
         BOOLEAN isUnique = FALSE;
         BOOLEAN enforeced = FALSE;
         BOOLEAN notNull = FALSE;
         BOOLEAN notArray = FALSE;

         /******* btree only begin   *******/

         BOOLEAN btreeCompressionEnabled = FALSE;
         /// Max column count of prefix in btree prefix compression.
         /// It should be one unless there are a lot of duplicate index keys.
         UINT32 btreeMaxPrefixFields = 0;

         /// Min depth of btree to enable prefix compression.
         UINT32 btreeMinCompressionDepth = 2;
         
         /******* btree only end   *******/


         /******* lsm only bein   *******/
         UINT32 columnFamily = 0;
         /******* lsm only bein   *******/
   };//class indexParameters
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_PARAMETERS_H_