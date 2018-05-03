/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = qgmPlHashJoin.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPLHASHJOIN_HPP_
#define QGMPLHASHJOIN_HPP_

#include "qgmPlJoin.hpp"
#include "qgmHashTable.hpp"

namespace engine
{
   enum QGM_HJ_FETCH_STATE
   {
      QGM_HJ_FETCH_STATE_BUILD = 0,
      QGM_HJ_FETCH_STATE_PROBE,
      QGM_HJ_FETCH_STATE_FIND,
   } ;

   class _qgmOptiNLJoin ;

   class _qgmPlHashJoin : public _qgmPlJoin
   {
   public:
      _qgmPlHashJoin( INT32 type ) ;
      virtual ~_qgmPlHashJoin() ;

   public:
      virtual string toString()const ;

      INT32 init( _qgmOptiNLJoin *opti ) ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

      INT32 _buildHashTbl() ;

   private:
      _qgmHashTable _hashTbl ;
      _qgmPlan *_build ;
      _qgmPlan *_probe ;
      const qgmField *_buildAlias ;
      const qgmField *_probeAlias ;
      std::string _buildKey ;
      std::string _probeKey ;
      QGM_HT_CONTEXT _hashContext ;
      _qgmFetchOut _buildF ;
      _qgmFetchOut _probeF ;
      BSONElement _probeEle ;
      QGM_HJ_FETCH_STATE _state ;
      BOOLEAN _hitBuildEnd ;
   } ;
}

#endif

