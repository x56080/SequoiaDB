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

   Source File Name = extentSUContainer.cpp

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

#include "vessel/extentSUContainer.h"
#include "ossLikely.hpp"
#include "vessel/storageFileUtil.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem ;

namespace engine
{
namespace vessel
{
   extentSUContainer::extentSUContainer()
   :_isOpen(FALSE),
   _minIDInPool(0),
    _suVec(NULL)
   {
   }

   extentSUContainer::~extentSUContainer()
   {
   
   }

   INT32 extentSUContainer::open(requestContext *context,
                                 BOOLEAN crashRecovery)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (isOpen())
      {
         PD_LOG(PDERROR, "container has already been open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      
      _suVec = SDB_OSS_NEW _spaceSlot[MAX_SPACE_COUNT];
      if (NULL == _suVec)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = loadStorageUnits(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load stroage units:%d", rc);
         goto error;
      }

      if (!crashRecovery)
      {
         rc = initBitMapsOfSU(context, FALSE);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      _isOpen = TRUE;
   done:
      return rc;
   error:
      if (rollback)
      {
         close(context);
      }
      goto done;
   }

   INT32 extentSUContainer::allocateSpaceID(SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      
      ossScopedLock(&_mutex, EXCLUSIVE);
      if (_free.empty())
      {
         if (INVALID_SPACE_ID == _minIDInPool)
         {
            rc = SDB_DMS_SU_OUTRANGE;
            goto error;
         }
         else
         {
            sid = _minIDInPool++;
            if (MAX_SPACE_ID < _minIDInPool)
            {
               _minIDInPool = INVALID_SPACE_ID;
               goto done;
            }
            for (UINT32 i = 0; i < 31; ++i)
            {
               _free.insert(_minIDInPool++);
               if (MAX_SPACE_ID < _minIDInPool)
               {
                  _minIDInPool = INVALID_SPACE_ID;
                  break;
               }
            }
         }
      }
      else
      {
         sid = *(_free.begin());
         _free.erase(sid);
      }
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentSUContainer::releaseSpaceID(SPACE_ID sid)
   {
      INT32 rc = SDB_OK;

      ossScopedLock(&_mutex, EXCLUSIVE);
      if (INVALID_SPACE_ID == sid ||
          MAX_SPACE_ID < sid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (INVALID_SPACE_ID != _minIDInPool &&  _minIDInPool <= sid)
      {
         SDB_ASSERT(FALSE, "try to release a non-allocated space id");
         PD_LOG(PDERROR, "min id in pool:%d, id to be released:%d", _minIDInPool, sid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (MAX_SPACE_ID == sid)
      {
         _minIDInPool = sid;
      }
      else if (sid == (_minIDInPool - 1))
      {
         --_minIDInPool;
         SPACE_ID maxInFree = INVALID_SPACE_ID;
         while (!_free.empty())
         {
            maxInFree = *(_free.rbegin());
            if (maxInFree == (_minIDInPool - 1))
            {
               _free.erase(maxInFree);
               --_minIDInPool;
            }
            else
            {
               goto done;
            }
            
         }
      }
      else
      {
         _free.insert(sid);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentSUContainer::createSU(requestContext *context,
                                     const createSUOptions &options,
                                     extentStorageUnit **out)
   {
      INT32 rc = SDB_OK;
      extentStorageUnit *su = NULL;
      SPACE_ID sid = options.sid;
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(sid < MAX_SPACE_COUNT, "impossible");

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       MAX_SPACE_COUNT <= sid))
      {
         PD_LOG(PDERROR, "invalid space id:%d", sid);
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(sid != context->getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(EXCLUSIVE != context->getSpaceIDLockedMode()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!isOpen()))
      {
         PD_LOG(PDERROR, "su container not open yet");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!_suVec[sid].free())
      {
         SDB_ASSERT(FALSE, "impossible");
         PD_LOG(PDERROR, "space slot[%d] is not free", sid);
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }

      su = SDB_OSS_NEW extentStorageUnit();
      if (NULL == su)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = su->create(context, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new su: %s, %d", options.csName.str(), rc);
         goto error;
      }

      _suVec[sid].su = su;
      if (NULL != out)
      {
         *out = su;
      }
   done:
      return rc;
   error:
      if (NULL != su)
      {
         SDB_OSS_DEL su;
         su = NULL;
      }
      goto done;
   }

   INT32 extentSUContainer::dropSU(requestContext *context, BOOLEAN releaseSID)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      INT32 destroyRC = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");

      if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(EXCLUSIVE != context->getSpaceIDLockedMode()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = context->getSpaceID();
      SDB_ASSERT(INVALID_SPACE_ID != sid &&
                 sid <= MAX_SPACE_ID, "impossible");

      if (_suVec[sid].free())
      {
         PD_LOG(PDERROR, "space id:%d is free", sid);
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      destroyRC = _suVec[sid].su->destroy(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to destroy su:%d, %d", sid, rc);
         /// do not goto error!
      }

      SDB_OSS_DEL _suVec[sid].su;
      _suVec[sid].su = NULL;

      if (releaseSID)
      {
         releaseSpaceID(sid);
      }

      if (SDB_OK != destroyRC)
      {
         rc = destroyRC;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentSUContainer::loadStorageUnits(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      const storagePathOptions &path = context->getEnv()->options.path;
      fs::directory_iterator end_iter ;
      fs::path dataDir(path.dataPath);
      extentStorageUnit *su = NULL;
      SPACE_ID maxSid = INVALID_SPACE_ID;

      if (!fs::exists(dataDir) || !fs::is_directory(dataDir))
      {
         PD_LOG(PDERROR, "invalid data path:%s", path.dataPath.c_str());
         rc = SDB_INVALIDARG;
         goto error;
      }

       for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
       {
          std::string name = dir_iter->path().filename().string();
          strSlice nameSlice(name.c_str(), name.length());
          if (!parseStorageUnitDir(nameSlice, NULL))
          {
             continue;
          }

          if (!fs::is_directory(dir_iter->status()))
          {
             rc = SDB_VESSEL_INVALID_VESSEL_FILE;
             PD_LOG(PDERROR, "name valid but not a dir:%s", name.c_str());
             goto error;
          }

          SPACE_ID space = INVALID_SPACE_ID;
          su = SDB_OSS_NEW extentStorageUnit();
          if (NULL == su)
          {
             PD_LOG(PDERROR, "failed to allocate mem");
             rc = SDB_OOM;
             goto error;
          }

          rc = su->open(context, nameSlice);
          if (SDB_VESSEL_CRASHED_WHEN_CREATING == rc)
          {
             SDB_OSS_DEL su;
             su = NULL;
             rc = SDB_OK;
             PD_LOG(PDERROR, "crashed when creating su:%s", name.c_str());
             continue; /// recreate file when redo
          }
          else if (SDB_OK != rc)
          {
             PD_LOG(PDERROR, "failed to open su:%d", rc);
             goto error;
          }

          space = su->getSpaceID();
          if (MAX_SPACE_COUNT <= space)
          {
             
             PD_LOG(PDERROR, "invalid space id:%d", space);
             rc = SDB_VESSEL_INTERNAL_ERR;
             goto error;
          }
          
          if (!_suVec[space].free())
          {
             PD_LOG(PDERROR, "space[%d] slot is not free", space);
             rc = SDB_VESSEL_INTERNAL_ERR;
             goto error;
          }

          _suVec[space].su = su;
          su = NULL;
          if (INVALID_SPACE_ID ==  maxSid)
          {
             maxSid = space;
          }
          else if (maxSid < space)
          {
             maxSid = space;
          }
          
       }

       initFreeListAfterLoading(maxSid);
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(su);
      goto done;
   }

   INT32 extentSUContainer::initFreeListAfterLoading(SPACE_ID maxID)
   {
      INT32 rc = SDB_OK;
      if (INVALID_SPACE_ID == maxID)
      {
         _minIDInPool = 0;
         goto done;
      }
      for (SPACE_ID sid = 0; sid <= maxID; ++sid)
      {
         if (_suVec[sid].free())
         {
            _free.insert(sid);
         }
      }

      if (MAX_SPACE_ID == maxID)
      {
         _minIDInPool = INVALID_SPACE_ID;
      }
      else
      {
         _minIDInPool = maxID + 1;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentSUContainer::close(requestContext *context)
   {
      INT32 rc = SDB_OK;

      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < MAX_SPACE_COUNT; ++i)
      {
         _spaceSlot &slot = _suVec[i];
         if (!slot.free())
         {
            slot.su->close(context);
            SDB_OSS_DEL slot.su;
            slot.su = NULL;
         }
      }
      SDB_OSS_DEL []_suVec;
      _suVec = NULL;
      _isOpen = FALSE;
      _minIDInPool = 0;
      _free.clear();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentSUContainer::getSUByContext(requestContext *context,
                                           extentStorageUnit **su)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context || NULL == su))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSUBySpaceID(context->getSpaceID(), su);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentSUContainer::getSUBySpaceID(SPACE_ID sid, extentStorageUnit **su)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       MAX_SPACE_ID < sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_suVec[sid].free())
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      *su = _suVec[sid].su;
   done:
      return rc;
   error:
      goto done;
   }

   SPACE_ID extentSUContainer::getFirstSpaceIDWhenStartup()
   {
      SPACE_ID sid = INVALID_SPACE_ID;
      if (0 == _minIDInPool)
      {
         goto done;
      }

      for (SPACE_ID i = 0; i < _minIDInPool; ++i)
      {
         if (!_suVec[i].free())
         {
            sid = i;
            break;
         }
      }
   done:
      return sid;
   }

   extentStorageUnit *extentSUContainer::getNextSUWhenStartup(SPACE_ID &sid)
   {
      extentStorageUnit *su = NULL;
      SPACE_ID id = sid;
      if (INVALID_SPACE_ID == sid ||
          MAX_SPACE_ID < sid)
      {
         goto done;
      }
      else if (0 == _minIDInPool ||
               _minIDInPool < sid)
      {
         goto done;
      }
      
      su = _suVec[sid].su;
      SDB_ASSERT(NULL != su, "sid may be modified outside");
      sid = INVALID_SPACE_ID;
      for (SPACE_ID i = id + 1; i < _minIDInPool && i <= MAX_SPACE_ID; ++i)
      {
         if (!_suVec[i].free())
         {
            sid = i;
            break;
         }
      }
      
   done:
      return su;
   }

   INT32 extentSUContainer::initBitMapsOfSU(requestContext *context, BOOLEAN rebuild)
   {
      INT32 rc = SDB_OK;
      for (SPACE_ID i = 0; i < _minIDInPool; ++i)
      {
         if (!_suVec[i].free())
         {
            rc = _suVec[i].su->initInMemBitMapWhenOpening(context, rebuild);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
}// namespace vessel
}// namespace engine