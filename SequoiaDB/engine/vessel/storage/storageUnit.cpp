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

   Source File Name = storageUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageUnit.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "utilStr.hpp"
#include "vessel/mainDataSpace.h"
#include "vessel/storageFileLoader.h"
#include "vessel/storageUtils.h"
#include "vessel/storageFileMaintainer.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   storageUnit::storageUnit():
   _los(&_manifest)
   {

   }

   storageUnit::~storageUnit()
   {
      close();
   }

   void storageUnit::close()
   {
      if (isOpen())
      {
         _mds.close();
         _is.close();
         _los.close();

         _manifest.reset();
      }
      return;
   }

   INT32 storageUnit::destroy()
   {
      INT32 rc = SDB_OK;
      storageFileMaintainer sfm;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      if (!isOpen())
      {
         goto done;
      }

      sfm.init(&po, _manifest.sid);

      _mds.destroy();
      _is.destroy();
      _los.destroy();

      sfm.removeSpaceDir();
      _manifest.reset();
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 storageUnit::create(SPACE_ID sid,
                             const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      BOOLEAN dirCreated = FALSE;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be invalid");
      const storagePathOptions &po = tc->getEnv()->options.path;
      storageFileMaintainer sfm;

      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      _manifest.sid = sid;
      _manifest.secretValue = ossRand();
      _manifest.dataArgs = options.dataArgs;
      _manifest.idxArgs = options.indexArgs;
      _manifest.lobArgs = options.lobArgs;

      sfm.init(&po, sid);

      rc = sfm.createSpaceDir();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create space[%d] dir, rc:%d", sid, rc);
         goto error;
      }
      dirCreated = TRUE;

      rc = createMainDataSpace();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }

      rc = createIndexSpace();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create index space:%d", rc);
         _mds.destroy();
         goto error;
      }

      /// delay to create lob space
   done:
      return rc;
   error:
      if (dirCreated)
      {
         sfm.removeSpaceDir();
      }
      
      _manifest.reset();
      goto done;
   }

   INT32 storageUnit::open(SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");

      storageFileLoader loader;
      storageFileMaintainer sfm;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path; 

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      /// TODO: init manifest from MANIFEST file
      _manifest.sid = sid;
      
      sfm.init(&po, sid);

      rc = sfm.testBeforeOpenning();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test space[%d] dir, rc:%d", sid, rc);
         goto error;
      }

      rc = sfm.load(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load files under space[%d]:%d", sid, rc);
         goto error;
      }

      rc = _mds.open(sid, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data space:%d", rc);
         goto error;
      }

      rc = _is.open(sid, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index space:%d", rc);
         goto error;
      }

      _manifest.dataArgs = _mds.getStorageCoreArgs();
      _manifest.idxArgs = _is.getStorageCoreArgs();
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 storageUnit::createMainDataSpace()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_manifest.isValid(), "can not be invalid");
      createLpsOptions o;
      o.dataArgs = _manifest.dataArgs;
      o.secretValue = _manifest.secretValue;

      rc = _mds.create(_manifest.sid, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createIndexSpace()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_manifest.isValid(), "can not be invalid");
      createLpsOptions o;
      o.dataArgs = _manifest.idxArgs;
      o.secretValue = _manifest.secretValue;

      rc = _is.create(_manifest.sid, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create main data space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::getMmapPagePointer(SPACE_TYPE spaceType,
                                          FILE_TYPE fileType,
                                          PAGE_ID pid,
                                          mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_SPACE_TYPE == spaceType ||
               INVALID_FILE_TYPE == fileType ||
               INVALID_PAGE_ID == pid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (SPACE_TYPE_MAIN_DATA == spaceType)
      {
         rc = _mds.getPagePtr(fileType, pid, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (SPACE_TYPE_IDX == spaceType)
      {
         SDB_ASSERT(FALSE, "todo");
      }
      else if (SPACE_TYPE_LOB == spaceType)
      {
         SDB_ASSERT(FALSE, "todo");
      }
      else
      {
         PD_LOG(PDERROR, "invalid space type[%d]", spaceType);
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine