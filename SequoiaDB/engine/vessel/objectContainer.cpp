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

   Source File Name = objectContainer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/objectContainer.h"
#include "vessel/collectionSpace.h"
#include "ossLikely.hpp"
#include "vessel/spaceIDLocker.h"
#include "vessel/extentSUContainer.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   objectContainer::objectContainer():
   _csVec(NULL)
   {

   }

   objectContainer::~objectContainer()
   {
      teardown();
   }

   INT32 objectContainer::setup()
   {
      INT32 rc = SDB_OK;
      _csVec = SDB_OSS_NEW _csSlot[MAX_SPACE_COUNT];
      if (NULL == _csVec)
      {
         PD_LOG(PDERROR, "failed to alloate mem");
         rc = SDB_OOM;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 objectContainer::teardown()
   {
      INT32 rc = SDB_OK;
      
      if (NULL != _csVec)
      {
         _nameIndex.clear();
         _idIndex.clear();
         for (UINT32 i = 0; i < MAX_SPACE_COUNT; ++i)
         {
            if (!_csVec[i].free())
            {
               SDB_OSS_DEL _csVec[i].cs;
               _csVec[i].cs = NULL;
            }
         }
         SDB_OSS_DEL []_csVec;
         _csVec = NULL;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN objectContainer::csExists(const CHAR *name, UINT32 logicalID)
   {
      SDB_ASSERT(NULL != name, "can not be null");
      if (OSS_UNLIKELY(NULL == name))
      {
         return TRUE;
      }
      
      _mutex.get_shared();
      BOOLEAN r = (0 < _nameIndex.count(name)) ||
                  (0 < _idIndex.count(logicalID));
      _mutex.release_shared();
      return r;
   }

   INT32 objectContainer::getSpaceIDFromIndex(UINT32 logicalID, SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      _mutex.get_shared();
      ID_INDEX::const_iterator itr = _idIndex.find(logicalID);
      if (_idIndex.end() == itr)
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
      sid = itr->second;
   done:
      _mutex.release_shared();
      return rc;
   error:
      goto done;
   }

   INT32 objectContainer::getSpaceIDFromIndex(const CHAR *name, SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      _mutex.get_shared();
      NAME_INDEX::const_iterator itr = _nameIndex.end();
      if (OSS_UNLIKELY(NULL == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      itr = _nameIndex.find(name);
      if (_nameIndex.end() == itr)
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
      sid = itr->second;
   done:
      _mutex.release_shared();
      return rc;
   error:
      goto done;
   }

   INT32 objectContainer::getSpaceIDByUpperBound(UINT32 logicalID, SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      ossScopedLock(&_mutex, SHARED);
      if (DMS_INVALID_LOGICCSID == logicalID)
      {
         if (_idIndex.empty())
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
         else
         {
            sid = _idIndex.begin()->second;
         }
      }
      else
      {
         ID_INDEX::const_iterator itr = _idIndex.upper_bound(logicalID);
         if (_idIndex.end() == itr)
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
         else
         {
            sid = itr->second;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 objectContainer::getCSByLogicalID(requestContext *context,
                                           UINT32 logicalID,
                                           OSS_LATCH_MODE mode,
                                           collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      SDB_ASSERT(NULL != context, "can not be null");
      BOOLEAN locked = FALSE;
  
      if (OSS_UNLIKELY(context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSpaceIDFromIndex(logicalID, sid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = context->lockSpaceID(sid, mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      rc = getCSUnderIDLocked(context, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// logical cs id can not be modified.
      /// if space id is not free, obj's logical id should be same to
      /// input logical id.
      SDB_ASSERT(logicalID == (*obj)->getLogicalID(), "must be same");
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockSpaceID();
      }
      goto done;
   }

   INT32 objectContainer::getCSUnderIDLocked(requestContext *context,
                                             collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      if (OSS_UNLIKELY(NULL == context || NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = context->getSpaceID();
      if (_csVec[sid].free())
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      *obj = _csVec[sid].cs;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 objectContainer::allocateCSObj(requestContext *context,
                                        const CHAR *name,
                                        UINT32 logicalID,
                                        collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *csObj = NULL;
      BOOLEAN rollbackIndex = FALSE;

      if (OSS_UNLIKELY(NULL == name || NULL == context ||
                       DMS_INVALID_LOGICCSID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!context->getSpaceIDLocked() ||
           EXCLUSIVE != context->getSpaceIDLockedMode()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = context->getSpaceID();
      if (!_csVec[sid].free())
      {
         PD_LOG(PDERROR, "space id is not free:%d", sid);
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = addToIndex(name, logicalID, sid);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollbackIndex = TRUE;

      csObj = SDB_OSS_NEW collectionSpace();
      if (NULL == csObj)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _csVec[sid].cs = csObj;
      if (NULL != obj)
      {
         *obj = csObj;
      }
   done:
      return rc;
   error:
      if (rollbackIndex)
      {
         eraseFromIndex(name, logicalID);
      }
      SAFE_OSS_DELETE(csObj);
      goto done;
   }

   INT32 objectContainer::allocateCSObjWhenStartup(requestContext *context,
                                                   extentStorageUnit *su,
                                                   collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *csObj = NULL;
      
      if (OSS_UNLIKELY(NULL == context || NULL == su))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = su->getSpaceID();
      if (!_csVec[sid].free())
      {
         PD_LOG(PDERROR, "space id:%d is not free", sid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      csObj = SDB_OSS_NEW collectionSpace();
      if (NULL == csObj)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = csObj->setup(context, su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!csObj->isOnline())
      {
         csObj->teardown();
         SDB_OSS_DEL csObj;
         csObj = NULL;
         goto done;
      }

      rc = addToIndex(csObj->getCSName(), csObj->getLogicalID(),
                      csObj->getSpaceID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "duplicated cs name or id:%s, %d", csObj->getCSName(), csObj->getLogicalID());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _csVec[sid].cs = csObj;
      
      if (NULL != obj)
      {
         *obj = csObj;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(csObj);
      goto done;
   }

   INT32 objectContainer::releaseCSObj(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *obj = NULL;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked() ||
           EXCLUSIVE != context->getSpaceIDLockedMode()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = context->getSpaceID();
      if (_csVec[sid].free())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      obj = _csVec[sid].cs;
      eraseFromIndex(obj->getCSName(), obj->getLogicalID());
      obj->teardown();
      SDB_OSS_DEL obj;
      _csVec[sid].cs = NULL;

   done:
      return rc;
   error:
      goto done;
   }


   UINT32 objectContainer::getNameCountInIndex()
   {
      ossScopedLock(&_mutex, SHARED);
      return _nameIndex.size();
   }


   INT32 objectContainer::addToIndex(const CHAR *name, UINT32 logicalID, SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != name, "can not be null");

      _mutex.get();

      if (!_nameIndex.insert(std::make_pair(name, sid)).second)
      {
         PD_LOG(PDERROR, "duplicated cs name:%s", name);
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }

      if (!_idIndex.insert(std::make_pair(logicalID, sid)).second)
      {
         _nameIndex.erase(name);
         PD_LOG(PDERROR, "duplicated cs logical id:%d", logicalID);
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }

   done:
       _mutex.release();
      return rc;
   error:
      goto done;
   }

   void objectContainer::eraseFromIndex(const CHAR *name, UINT32 logicalID)
   {
      SDB_ASSERT(NULL != name, "can not be null");
      _mutex.get();
      _nameIndex.erase(name);
      _idIndex.erase(logicalID);
      _mutex.release();
      return;
   }

}//namespace vessel
}//namespace engine