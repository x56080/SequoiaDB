/*******************************************************************************


   Copyright (C) 2011-present SequoiaDB Ltd.

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

******************************************************************************/
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
