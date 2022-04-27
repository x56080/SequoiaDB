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

   Source File Name = dmsEngineCB.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsEngineCB.hpp"
#include "pdTrace.hpp"
#include "pmd.hpp"
#include "vessel/api/vesselFactory.h"
#include "vessel/dummyJournal.h"

namespace engine
{
   SDB_DMS_ENGINE_CB *sdbGetDMSEngineCB()
   {
      static _dmsEngineCB engineCB;
      return &engineCB;
   }

   _dmsEngineCB::_dmsEngineCB()
   {

   }

   _dmsEngineCB::~_dmsEngineCB()
   {
      SDB_ASSERT(NULL == _engine, "must be null");
   }

   INT32 _dmsEngineCB::init()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _engine, "do not reinit");
      vessel::IVessel *instance = vessel::vesselFactroy().createInstance();
      if (NULL == instance)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      _engine = static_cast<IDataStorageEngine *>(instance);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsEngineCB::fini()
   {
      if (NULL != _engine)
      {
         vessel::IVessel *instance = static_cast<vessel::IVessel *>(_engine);
         vessel::vesselFactroy().releaseInstance(instance);
         _engine = NULL;
      }
      
      return SDB_OK;
   }

   INT32 _dmsEngineCB::active()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _engine, "init first");
      vessel::IVessel *instance = static_cast<vessel::IVessel *>(_engine);
      vessel::outerResource resource;
      resource.executorPool = pmdGetKRCB()->getExecutorMgr();
      resource.journal = vessel::dummyDataJournal::instance();
      resource.transLockConsole = pmdGetKRCB()->getTransCB()->getLockMgrHandle();

      vessel::openDBOptions o;
      pmdGetOptionCB()->makeOpenDBOptions(o);

      rc = instance->open(pmdGetThreadEDUCB(), &resource, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to active dms engine:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsEngineCB::deactive()
   {
      INT32 rc = SDB_OK;
      if (NULL != _engine)
      {
         vessel::IVessel *instance = static_cast<vessel::IVessel *>(_engine);
         vessel::closeDBOptions o;
         rc = instance->close(pmdGetThreadEDUCB(), o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to close db engine:%d", rc);
            goto error;
         }
      }
   
   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine
