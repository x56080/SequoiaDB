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

   Source File Name = lpageMapping.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPAGE_MAPPING_H_
#define VESSEL_LPAGE_MAPPING_H_

#include "vessel/lpageMetaDataFile.h"
#include "vessel/lpageDescriptor.h"
#include "vessel/metaDataUberBlock.h"
#include "vessel/lpageMappingRoot.h"
#include "vessel/lpageMappingPteCtx.h"
#include "vessel/fixedBitset.hpp"

namespace engine
{
namespace vessel
{   
   class lpageMapping : public SDBObject
   {
      public:
         lpageMapping() = default;
         ~lpageMapping() = default;
         lpageMapping(const lpageMapping &) = delete;
         lpageMapping &operator=(const lpageMapping &) = delete;

      public:
         static constexpr UINT32 LPID_UNIT_SIZE =
                         lpageMetaDataFile::PAGE_SIZE / LPAGE_DESC_SIZE;
         static constexpr UINT32 MAPPING_ENTRY_CAPACITY =
                       lpageMetaDataFile::PAGE_SIZE / sizeof(PAGE_ID);
         static constexpr UINT32 MAX_UNIT_COUNT =
                         lpmUberBlock::MAPPING_ENTRY_SIZE * MAPPING_ENTRY_CAPACITY;
         static constexpr UINT32 MAX_LPID_COUNT = MAX_UNIT_COUNT * LPID_UNIT_SIZE;

      private:
         static const UINT32 _UNIT_SIZE_SQUARE;
         static const UINT32 _MAPPING_CAPACITY_SQUARE;

      public:
         OSS_INLINE BOOLEAN isValid()const {return nullptr != _mfile;}
         OSS_INLINE BOOLEAN isOutOfMaxBound(PAGE_ID lpid)const
         {
            return MAX_LPID_COUNT <= lpid;
         }
         OSS_INLINE const lpageMappingRoot &getRoot()const {return _root;}

      public:
         INT32 init(lpageMetaDataFile *mfile,
                    const lpmUberBlock *ub);
         void fini();

         /// not thread-safe
         INT32 ensureUnitSpace(UINT32 unitId);

         INT32 dumpUnitSme(UINT32 unitId,
                           BOOLEAN &exists,
                           fixedBitset<LPID_UNIT_SIZE> &bs);
   

      public:/// user should lock lpids outside first.
             /// or guarantee these lpids invisible.
         INT32 set(PAGE_ID lpid,
                   const lpageDescriptor &desc,
                   lpageDescriptor *oldVal=nullptr);

         /// all or nothing.
         INT32 setBatch(UINT32 size,
                        PAGE_SNAPSHOT_VERION psv,
                        const PAGE_ID *lpids,
                        const PAGE_ID *pids,
                        lpageDescriptor *oldVals=nullptr);

         /// return ok but invalid desc if lpid not mapped.
         INT32 get(PAGE_ID lpid, lpageDescriptor &desc);
         INT32 reset(PAGE_ID lpid, lpageDescriptor *oldVal=nullptr);
         INT32 resetBatch(UINT32 size,
                          const PAGE_ID *lpids,
                          lpageDescriptor *oldVals=nullptr);

      public:///WARNING: make sure there is at most one active ctx at one time!
         INT32 set(lpageMappingPteCtx &ctx,
                   PAGE_ID lpid,
                   const lpageDescriptor &desc);

         INT32 setBatch(lpageMappingPteCtx &ctx,
                        PAGE_SNAPSHOT_VERION psv,
                        UINT32 size,
                        const PAGE_ID *lpids,
                        const PAGE_ID *pids);

         /// return ok but invalid desc if lpid not mapped.
         INT32 getPtePrior(const lpageMappingPteCtx &ctx,
                           PAGE_ID lpid,
                           lpageDescriptor &desc);

         INT32 reset(lpageMappingPteCtx &ctx,
                     PAGE_ID lpid,
                     lpageDescriptor *oldVal=nullptr);

         /// revert to published mapping
         // INT32 revert(lpageMappingPteCtx &ctx,
         //              PAGE_ID lpid,
         //              lpageDescriptor &beforeRevert,
         //              lpageDescriptor &afterRevert);

         void publish(lpageMappingPteCtx &ctx);

         void freeOboleteSetAfterPublish(lpageMappingPteCtx &ctx);

         void abort(lpageMappingPteCtx &ctx);

      private:
         void _free(UINT32 size, const PAGE_ID *lpids);
         INT32 _ensureDescriptorPage(UINT32 unitId, PAGE_ID &pid);
         INT32 _getDescriptorPage(const lpageMappingRoot *pte,
                                  UINT32 unitId, PAGE_ID &pid);
         INT32 _createEntry(UINT32 pos, PAGE_ID &pid);

         void _reset(UINT32 size, lpageDescriptor *descriptors);

      private:
         INT32 _ensurePrivatePath(lpageMappingPteCtx &ctx,
                                  UINT32 unitId,
                                  PAGE_ID &descPid);

         INT32 _ensurePrivateRootEntry(UINT32 pos, lpageMappingPteCtx &ctx);

      private:

         OSS_INLINE UINT32 _getEntryPosByLpid(PAGE_ID lpid, UINT32 &posInEntry)const
         {
            UINT32 unitId = _getUnitId(lpid);
            return _getEntryPosByUnitId(unitId, posInEntry);
         }  

         OSS_INLINE UINT32 _getEntryPosByUnitId(UINT32 unitId, UINT32 &posInEntry)const
         {
            posInEntry = unitId & (MAPPING_ENTRY_CAPACITY - 1);
            return unitId >> _MAPPING_CAPACITY_SQUARE;
         }

         OSS_INLINE UINT32 _getDescPos(PAGE_ID lpid)const
         {
            return lpid & (LPID_UNIT_SIZE - 1);
         }

         OSS_INLINE UINT32 _getUnitId(PAGE_ID lpid)const
         {
            return lpid >> _UNIT_SIZE_SQUARE;
         }

      private:
         lpageMetaDataFile *_mfile = nullptr;
         lpageMappingRoot _root;
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_MAPPING_H_
