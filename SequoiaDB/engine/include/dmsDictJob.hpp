/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY{} without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
