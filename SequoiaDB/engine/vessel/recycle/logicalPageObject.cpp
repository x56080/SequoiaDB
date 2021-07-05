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

   Source File Name = logicalPageObject.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageObject.h"

namespace engine
{
namespace vessel
{
   logicalPageObject::logicalPageObject()
   {}

   logicalPageObject::~logicalPageObject()
   {

   }

   void logicalPageObject::fini()
   {
      _pageId = mappedLogicalPageId();
      _psv = INVALID_PAGE_SNAPSHOT_VERSION;
      _mmapPtr = 0;
      return;
   }

   INT32 logicalPageObject::init(PAGE_ID lpid,
                                 PAGE_ID pid,
                                 PAGE_SNAPSHOT_VERION psv,
                                 ossValuePtr ptr)
   {
      INT32 rc = SDB_OK;
      if (isValid())
      {
         fini();
      }

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                       INVALID_PAGE_ID == pid ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       0 == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pageId = mappedLogicalPageId(lpid, pid);
      _psv = psv;
      _mmapPtr = ptr;
   done:
      return rc;
   error:
      goto done;
   }
}//namesapce vessel
}//namespace engine