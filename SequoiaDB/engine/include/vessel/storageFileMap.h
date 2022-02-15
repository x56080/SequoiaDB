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

   Source File Name = storageFileMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_MAP_H_
#define VESSEL_STORAGE_FILE_MAP_H_

#include "ossMemPool.hpp"
#include "vessel/storageFileName.h"

namespace engine
{
namespace vessel
{
   class storageFile;
   class storageFileMap : public SDBObject
   {
      public:
         storageFileMap(){}
         ~storageFileMap();
         storageFileMap(const storageFileMap &) = delete;
         storageFileMap &operator=(const storageFileMap &) = delete;

      public:
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _map.empty();
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _map.size();
         }

      public:
         void fini();
         INT32 insertFile(storageFile *file);

         /// will not dump closed file name if list is null.
         void closeIfLess(UINT64 sequence,
                          STORAGE_FILE_NAME_LIST *list);

         void destroyIfLess(UINT64 sequence);

         void destroyIfGreater(UINT64 sequence);

         void destroy();

         void destroy(UINT64 sequence);

         BOOLEAN hasSequenceBreakpoint()const;

         /// return null if empty
         storageFile *getFirst()const;

         ///return null if empty
         storageFile *getLast()const;

         ///return null if not exists
         storageFile *get(UINT64 sequence)const;

      private:
         typedef ossPoolMap<UINT64, storageFile *> _FILE_MAP;

      public:
         typedef _FILE_MAP::const_iterator CONST_ITERATOR;
         CONST_ITERATOR begin()const
         {
            return _map.begin();
         }
         CONST_ITERATOR end()const
         {
            return _map.end();
         }
      private:
         _FILE_MAP _map;

   };//class storageFileMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_FILE_MAP_H_