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

   Source File Name = rtnCSHandlers.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#include "rtnCSHandlers.hpp"
#include "pdTrace.hpp"
#include "pmd.hpp"
#include "dmsEngineCB.hpp"

namespace engine
{
//////////rtnCreateCSHandler begin
   void rtnCreateCSHandler::init(const CHAR *name,
                                 const utilCSUniqueID &uniqueId,
                                 const dmsCreateCSOptions &o)
   {
      SDB_ASSERT(NULL != name, "can not be null");
      _name = name;
      _uniqueId = uniqueId;
      _o = o;
   }

   INT32 rtnCreateCSHandler::launch(pmdEDUCB *cb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != cb, "can not be null");
      SDB_ASSERT(NULL != _name, "can not be null");

      SDB_DMS_ENGINE_CB *engineCB = pmdGetKRCB()->getDMSEngineCB();
      IDataStorageEngine *engine = engineCB->getEngine();

      rc = dmsCheckCSName(_name, _syscall);
      if ( rc )
      {
         PD_LOG ( PDERROR, "Invalid collection space name, rc = %d",
                  rc ) ;
         goto error ;
      }

      rc = engine->createCS(cb, _name, _uniqueId,
                            _o, _adjunct);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cs[%s, %d], rc:%d",
                _name, _uniqueId, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
//////////rtnCreateCSHandler end

//////////rtnTestCSHandler begin
   INT32 rtnTestCSHandler::launch(pmdEDUCB *cb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != cb, "can not be null");

      SDB_DMS_ENGINE_CB *engineCB = pmdGetKRCB()->getDMSEngineCB();
      IDataStorageEngine *engine = engineCB->getEngine();

      _output = UTIL_UNIQUEID_NULL;

      if (UTIL_IS_VALID_CSUNIQUEID(_input))
      {
         rc = engine->testCS(cb, _input);
         if (SDB_OK != rc)
         {
            goto error;
         }
         _output = _input;
      }
      else if (NULL != _name)
      {
         utilCSUniqueID output = UTIL_UNIQUEID_NULL;
         rc = engine->testCS(cb, _name, output);
         if (SDB_OK != rc)
         {
            goto error;
         }
         
         _output = output;
      }
   done:
      return rc;
   error:
      goto done;
   }

//////////rtnTestCSHandler end
} // namespace engine
