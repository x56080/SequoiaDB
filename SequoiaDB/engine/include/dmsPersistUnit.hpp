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

   Source File Name = dmsPersistUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_PERSIST_UNIT_HPP_
#define SDB_DMS_PERSIST_UNIT_HPP_

#include "dmsDef.hpp"
#include "interface/IPersistUnit.hpp"

namespace engine
{

   /*
      dmsPersistUnitState define
    */
   enum class _dmsPersistUnitState
   {
      INACTIVE,
      ACTIVE,
      ACTIVE_IN_TRANS,
      PREPARED,
   } ;
   typedef enum _dmsPersistUnitState dmsPersistUnitState ;

   /*
      _dmsPersistUnit define
    */
   class _dmsPersistUnit : public IPersistUnit
   {
   public:
      _dmsPersistUnit() ;
      virtual ~_dmsPersistUnit() ;
      _dmsPersistUnit( const _dmsPersistUnit &o ) = delete ;
      _dmsPersistUnit &operator =( const _dmsPersistUnit & ) = delete ;

   public:
      virtual INT32 beginUnit( IExecutor *executor,
                               BOOLEAN isTrans ) ;
      virtual INT32 prepareUnit( IExecutor *executor,
                                 BOOLEAN isTrans ) ;
      virtual INT32 commitUnit( IExecutor *executor,
                                BOOLEAN isTrans ) ;
      virtual INT32 abortUnit( IExecutor *executor,
                               BOOLEAN isTrans,
                               BOOLEAN isForced ) ;

      virtual BOOLEAN useAtomicAbort() const
      {
         return dmsPersistUnitState::INACTIVE != _state &&
                _isAtomicSupported() ;
      }

   protected:
      virtual INT32 _beginUnit( IExecutor *executor ) = 0 ;
      virtual INT32 _prepareUnit( IExecutor *executor ) = 0 ;
      virtual INT32 _commitUnit( IExecutor *executor ) = 0 ;
      virtual INT32 _abortUnit( IExecutor *executor ) = 0 ;

      virtual BOOLEAN _isTransSupported() const = 0 ;
      virtual BOOLEAN _isAtomicSupported() const = 0 ;

   protected:
      dmsPersistUnitState _state = dmsPersistUnitState::INACTIVE ;
      UINT32 _activeLevel = 0 ;
   } ;

   typedef class _dmsPersistUnit dmsPersistUnit ;

}

#endif // SDB_DMS_PERSIST_UNIT_HPP_