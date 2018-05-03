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

   Source File Name = qgmOptiSelect.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  JHL  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMOPTIMTHMATCHSELECT_HPP_
#define QGMOPTIMTHMATCHSELECT_HPP_

#include "qgmOptiSelect.hpp"
#include "../bson/bsonobj.h"

namespace engine
{
   class qgmOptiMthMatchSelect : public _qgmOptiSelect
   {
   public:
      qgmOptiMthMatchSelect( _qgmPtrTable *pTable, _qgmParamTable *pParam );
      virtual ~qgmOptiMthMatchSelect();

      virtual BOOLEAN isEmpty();

      virtual INT32 init (){ return SDB_OK; }

      INT32 fromBson( const bson::BSONObj &matcher );

   protected:
      virtual INT32 _extend( _qgmOptiTreeNode *&exNode ) ;

   public:
      bson::BSONObj        _matcher;
   };
}

#endif
