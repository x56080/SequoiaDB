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

   Source File Name = dmsDictJob.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/29/2022  ZHY Moved from dmsCB.hpp

   Last Changed =

*******************************************************************************/
#include "dmsStorageUnit.hpp"

namespace engine
{
   struct _dmsDictJob
   {
      dmsStorageUnitID _suID;
      UINT32 _suLID;
      UINT16 _clID;
      UINT32 _clLID;
      UINT64 _recordNum;
      UINT64 _lastWriteTick;

      _dmsDictJob()
      : _suID( DMS_INVALID_SUID )
      , _suLID( DMS_INVALID_SUID )
      , _clID( DMS_INVALID_CLID )
      , _clLID( DMS_INVALID_CLID )
      , _recordNum( 0 )
      , _lastWriteTick( 0 )
      {
      }

      _dmsDictJob( dmsStorageUnitID suID, UINT32 suLID, UINT16 clID, UINT32 clLID )
      : _suID( suID )
      , _suLID( suLID )
      , _clID( clID )
      , _clLID( clLID )
      , _recordNum( 0 )
      , _lastWriteTick( 0 )
      {
      }
   };
   typedef _dmsDictJob dmsDictJob;
} // namespace engine
