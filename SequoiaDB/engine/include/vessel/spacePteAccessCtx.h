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

   Source File Name = spacePteAccessCtx.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSE_SPACE_PTE_ACCESS_CTX_H_
#define VESSE_SPACE_PTE_ACCESS_CTX_H_

#include "ossMemPool.hpp"
#include "vessel/pageIdentifier.h"
#include "vessel/sparseBitmap32.h"
#include "vessel/storageFileDef.h"

#include <memory>

namespace engine
{
namespace vessel
{
   class spacePteAccessCtx : public SDBObject
   {
      friend class logicalPageSpacePte;
      public:
         spacePteAccessCtx(UINT32 id):_id(id){}
         ~spacePteAccessCtx(){}
         spacePteAccessCtx(const spacePteAccessCtx &) = delete;
         spacePteAccessCtx &operator=(const spacePteAccessCtx &) = delete;

      public:
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _pmap.empty() &&
                   _obsoleteLpids.isEmpty() &&
                   _obsoletePids.isEmpty();
         }
         OSS_INLINE UINT32 getId()const {return _id;}

         void exportDirtySegments(const storageCoreArgs &args,
                                  ossPoolSet<UINT32> &segments)const;
         void exportDirtyFiles(const storageCoreArgs &args,
                               ossPoolSet<UINT32> &files)const;

      private:
         using _P_MAPPING = ossPoolMap<PAGE_ID, PAGE_ID>;

      private:
         void set(PAGE_ID lpid, PAGE_ID pid);
         void reset(PAGE_ID lpid);
         void erase(PAGE_ID lpid);
         BOOLEAN get(PAGE_ID lpid, PAGE_ID &pid)const;
         BOOLEAN isPrivate(PAGE_ID lpid)const;
         void obsoleteLpid(PAGE_ID lpid);
         void obsoletePid(PAGE_ID lpid);
         void resetObsoleteResources();
         
      private:
         UINT32 _id = 0;
         _P_MAPPING _pmap;

         sparseBitmap32 _obsoleteLpids;
         sparseBitmap32 _obsoletePids;
   };//class spacePteAccessCtx

   using PTE_ACCESS_CTX_PTR = std::unique_ptr<spacePteAccessCtx>;
} // namespace vessel

} // namespace engine


#endif//VESSE_SPACE_PTE_ACCESS_CTX_H_
