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

   Source File Name = aggrUnwindParser.hpp

   Descriptive Name = Parser for $unwind in aggregation.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/05/2023  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AGGRUNWINDPARSER_HPP
#define AGGRUNWINDPARSER_HPP

#include "aggrParser.hpp"

using namespace bson ;

namespace engine
{
   class aggrUnwindParser : public aggrParser
   {
   private:
      INT32 buildNode( const BSONElement &elem,
                       const CHAR *pCLName,
                       BSONObj &hint,
                       qgmOptiTreeNode *&pNode,
                       _qgmPtrTable *pTable,
                       _qgmParamTable *pParamTable ) ;

      /**
         * @brief parse unwind field path: { $unwind: "$<field name>" }
         *
         * @param pFieldPath Format: $<field name>
         * @param splitby
         * @param pTable
         * @param pCLName
         *
         * @return
      */
      INT32 _parseUnwindFieldPath( const CHAR *pFieldPath,
                                   qgmDbAttr &splitby,
                                   _qgmPtrTable *pTable,
                                   const CHAR *pCLName ) ;

      /**
         * @brief parse unwind option, it should be an object.
         *
         * @param option Format: { Path: <field path> [, PreserveNullAndEmptyArrays: true|false,
         *                         IncludeArrayIndex: <name>]}
         * @param splitby
         * @param pTable
         * @param pCLName
         * @param builder
         *
         * @return
      */
      INT32 _parseUnWindOption( const BSONObj &option,
                                qgmDbAttr &splitby,
                                _qgmPtrTable *pTable,
                                const CHAR *pCLName,
                                BSONObjBuilder &builder ) ;

   private:
      qgmDbAttr _splitby ;
   } ;
}

#endif /* AGGRUNWINDPARSER_HPP */
