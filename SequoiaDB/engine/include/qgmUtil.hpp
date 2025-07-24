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

   Source File Name = qgmUtil.hpp

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

*******************************************************************************/
#ifndef QGMUTIL_HPP_
#define QGMUTIL_HPP_

#include "core.hpp"
#include "qgmOptiTree.hpp"
#include "qgmPlan.hpp"
#include "qgmOptiAggregation.hpp"

using namespace bson ;

namespace engine
{

   struct _qgmConditionNode ;

   BOOLEAN qgmUtilFirstDot( const CHAR *str, UINT32 len, UINT32 &pos ) ;
   BOOLEAN qgmUtilLastDot( const CHAR *str, UINT32 len, UINT32 &pos ) ;

   BOOLEAN qgmUtilSame( const CHAR *src, UINT32 srcLen,
                        const CHAR *dst, UINT32 dstLen ) ;

   INT32 qgmFindFieldFromFunc( const CHAR *str, UINT32 size,
                               _qgmField &func,
                               vector<qgmOpField> &params,
                               _qgmPtrTable *table,
                               BOOLEAN needRele ) ;

   BOOLEAN isFromOne( const qgmOpField &left, const qgmOPFieldVec &right,
                      BOOLEAN useAlias = TRUE, UINT32 *pPos = NULL ) ;
   BOOLEAN isSameFrom( const qgmOPFieldVec &left, const qgmOPFieldVec &right ) ;

   BOOLEAN isFrom( const qgmDbAttr &left, const qgmOpField &right,
                   BOOLEAN useAlias = TRUE ) ;
   BOOLEAN isFromOne( const qgmDbAttr &left, const qgmOPFieldVec &right,
                      BOOLEAN useAlias = TRUE, UINT32 *pPos = NULL ) ;

   BSONObj qgmMerge( const BSONObj &left, const BSONObj &right ) ;

   string qgmPlanType( QGM_PLAN_TYPE type ) ;


   BOOLEAN isWildCard( const qgmOPFieldVec &fields ) ;
   void  replaceFieldRele( qgmOPFieldVec &fields, const qgmField &newRele ) ;
   void  replaceAttrRele( qgmDbAttrPtrVec &attrs,  const qgmField &newRele ) ;
   void  replaceAttrRele( qgmDbAttrVec &attrs, const qgmField &newRele ) ;
   void  replaceAggrRele( qgmAggrSelectorVec &aggrs, const qgmField &newRele ) ;

   void  clearFieldAlias( qgmOPFieldVec &fields ) ;

   INT32 downFieldsByFieldAlias( qgmOPFieldVec &fields,
                                 const qgmOPFieldPtrVec & fieldAlias,
                                 BOOLEAN needCopyAlias,
                                 BOOLEAN isOptional ) ;
   INT32 downAttrsByFieldAlias( qgmDbAttrPtrVec &attrs,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional ) ;
   INT32 downAttrsByFieldAlias( qgmDbAttrVec &attrs,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional ) ;
   INT32 downAAttrByFieldAlias( qgmDbAttr &attr,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional ) ;
   INT32 downAggrsByFieldAlias( qgmAggrSelectorVec &aggrs,
                                const qgmOPFieldPtrVec &fieldAlias,
                                BOOLEAN isOptional ) ;

   INT32 upFieldsByFieldAlias( qgmOPFieldVec &fields,
                               const qgmOPFieldPtrVec & fieldAlias,
                               BOOLEAN needClearAlias ) ;
   INT32 upAttrsByFieldAlias( qgmDbAttrPtrVec &attrs,
                              const qgmOPFieldPtrVec & fieldAlias ) ;
   INT32 upAttrsByFieldAlias( qgmDbAttrVec &attrs,
                              const qgmOPFieldPtrVec & fieldAlias ) ;
   INT32 upAAttrByFieldAlias( qgmDbAttr &attr,
                              const qgmOPFieldPtrVec & fieldAlias ) ;
   INT32 upAggrsByFieldAlias( qgmAggrSelectorVec &aggrs,
                              const qgmOPFieldPtrVec & fieldAlias ) ;

   string qgmHintToString( const QGM_HINS &hint ) ;

   BSONObj qgmUseIndexHintToBson( const qgmHint &h ) ;

   const CHAR* qgmGetNodeTypeStr( INT32 type ) ;

   INT32    qgmBuildANodeItem( BSONObjBuilder &bb,
                               const CHAR *pKeyName,
                               const _qgmConditionNode *node ) ;

}

#endif

