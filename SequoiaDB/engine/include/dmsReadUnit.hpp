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

   Source File Name = dmsReadUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_READ_UNIT_HPP_
#define SDB_DMS_READ_UNIT_HPP_

#include "dmsDef.hpp"
#include "interface/IReadUnit.hpp"
#include "pmdDummySession.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;

   /*
      _dmsReadUnit define
    */
   class _dmsReadUnit : public IReadUnit
   {
   public:
      _dmsReadUnit( IStorageSession *session )
      : _session( session )
      {
      }

      virtual ~_dmsReadUnit() = default ;

      IStorageSession *getSession()
      {
         return _session ;
      }

   protected:
      IStorageSession *_session ;
   } ;

   typedef class _dmsReadUnit dmsReadUnit ;

   /*
      _dmsReadUnitScope define
    */
   class _dmsReadUnitScope
   {
   public:
      _dmsReadUnitScope( IStorageSession *session, _pmdEDUCB *cb ) ;
      ~_dmsReadUnitScope() ;

   protected:
      _pmdEDUCB *_cb = nullptr ;
      dmsReadUnit _currentReadUnit ;
      IReadUnit *_lastReadUnit = nullptr ;
      pmdDummySession _dummySession ;
      BOOLEAN _attached = FALSE ;
   } ;
   typedef class _dmsReadUnitScope dmsReadUnitScope ;

}

#endif // SDB_DMS_READ_UNIT_HPP_