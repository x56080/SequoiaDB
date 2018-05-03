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

   Source File Name = qgmPlMthMatcherScan.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/29/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPLMTHMATCHERSCAN_HPP__
#define QGMPLMTHMATCHERSCAN_HPP__

#include "qgmPlScan.hpp"

namespace engine
{
   class qgmPlMthMatcherScan : public _qgmPlScan
   {
   public:
      qgmPlMthMatcherScan( const qgmDbAttr &collection,
                           const qgmOPFieldVec &selector,
                           const bson::BSONObj &orderby,
                           const bson::BSONObj &hint,
                           INT64 numSkip,
                           INT64 numReturn,
                           const qgmField &alias,
                           const bson::BSONObj &matcher );

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;
   };
}

#endif
