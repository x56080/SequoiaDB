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

   Source File Name = rtnCLHandlers.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "rtnCLHandlers.hpp"
#include "pdTrace.hpp"
#include "pmd.hpp"
#include "dmsEngineCB.hpp"

namespace engine
{
//////////rtnCreateCLHandler begin
   INT32 rtnCreateCLHandler::launch(pmdEDUCB *cb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != cb, "can not be null");
      SDB_ASSERT(NULL != _fullName, "can not be null");

      SDB_DMS_ENGINE_CB *engineCB = pmdGetKRCB()->getDMSEngineCB();
      IDataStorageEngine *engine = engineCB->getEngine();

      rc = engine->createCL(cb, _fullName, _uniqueId, _o, _adjunct);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cl[%s], rc:%d", _fullName, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

//////////rtnCreateCLHandler end

//////////rtnTestCLHandler  begine
   INT32 rtnTestCLHandler::launch(pmdEDUCB *cb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != cb, "can not be null");
      SDB_DMS_ENGINE_CB *engineCB = pmdGetKRCB()->getDMSEngineCB();
      IDataStorageEngine *engine = engineCB->getEngine();

      _output = UTIL_UNIQUEID_NULL;

      if (UTIL_IS_VALID_CLUNIQUEID(_input))
      {
         rc = engine->testCL(cb, _input);
         if (SDB_OK != rc)
         {
            goto error;
         }

         _output = _input;
      }
      else if (NULL != _fullName)
      {
         utilCLUniqueID out = UTIL_UNIQUEID_NULL;
         rc = engine->testCL(cb, _fullName, out);
         if (SDB_OK != rc)
         {
            goto error;
         }

         _output = out;
      }

   done:
      return rc;
   error:
      goto done;
   }

//////////rtnTestCLHandler  end
} // namespace engine

