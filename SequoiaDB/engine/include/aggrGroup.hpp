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

   Source File Name = aggrGroup.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2013  JHL  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef AGGRGROUP_HPP__
#define AGGRGROUP_HPP__

#include "aggrParser.hpp"
#include "qgmPtrTable.hpp"
#include "qgmOptiTree.hpp"
#include "qgmParamTable.hpp"
#include "qgmOptiAggregation.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   /*
      aggrGroupParser define
   */
   class aggrGroupParser : public aggrParser
   {
   private:
      INT32 buildNode( const BSONElement &elem,
                       const CHAR *pCLName,
                       qgmOptiTreeNode *&pNode,
                       _qgmPtrTable *pTable,
                       _qgmParamTable *pParamTable ) ;

      INT32 parseSelectorField( const BSONElement &beField,
                                const CHAR *pCLName,
                                qgmOPFieldVec &selectorVec,
                                _qgmPtrTable *pTable,
                                BOOLEAN &hasFunc ) ;

      INT32 addGroupByField( const CHAR *pFieldName,
                             qgmOPFieldVec &groupby,
                             _qgmPtrTable *pTable,
                             const CHAR *pCLName ) ;

      INT32 parseGroupbyField( const BSONElement &beId,
                               qgmOPFieldVec &groupby,
                               _qgmPtrTable *pTable,
                               const CHAR *pCLName ) ;

      INT32 parseInputFunc( const BSONObj &funcObj,
                            const CHAR *pCLName,
                            qgmField &funcField,
                            _qgmPtrTable *pTable ) ;
   } ;

}

#endif // AGGRGROUP_HPP__
