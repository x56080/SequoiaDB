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

   Source File Name = aggrParser.hpp

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
#ifndef AGGRPARSER_HPP__
#define AGGRPARSER_HPP__

#include "qgmPtrTable.hpp"
#include "qgmOptiTree.hpp"
#include "qgmParamTable.hpp"
#include "qgmOptiAggregation.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   class _qgmOptiTreeNode;
   class _qgmPtrTable;
   class _qgmParamTable;

   /*
      aggrParser define
   */
   class aggrParser : public SDBObject
   {
   public:
      virtual ~aggrParser(){}
      // change to plan to parent node
      virtual INT32 parse( const BSONElement &elem,
                           _qgmOptiTreeNode *&root,
                           _qgmPtrTable * pPtrTable,
                           _qgmParamTable *pParamTable,
                           const CHAR *pCollectionName );

   private:
      virtual INT32 buildNode( const BSONElement &elem,
                               const CHAR *pCLName,
                               qgmOptiTreeNode *&pNode,
                               _qgmPtrTable *pTable,
                               _qgmParamTable *pParamTable ) = 0 ;
   };
}

#endif // AGGRPARSER_HPP__
