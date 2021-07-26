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

   Source File Name = dmlContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DML_CONTEXT_H_
#define VESSEL_DML_CONTEXT_H_

#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "utilCompression.hpp"
#include "dpsDef.hpp"
#include "vessel/recordID.h"
#include "ossMemPool.hpp"
#include "ossLikely.hpp"
#include "vessel/vesselIdDef.h"
#include "vessel/requestContext.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class collection;

   class dmlContext : public requestContext
   {
      public:
         dmlContext(){}
         virtual ~dmlContext();

      public:
         virtual void close();

         void setCLInfo(const strSlice &csName,
                         const strSlice &clName);

         OSS_INLINE const strSlice &getCSName()const
         {
            return _csName;
         }
         OSS_INLINE const strSlice &getCLName()const
         {
            return _clName;
         }
         OSS_INLINE BOOLEAN clInfoIsValid()const
         {
            return !_csName.empty() &&
                   !_clName.empty();
         }
         OSS_INLINE void setTransID(const DPS_TRANS_ID &transID)
         {
            _transID = transID;
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const
         {
            return _transID;
         }
         OSS_INLINE UINT32 getUniqueKeyCount()const
         {
            return _uniqueKeyHash.size();
         }
         OSS_INLINE const UINT16 *getUniqueKeys()const
         {
            return _uniqueKeyHash.data();
         }

         ///WARNING: Unqiue keys must be added in order.
         void addUniqueKey(UINT16 key);

         INT32 lockUniqueIndexKeys();

         void unlockUniqueKeys();

         void setMinFreeSize(UINT32 size)
         {
            _minFreeSize = size;
         }
         UINT32 getMinFreeSize()const
         {
            return _minFreeSize;
         }
      private:
         void fini();

      private:
         strSlice _csName;
         strSlice _clName;
         DPS_TRANS_ID _transID;
         ossPoolVector<UINT16> _uniqueKeyHash;
         BOOLEAN _uniqueKeyLocked = FALSE;
         UINT32 _minFreeSize = 0;
   };//class dmlContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_INSERT_CONTEXT_H_
