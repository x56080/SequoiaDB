/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = logicalPageBufferPte.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/logicalPageBufferPte.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/spacePteAccessCtx.h"
#include "vessel/logicalPageSpacePte.h"

namespace engine
{
namespace vessel
{
   logicalPageBufferPte::logicalPageBufferPte(logicalPageBufferPte &&o):
   logicalPageBuffer(std::move(o)),
   _ac(o._ac)
   {
      o._ac = nullptr;
   }

   logicalPageBufferPte &logicalPageBufferPte::operator=(logicalPageBufferPte &&o)
   {
      logicalPageBuffer::operator=(std::move(o));
      _ac = o._ac;
      o._ac = nullptr;
      return *this;
   }

   void logicalPageBufferPte::fini()
   {
      _ac = nullptr;
      logicalPageBuffer::_fini();
      return;
   }

   INT32 logicalPageBufferPte::prepareToWrite()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (nullptr == _ac)
      {
         SDB_ASSERT(FALSE, "has no access ctx");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else
      {
         logicalPageSpacePte *spacePte = static_cast<logicalPageSpacePte *>(_lps);
         rc = spacePte->makePrivateBuffer(_context, _ac, *this);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make private buffer:%d", rc);
            goto error;
         }

         SDB_ASSERT(isWritable(), "must be writable");
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel

} // namespace engine
