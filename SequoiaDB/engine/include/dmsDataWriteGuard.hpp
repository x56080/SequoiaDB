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

   Source File Name = dmsDataWriteGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_DATA_WRITE_GUARD_HPP_
#define SDB_DMS_DATA_WRITE_GUARD_HPP_

#include "ossUtil.hpp"
#include "dms.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;
   class _dmsMBStatInfo ;

   /*
      _dmsDataWriteGuard define
    */
   class _dmsDataWriteGuard : public SDBObject
   {
   public:
      _dmsDataWriteGuard() ;
      _dmsDataWriteGuard( _dmsStorageDataCommon *su,
                          _dmsMBContext *mbContext,
                          _pmdEDUCB *cb,
                          BOOLEAN isEnabled = TRUE ) ;
      ~_dmsDataWriteGuard() ;

      void beforeWrite() ;
      void afterWrite() ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

   protected:
      _dmsStorageDataCommon *_su = nullptr ;
      _dmsMBStatInfo *_mbStat = nullptr ;
      UINT16 _mbID ;
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      BOOLEAN _isInWrite ;
   } ;

   typedef class _dmsDataWriteGuard dmsDataWriteGuard ;

}

#endif // SDB_DMS_DATA_WRITE_GUARD_HPP_
