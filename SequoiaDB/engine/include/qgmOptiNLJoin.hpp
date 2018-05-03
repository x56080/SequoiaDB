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

   Source File Name = qgmOptiNLJoin.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMOPTINLJOIN_HPP_
#define QGMOPTINLJOIN_HPP_

#include "qgmOptiTree.hpp"
#include "qgmConditionNode.hpp"

namespace engine
{
   /// TODO:we should not use nljoin here indeed.
   class _qgmOptiNLJoin : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiNLJoin( INT32 type, _qgmPtrTable *table,
                      _qgmParamTable *param ) ;
      virtual ~_qgmOptiNLJoin() ;

      virtual INT32        init () ;

      BOOLEAN              canSwapInnerOuter() const ;
      INT32                swapInnerOuter() ;
      BOOLEAN              needMakeCondition() const ;
      INT32                makeCondition() ;

   public:
      virtual INT32     outputSort( qgmOPFieldVec &sortFields ) ;
      virtual INT32     outputStream( qgmOpStream &stream ) ;
      virtual INT32     handleHints( QGM_HINS &hint ) ;
      virtual BOOLEAN   validateBeforeChange( QGM_OPTI_TYPE type ) const ;

      virtual string toString() const ;

      INT32 joinType() const { return _joinType ; }
      qgmOptiTreeNode* outer () { return *_outer ; }
      qgmOptiTreeNode* inner () { return *_inner ; }

   protected:
      virtual INT32 _pushOprUnit( qgmOprUnit *oprUnit, PUSH_FROM from ) ;
      virtual INT32 _removeOprUnit( qgmOprUnit *oprUnit ) ;
      virtual INT32 _updateChange( qgmOprUnit *oprUnit ) ;

      INT32   _makeCondVar( qgmConditionNode *cond ) ;
      INT32   _makeOuterInner () ;

      INT32   _createJoinUnit() ;

   private:
      virtual INT32 _extend( _qgmOptiTreeNode *&exNode ) ;

      INT32 _validate() ;

      INT32 _validateHint() ;

      INT32 _handleHints( _qgmOptiTreeNode *sub,
                          const QGM_HINS &hint ) ;

   public:
      INT32 _joinType ;
      qgmConditionNode *_condition ;
      qgmOptiTreeNode **_outer ;
      qgmOptiTreeNode **_inner ;
      QGM_VARLIST       _varList ;
      BOOLEAN           _hasMakeVar ;
      BOOLEAN           _hasPushSort ;
      qgmField          _uniqueNameR ;
      qgmField          _uniqueNameL ;
   } ;
   typedef class _qgmOptiNLJoin qgmOptiNLJoin ;
}

#endif

