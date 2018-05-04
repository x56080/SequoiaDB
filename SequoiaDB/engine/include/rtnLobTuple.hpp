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

   Source File Name = rtnLobTuple.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBTUPLE_HPP
#define RTN_LOBTUPLE_HPP

#include "msg.h"
#include <list>

namespace engine
{
   struct _rtnLobTuple
   {
      MsgLobTuple tuple ;
      const CHAR *data ;

      _rtnLobTuple( UINT32 len,
                    UINT32 sequence,
                    SINT64 offset,
                    const CHAR *d )
      :data( d )
     {
        tuple.columns.len = len ;
        tuple.columns.sequence = sequence ;
        tuple.columns.offset = offset ; 
     }

     _rtnLobTuple()
      :data( NULL )
     {
        tuple.columns.len = 0 ;
        tuple.columns.sequence = 0 ;
        tuple.columns.offset = -1 ;
     }

     BOOLEAN empty() const
     {
        return 0 == tuple.columns.len ;
     }

     void clear()
     {
        tuple.columns.len = 0 ;
        tuple.columns.sequence = 0 ;
        tuple.columns.offset = -1 ;
        data = NULL ;
     }
   } ;

   typedef std::list<_rtnLobTuple> RTN_LOB_TUPLES ;
}

#endif

