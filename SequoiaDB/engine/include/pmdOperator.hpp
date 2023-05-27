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

   Source File Name = pmdOperator.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Operaotr Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for EDU Control
   Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          11/15/2022  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMDOPERATOR_HPP__
#define PMDOPERATOR_HPP__

#include "sdbInterface.hpp"
#include "pmdEnv.hpp"

namespace engine
{

   /*
      _pmdOperator define
   */
   class _pmdOperator : public _IOperator
   {
   public:
      _pmdOperator()
      {
         _pMsg = NULL ;
         _maxTime = -1 ;
         _beginTick = 0 ;
         _hasInterruptOnTimeLimit = FALSE ;
      }
      virtual ~_pmdOperator()
      {
         _pMsg = NULL ;
      }

   public:
      virtual const MsgHeader* getMsg() const
      {
         return _pMsg ;
      }
      virtual const MsgGlobalID& getGlobalID() const
      {
         return _globalID ;
      }
      virtual void updateGlobalID( const MsgGlobalID &globalID )
      {
         _globalID = globalID ;
         if ( _pMsg )
         {
            _pMsg->globalID = _globalID ;
         }
      }
      /*
         <0 means no limit
      */
      virtual INT64 getRemainingMaxTime() const
      {
         if ( _maxTime < 0 )
         {
            return -1 ;
         }

         UINT64 timeSpent = pmdGetTickSpanTime( _beginTick ) ;
         if ( (UINT64)_maxTime > timeSpent )
         {
            return _maxTime - timeSpent ;
         }
         return 0 ;
      }
      virtual INT64 getMaxTime() const { return _maxTime ; }
      virtual void  setMaxTime( INT64 maxTime )
      {
         _maxTime = maxTime ;
         _beginTick = pmdGetDBTick() ;
      }
      virtual BOOLEAN needInterrupt() const
      {
         if ( !_hasInterruptOnTimeLimit && 0 == getRemainingMaxTime() )
         {
            _hasInterruptOnTimeLimit = TRUE ;
         }
         return _hasInterruptOnTimeLimit ;
      }
      virtual BOOLEAN isInterruptOnTimeLimit() const
      {
         return _hasInterruptOnTimeLimit ;
      }

   public:
      void setMsg( MsgHeader *pMsg )
      {
         if ( pMsg )
         {
            _pMsg = pMsg ;
            _globalID = _pMsg->globalID ;
         }
      }
      void reset()
      {
         _pMsg = NULL ;
         _maxTime = -1 ;
         _beginTick = 0 ;
         _hasInterruptOnTimeLimit = FALSE ;
      }

   private:
      MsgHeader*  _pMsg ;
      MsgGlobalID _globalID ;
      INT64       _maxTime ;     /// ms
      UINT64      _beginTick ;
      mutable BOOLEAN     _hasInterruptOnTimeLimit ;
   } ;
   typedef _pmdOperator pmdOperator ;

}

#endif // PMDOPERATOR_HPP__