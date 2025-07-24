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
#include "dmsDataWriteGuard.hpp"
#include "dmsIndexWriteGuard.hpp"
#include "dmsPersistGuard.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   /*
      _dmsWriteGuard define
    */
   class _dmsWriteGuard : public SDBObject
   {
   public:
      _dmsWriteGuard() = default ;
      _dmsWriteGuard( IStorageService *service,
                      _dmsStorageDataCommon *su,
                      _dmsMBContext *mbContext,
                      _pmdEDUCB *cb,
                      BOOLEAN isDataWriteGuardEnabled = TRUE,
                      BOOLEAN isIndexWriteGuardEnabled = TRUE,
                      BOOLEAN isPersistGuardEnabled = TRUE ) ;

      ~_dmsWriteGuard() = default ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      dmsDataWriteGuard &getDataWriteGuard()
      {
         return _dataGuard ;
      }

      dmsIndexWriteGuard &getIndexWriteGuard()
      {
         return _indexGuard ;
      }

      dmsPersistGuard &getPersistGuard()
      {
         return _persistGuard ;
      }

   protected:
      dmsDataWriteGuard _dataGuard ;
      dmsIndexWriteGuard _indexGuard ;
      dmsPersistGuard _persistGuard ;
   } ;

   typedef class _dmsWriteGuard dmsWriteGuard ;

}

#endif // SDB_DMS_WRITE_GUARD_HPP_
