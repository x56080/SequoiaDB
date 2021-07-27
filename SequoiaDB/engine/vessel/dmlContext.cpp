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
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   dmlContext::~dmlContext()
   {
      fini();
   }

   void dmlContext::close()
   {
      unlockUniqueKeys();
      fini();
      requestContext::close();
      return;
   }

   void dmlContext::fini()
   {
      SDB_ASSERT(_uniqueKeyContext.empty(), "release locks first");
      _csName.reset();
      _clName.reset();
      _transID.reset();
      _uniqueKeyHash.clear();
      _uniqueKeyContext.clear();
      _minFreeSize = 0;
      return;
   }

   void dmlContext::setCLInfo(const strSlice &csName,
                               const strSlice &clName)
   {
      SDB_ASSERT(!csName.empty(), "can not be empty");
      SDB_ASSERT(!clName.empty(), "can not be empty");
      _csName = csName;
      _clName = clName;
      return;
   }

   void dmlContext::addUniqueKey(UINT16 key)
   {
      SDB_ASSERT(_uniqueKeyContext.empty(), "do not add key after locking");
      _uniqueKeyHash.push_back(key);
   }

   INT32 dmlContext::lockUniqueIndexKeys()
   {
      INT32 rc = SDB_OK;
      if (!requestContext::isOpen() ||
          !requestContext::isSpaceIdLocked() ||
          !requestContext::isMbLocked())
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_uniqueKeyContext.empty())
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _lockUniqueIndexKeys();
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dmlContext::_lockUniqueIndexKeys()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(requestContext::isOpen(), "can not be closed");
      SDB_ASSERT(_uniqueKeyContext.empty(), "must be empty");
      SDB_ASSERT(requestContext::getSpaceID() != INVALID_SPACE_ID, "impossible");
      uniqueIndexLatchKey key;
      UNIQUE_INDEX_LATCH_MAP::object obj;
      UNIQUE_INDEX_LATCH_MAP &latchMap = requestContext::getEnv()->uniqueIndexLathMap;

      if (_uniqueKeyHash.empty())
      {
         goto done;
      }
      if (1 < _uniqueKeyHash.size())
      {
         std::sort(_uniqueKeyHash.begin(), _uniqueKeyHash.end());
      }

      key = uniqueIndexLatchKey(requestContext::getSpaceID(), _uniqueKeyHash.at(0));
      rc = latchMap.ensure(key, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get latch obj:%d", rc);
         goto error;
      }
      _uniqueKeyContext.push_back(obj);

      for (UINT32 i = 1; i < _uniqueKeyHash.size(); ++i)
      {
         if (key.getKeyHash() == _uniqueKeyHash.at(i))
         {
            continue;
         }

         key = uniqueIndexLatchKey(requestContext::getSpaceID(), _uniqueKeyHash.at(i));
         rc = latchMap.ensure(key, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get latch obj:%d", rc);
            goto error;
         }
         _uniqueKeyContext.push_back(obj);
      }

      do
      {
         INT32 locked = 0;
         for (UINT32 i = 0; i < _uniqueKeyContext.size(); ++i)
         {
            UNIQUE_INDEX_LATCH_MAP::object &latch = _uniqueKeyContext[i];
            if (latch.getValue().try_get())
            {
               ++locked;
            }
            else
            {
               break;
            }
         }

         if (locked == (INT32)_uniqueKeyContext.size())
         {
            break;
         }

         for (; locked > 0; --locked)
         {
            _uniqueKeyContext[locked - 1].getValue().release();
         }
      } while (TRUE);
      

   done:
      return rc;
   error:
      for (UINT32 i = 0; i < _uniqueKeyContext.size(); ++i)
      {
         latchMap.release(_uniqueKeyContext.at(i));
      }
      _uniqueKeyContext.clear();
      goto done;
   }

   void dmlContext::unlockUniqueKeys()
   {
      SDB_ASSERT(requestContext::isOpen(), "can not be closed");
      UNIQUE_INDEX_LATCH_MAP &latchMap = requestContext::getEnv()->uniqueIndexLathMap;
      _UNIQUE_KEY_CONTEXT::reverse_iterator ritr = _uniqueKeyContext.rbegin();
      for (; ritr != _uniqueKeyContext.rend(); ++ritr)
      {
         ritr->getValue().release();
         latchMap.release(*ritr);
      }
      _uniqueKeyContext.clear();
      return;
   }
}//namespace vessel
}//namespace engine