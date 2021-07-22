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

   Source File Name = indexOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_OPTIONS_H_
#define VESSEL_INDEX_OPTIONS_H_

#include "vessel/indexDef.h"
#include "vessel/strSlice.h"
#include "ossMemPool.hpp"
#include "vessel/indexKeyPattern.h"

namespace engine
{
namespace vessel
{
   class createIndexOptions : public SDBObject
   {
      public:
         OSS_INLINE createIndexOptions(){}
         OSS_INLINE ~createIndexOptions(){}
         OSS_INLINE createIndexOptions &operator=(const createIndexOptions &o)
         {
            type = o.type;
            btreePrefixCompressionColumns = o.btreePrefixCompressionColumns;
            isUnique = o.isUnique;
            enforeced = o.enforeced;
            sortBufferSize = o.sortBufferSize;
            notNull = o.notNull;
            lsmColumnFamily = o.lsmColumnFamily;
            return *this;
         }

      public:
         BOOLEAN isValid()const;

      public:
         INDEX_TYPE type = INDEX_TYPE_LSM;
         UINT32 btreePrefixCompressionColumns = 0;
         BOOLEAN isUnique = FALSE;
         BOOLEAN enforeced = FALSE;
         UINT32 sortBufferSize = 64;//MB
         BOOLEAN notNull = FALSE;
         UINT32 lsmColumnFamily = 0;
   };//class createIndexOptions

   BOOLEAN isValidCreatingIndexArgs(const strSlice &indexName,
                                    const indexKeyPattern &pattern,
                                    const createIndexOptions &options);
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_OPTIONS_H_