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

   Source File Name = btreeIndexAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexAccessor.h"
#include "vessel/requestContext.h"
#include "vessel/indexContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePage.h"
#include "vessel/indexDefPageAccessor.h"
#include "vessel/btreeNodePageIniter.h"

namespace engine
{
namespace vessel
{
   btreeIndexAccessor::btreeIndexAccessor()
   {}

   btreeIndexAccessor::~btreeIndexAccessor()
   {
      _fini();
   }

   INT32 btreeIndexAccessor::_init(requestContext *context,
                                   indexContext *ic)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace *lps = NULL;

      _fini();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->getCollectionHandle().isValid() ||
                       NULL == ic ||
                       !ic->isValid() ||
                       ic->getObj().getParams().type != INDEX_TYPE_BTREE))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _ic = ic;

      rc = context->getEnv()->dms.getLogicalPageSpace(context->getSpaceID(),
                                                      SPACE_TYPE_IDX,
                                                      &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index space[%d]:%d", context->getSpaceID(), rc);
         goto error;
      }

      _is = static_cast<indexSpace *>(lps);
      _path.init(_ic, _is);

      rc = _is->blockCheckpoint(_context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      
      _checkpointBlocked = TRUE;
   done:
      return rc;
   error:
      _fini();
      goto done;
   }

   void btreeIndexAccessor::_fini()
   {
      if (NULL == _context)
      {
         goto done;
      }

      _path.fini();
      if (_checkpointBlocked)
      {
         _context->unblockCheckpoint();
         _checkpointBlocked = FALSE;
      }
      _context = NULL;
      _ic = NULL;
      _is = NULL;
   done:
      return;
   }
} // namespace vessel

} // namespace engine
