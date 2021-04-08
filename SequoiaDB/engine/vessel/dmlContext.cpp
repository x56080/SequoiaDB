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

   Source File Name = dmlContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dmlContext.h"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/collectionSpace.h"
#include "vessel/collection.h"

namespace engine
{
namespace vessel
{
   dmlContext::~dmlContext()
   {
      if (isOpen())
      {
         close();
      }
   }

   void dmlContext::close()
   {
      reset();
      requestContext::close();
      return;
   }

   void dmlContext::reset()
   {
      _csName.reset();
      _clName.reset();
      _clLogicalID = DMS_INVALID_LOGICCLID;
      _clUniqueID = utilBuildCLUniqueID(UTIL_INVALID_CS_UNIQUE_ID,
                                    UTIL_INVALID_CL_INNER_ID);
      _transID.reset();
      _striping = INVALID_STRIPING_ID;
      _originalRecord.reset();
      _compressedRecord.reset();
      _compressionType = UTIL_COMPRESSOR_INVALID;
      if (NULL != _compressionBuffer)
      {
         SDB_THREAD_FREE(_compressionBuffer);
         _compressionBuffer = NULL;
      }
      _compressionBufferSize = 0;
      
      _lsn = DPS_INVALID_LSN_OFFSET;
      _rid = recordID();
      _uniqueIndexHash.clear();
      return;
   }

   void dmlContext::initNewRequest(collectionSpace *cs,
                                    collection *cl,
                                    const recordData &record,
                                    const DPS_TRANS_ID &transID,
                                    STRIPING_ID striping)
   {
      SDB_ASSERT(NULL != cs && NULL != cl, "can not be null");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      reset();
      _csName.reset(cs->getCSName());
      _clName.reset(cl->getName());
      _clLogicalID = cl->getLogicalID();
      _clUniqueID = utilBuildCLUniqueID(cs->getUniqueID(), cl->getInnerID());
      if (transID.isValid())
      {
         _transID = transID;
      }
      _striping = striping;
      _originalRecord = record;
      _compressionType = cl->getCompressionType();
      return;
   }


    INT32 dmlContext::allocateCompressionBuffer(UINT32 size)
    {
       INT32 rc = SDB_OK;
       if (size <= _compressionBufferSize)
       {
          goto done;
       }
       else if (NULL != _compressionBuffer)
       {
          SDB_THREAD_FREE(_compressionBuffer);
          _compressionBuffer = NULL;
          _compressionBufferSize = 0;
       }

       _compressionBuffer = (CHAR *)SDB_THREAD_ALLOC(size);
       if (NULL == _compressionBuffer)
       {
          PD_LOG(PDERROR, "failed to allocate mem");
          rc = SDB_OOM;
          goto error;
       }
       _compressionBufferSize = size;
    done:
       return rc;
    error:
       goto done;
    }
}//namespace vessel
}//namespace engine