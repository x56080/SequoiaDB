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

   Source File Name = listLobChunkCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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