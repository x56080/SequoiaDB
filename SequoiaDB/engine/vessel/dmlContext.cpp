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

   INT32 dmlContext::initCL(collectionSpace *cs,
                            collection *cl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(getSpaceIDLocked(), "space id should be locked");
      SDB_ASSERT(mbLocked(), "mb id should be locked");
      SDB_ASSERT(isOpen(), "should be open first");
      SDB_ASSERT(_csName.empty(), "do not reinit");
      UINT32 csNameLen = 0;
      UINT32 clNameLen = 0;

      if (OSS_UNLIKELY(NULL == cs || NULL == cl))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (DMS_INVALID_LOGICCLID == cl->getLogicalID())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      csNameLen = ossStrlen(cs->getCSName());
      clNameLen = ossStrlen(cl->getName());
      if (0 == csNameLen || 0 == clNameLen)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _csName.reset(cs->getCSName(), csNameLen);
      _clName.reset(cl->getName(), clNameLen);
      _clLogicalID = cl->getLogicalID();
      _uniqueID = utilBuildCLUniqueID(cs->getUniqueID(), cl->getInnerID());
      _compressionType = cl->getCompressionType();
      
   done:
      return rc;
   error:
      goto done;
   }

    void dmlContext::reset()
    {
       _csName.reset();
       _clName.reset();
       _clLogicalID = DMS_INVALID_LOGICCLID;
       _uniqueID = utilBuildCLUniqueID(UTIL_INVALID_CS_UNIQUE_ID,
                                       UTIL_INVALID_CL_INNER_ID);
       _compressionType = UTIL_COMPRESSOR_INVALID;
       _record.reset();
       _compressedRecordSize = 0;
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

    INT32 dmlContext::allocateCompressionBuffer(UINT32 size)
    {
       INT32 rc = SDB_OK;
       if (size <= _compressionBufferSize)
       {
          _compressedRecordSize = 0;
          goto done;
       }
       else if (NULL != _compressionBuffer)
       {
          SDB_THREAD_FREE(_compressionBuffer);
          _compressionBuffer = NULL;
          _compressionBufferSize = 0;
          _compressedRecordSize = 0;
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