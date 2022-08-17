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

   Source File Name = logicalPageBufferPte.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
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
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel

} // namespace engine
