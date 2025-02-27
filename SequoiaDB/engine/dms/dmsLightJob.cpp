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

   Source File Name = dmsLightJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/12/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsLightJob.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "pmd.hpp"

namespace engine
{
   /*
      _dmsNoMetaSuFilter define and implement
   */
   class _dmsNoMetaSuFilter : public _dmsCSFilter
   {
      public:
         _dmsNoMetaSuFilter( UINT64 noWriteMS )
         {
            _noWriteMS = noWriteMS ;
         }

         virtual ~_dmsNoMetaSuFilter()
         {
         }

         virtual BOOLEAN filter( _dmsStorageUnit *su )
         {
            if ( su->metaFile()->isValid() )
            {
               return FALSE ;
            }
            else if ( pmdGetTickSpanTime( su->getLastWriteDBTick() ) < _noWriteMS )
            {
               return FALSE ;
            }
            return TRUE ;
         }

      private:
         UINT64      _noWriteMS ;
   } ;

   #define DMS_SAVEMETA_NOWRITE_TIME         ( 24 * 3600LL * OSS_ONE_SEC )
   #define DMS_SAVEMETA_SULOCK_TIME          ( 100 )

   /*
      _dmsSaveMetaJob implement
   */
   _dmsSaveMetaJob::_dmsSaveMetaJob()
   {
      _index = 0 ;
   }

   _dmsSaveMetaJob::~_dmsSaveMetaJob()
   {
   }

   const CHAR* _dmsSaveMetaJob::name() const
   {
      return "Save DMS Meta" ;
   }

   INT32 _dmsSaveMetaJob::_dumpCS()
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB  *pDmsCB = sdbGetDMSCB() ;
      _dmsNoMetaSuFilter filter( DMS_SAVEMETA_NOWRITE_TIME ) ;

      _vecCS.clear() ;
      _index = 0 ;
      _dumpOK = TRUE ;

      rc = pDmsCB->dumpInfo( _vecCS, FALSE, &filter ) ;
      if ( rc )
      {
         _dumpOK = FALSE ;
      }
      return rc ;
   }

   INT32 _dmsSaveMetaJob::doit( IExecutor *pExe,
                                UTIL_LJOB_DO_RESULT &result,
                                UINT64 &sleepTime )
   {
      INT32 rcTmp = SDB_OK ;
      SDB_DMSCB  *pDmsCB = sdbGetDMSCB() ;

      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      sleepTime = 24 * 3600LL * 1000000 ;   /// 1 day
      result = UTIL_LJOB_DO_CONT ;

      if ( PMD_IS_DB_DOWN() || pExe->isForced() )
      {
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      if ( _vecCS.empty() )
      {
         _dumpCS() ;
      }

      while ( _index < _vecCS.size() )
      {
         monCSName &item = _vecCS[ _index ] ;
         rcTmp = pDmsCB->nameToSUAndLock( item._csName, suID, &su, SHARED,
                                          DMS_SAVEMETA_SULOCK_TIME ) ;
         if ( SDB_TIMEOUT == rcTmp )
         {
            sleepTime = 5 * 1000000 ;    /// 5 secs
            goto done ;
         }
         else if ( rcTmp )
         {
            /// ignore error
            ++_index ;
            continue ;
         }

         if ( su->metaFile()->isValid() ||
              pmdGetTickSpanTime( su->getLastWriteDBTick() ) < DMS_SAVEMETA_NOWRITE_TIME )
         {
            /// skip
            pDmsCB->suUnlock( suID ) ;
            su = NULL ;
            suID = DMS_INVALID_SUID ;

            ++_index ;
            continue ;
         }

         su->saveMeta() ;
         pDmsCB->suLock( suID ) ;
         su = NULL ;
         suID = DMS_INVALID_SUID ;

         ++_index ;
         break ;
      }

      if ( _index >= _vecCS.size() )
      {
         /// clear
         _vecCS.clear() ;
         _index = 0 ;

         if ( !_dumpOK )
         {
            sleepTime = 60 * 1000000 ;    /// 1 min
         }
      }
      else
      {
         sleepTime = 500000 ;    /// 500 ms
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         pDmsCB->suUnlock( suID ) ;
      }
      return SDB_OK ;
   }

   INT32 dmsStartSaveMetaJob()
   {
      INT32 rc = SDB_OK ;
      _dmsSaveMetaJob *pJob = NULL ;

      pJob = SDB_OSS_NEW dmsSaveMetaJob() ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate save meta job failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = pJob->submit( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Submit dmsSaveMetaJob failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}


