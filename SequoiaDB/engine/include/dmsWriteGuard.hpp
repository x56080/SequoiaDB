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

   Source File Name = dmsWriteGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_WRITE_GUARD_HPP_
#define SDB_DMS_WRITE_GUARD_HPP_

#include "ossUtil.hpp"
#include "dms.hpp"
#include "ossRWMutex.hpp"
#include "dmsMetadata.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;

   /*
      dmsIndexBuildLockPtr define
    */
   typedef std::shared_ptr<ossRWMutex> dmsIndexBuildLockPtr ;
   typedef ossPoolMap<dmsIdxMetadataKey, dmsIndexBuildLockPtr> dmsIdxBuildLockMap ;
   typedef dmsIdxBuildLockMap::iterator dmsIdxBuildLockMapIter ;
   /*
      _dmsIndexWriteGuard define
    */
   class _dmsIndexWriteGuard : public SDBObject
   {
   public:
      _dmsIndexWriteGuard( _pmdEDUCB *cb, BOOLEAN isEnabled = TRUE ) ;
      ~_dmsIndexWriteGuard() ;

      INT32 lock( const dmsIdxMetadataKey &metadataKey,
                  dmsIndexBuildLockPtr &lockPtr ) ;
      void releaseAll() ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

      void setEnabled( BOOLEAN isEnabled )
      {
         _isEnabled = isEnabled ;
      }

   protected:
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      dmsIdxBuildLockMap _locks ;
   } ;

   typedef class _dmsIndexWriteGuard dmsIndexWriteGuard ;

}

#endif // SDB_DMS_WRITE_GUARD_HPP_
