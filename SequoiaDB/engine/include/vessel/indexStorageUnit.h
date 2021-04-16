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

   Source File Name = indexStorageUnit.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_STORAGE_UNIT_H_
#define VESSEL_INDEX_STORAGE_UNIT_H_

#include "ossMemPool.hpp"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class indexMetaFile;
   class indexDataFile;
   class cowSUDeltaLog;

   class indexStorageUnit : public SDBObject
   {
      public:
         indexStorageUnit();
         ~indexStorageUnit();

         indexStorageUnit(const indexStorageUnit &) = delete;
         indexStorageUnit &operator=(const indexStorageUnit &) = delete;

      private:
         typedef ossPoolVector<indexDataFile *> _INDEX_DATA_VEC;

      private:
         indexMetaFile *_idexMeta;
         _INDEX_DATA_VEC _idx;
         cowSUDeltaLog *_delta;

   };//class indexStorageUnit
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_STORAGE_UNIT_H_