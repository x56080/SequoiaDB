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

   Source File Name = aggrSort.hpp

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
#ifndef AGGRSORT_HPP__
#define AGGRSORT_HPP__

#include "aggrParser.hpp"

using namespace bson ;

namespace engine
{
   /*
      aggrSortParser define
   */
   class aggrSortParser : public aggrParser
   {
   private:
      INT32 buildNode( const BSONElement &elem,
                       const CHAR *pCLName,
                       qgmOptiTreeNode *&pNode,
                       _qgmPtrTable *pTable,
                       _qgmParamTable *pParamTable ) ;

      INT32 buildOrderBy( const BSONElement &elem,
                          qgmOPFieldVec &orderBy,
                          _qgmPtrTable *pTable,
                          const CHAR *pCLName ) ;
   } ;

}

#endif // AGGRSORT_HPP__

