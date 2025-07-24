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

   Source File Name = IPersistUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_PERSIST_UNIT_HPP_
#define SDB_I_PERSIST_UNIT_HPP_

#include "sdbInterface.hpp"
#include "interface/IStorageSession.hpp"
#include "utilPooledAutoPtr.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      IStatPersistUnit define
    */
   class IStatPersistUnit : public _utilPooledObject
   {
   public:
      IStatPersistUnit() = default ;
      virtual ~IStatPersistUnit() = default ;
      IStatPersistUnit( const IStatPersistUnit &o ) = delete ;
      IStatPersistUnit &operator =( const IStatPersistUnit& ) = delete ;

   public:
      virtual INT32 commitUnit( IExecutor *executor ) = 0 ;
      virtual INT32 abortUnit( IExecutor *executor ) = 0 ;
   } ;

   /*
      IPersistUnit define
    */
   class IPersistUnit : public _utilPooledObject
   {
   public:
      IPersistUnit() = default ;
      virtual ~IPersistUnit() = default ;
      IPersistUnit( const IPersistUnit &o ) = delete ;
      IPersistUnit &operator =( const IPersistUnit& ) = delete ;

   public:
      virtual INT32 beginUnit( IExecutor *executor,
                               BOOLEAN isTrans ) = 0 ;
      virtual INT32 prepareUnit( IExecutor *executor,
                                 BOOLEAN isTrans ) = 0 ;
      virtual INT32 commitUnit( IExecutor *executor,
                                BOOLEAN isTrans ) = 0 ;
      virtual INT32 abortUnit( IExecutor *executor,
                               BOOLEAN isTrans,
                               BOOLEAN isForced ) = 0 ;

      virtual INT32 registerStatUnit( utilThreadLocalPtr<IStatPersistUnit> &statUnitPtr ) = 0 ;

      virtual BOOLEAN useAtomicAbort() const = 0 ;
   } ;

}

#endif // SDB_I_PERSIST_UNIT_HPP_