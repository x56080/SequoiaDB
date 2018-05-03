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

   Source File Name = qgmPlMthMatcherFilter.hpp

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

#ifndef QGMPLMTHMATCHERFILTER_HPP__
#define QGMPLMTHMATCHERFILTER_HPP__

#include "qgmPlFilter.hpp"
#include "mthMatchTree.hpp"

namespace engine
{
   class qgmPlMthMatcherFilter : public _qgmPlFilter
   {
   public:
      qgmPlMthMatcherFilter( const qgmOPFieldVec &selector,
                           INT64 numSkip,
                           INT64 numReturn,
                           const qgmField &alias );

      INT32 loadPattern( bson::BSONObj matcher );

   private:
      virtual INT32 _fetchNext( qgmFetchOut & next );

   private:
      _mthMatchTree        _matcher;
   };
}

#endif