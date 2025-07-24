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

   Source File Name = indexKeyGenerator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexKeyGenerator.h"
#include "pdTrace.hpp"
#include "ixmIndexKey.hpp"

namespace engine
{
namespace vessel
{
   INT32 indexKeyGenForBsonRecord(const bson::BSONObj &pattern,
                                  BOOLEAN notArray,
                                  const slice &record,
                                  bson::BSONObjSet &keys)
   {
      INT32 rc = SDB_OK;
      _ixmIndexKeyGen keygen(pattern);
      bson::BSONObj obj(record.data());

      rc = keygen.getKeys(obj, keys);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to generate keys:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
