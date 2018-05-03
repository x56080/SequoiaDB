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

   Source File Name = sqlUtil.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sqlUtil.hpp"
#include "../bson/bson.h"
#include "pdTrace.hpp"
#include "sqlTrace.hpp"

using namespace bson ;

namespace engine
{
   static void dump( const SQL_CONTAINER &c,
                          INT32 indent )
   {
      for ( SQL_CONTAINER::const_iterator itr = c.begin();
            itr != c.end();
            itr++ )
      {
         cout << string( indent * 4, ' ') << "|--(value:" <<
                 string( itr->value.begin(), itr->value.end())
                 << " id:"<< itr->value.id().to_long()<< ")" << endl ;
         dump( itr->children, indent + 1) ;
      }
   }


   void sqlDumpAst( const SQL_CONTAINER &c )
   {
      dump( c, 0 ) ;
   }
}
