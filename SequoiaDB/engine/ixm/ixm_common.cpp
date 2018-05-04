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

   Source File Name = ixm.cpp

   Descriptive Name = Index Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for
   Index Manager Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/07/2014  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ixm_common.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"

using namespace bson;

namespace engine
{

   /*
      Tool functions
   */
   void ixmMakeHashValue( const BSONObj &obj, const BSONObj &key,
                          ixmHashValue & hashValue )
   {
      hashValue.hash = 0 ;
      try
      {
         BSONObjIterator itr( key ) ;
         while ( itr.more() )
         {
            BSONElement orderEle = itr.next() ;
            const CHAR* name = orderEle.fieldName() ;
            BSONElement arrEle = obj.getFieldDottedOrArray( name ) ;
            if ( Array == arrEle.type() )
            {
               hashValue.columns.hash1 = ossHash( arrEle.value(),
                                                  arrEle.valuesize() ) ;
               hashValue.columns.hash2 = ossHash( arrEle.value(),
                                                  arrEle.valuesize(), 3 ) ;
               break ;
            }
         }
      }
      catch( std::exception & e )
      {
         PD_LOG( PDWARNING, "Make hash value occurred exception: %s",
                 e.what() ) ;
      }
   }

   void ixmMakeHashValue( const BSONElement &eleArray,
                          ixmHashValue &hashValue )
   {
      hashValue.columns.hash1 = ossHash( eleArray.value(),
                                         eleArray.valuesize() ) ;
      hashValue.columns.hash2 = ossHash( eleArray.value(),
                                         eleArray.valuesize(), 3 ) ;
   }

}

