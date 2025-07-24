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

   Source File Name = listLobChunkCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LIST_LOB_CHUNK_CURSOR_H_
#define VESSEL_LIST_LOB_CHUNK_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "vessel/lobcBucketRegion.h"
#include "dmsEngineOptions.hpp"
#include "vessel/objectIdentifier.h"
#include "vessel/fixedBitset.hpp"

namespace engine
{
namespace vessel
{
   class listLobChunkCursor : public cursorKernal
   {
      public:
         listLobChunkCursor(const globalCollectionId &gcid,
                            const dmsListLobChunkOptions &o):
         _gcid(gcid),
         _o(o){}
         virtual ~listLobChunkCursor(){}

      public:
         virtual const CHAR *getName()const override
         {
            return "vessel.listLobChunkCursor";
         }
         virtual slice getDataSlice()const override
         {
            return getRawData();
         }

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_LIST_LOBC;
         } 
   
      public:
         OSS_INLINE const globalCollectionId &getCollectionId()const {return _gcid;}
         OSS_INLINE const dmsListLobChunkOptions &getOptions()const {return _o;}


         INT32 getBucketPosToScan()const;

         INT32 getRegionId()const {return _regionId;}

         void endToScanBucket(UINT32 pos);

         void setRegionToScan(UINT32 regionId,
                              const lobcBucketRegionBlock &regionBlock);

      private:
         const globalCollectionId _gcid;
         dmsListLobChunkOptions _o;
         INT32 _regionId = -1;
         fixedBitset<lobcBucketRegionBlock::BUCKET_COUNT> _bucketToScan;
   };//class listLobChunkCursor
} // namespace vessel

} // namespace engine


#endif//VESSEL_LIST_LOB_CHUNK_CURSOR_H_