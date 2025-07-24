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

   Source File Name = rtnAlterFuncs.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_ALTERFUNCS_HPP_
#define RTN_ALTERFUNCS_HPP_

#include "rtnAlterDef.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _dpsLogWrapper ;
   /// see func's name in msgDef.hpp

   /// SDB_ALTER_CRT_ID_INDEX
   INT32 rtnCreateIDIndex( const CHAR *name,
                           const bson::BSONObj &pubArgs,
                           const bson::BSONObj &args,
                           _pmdEDUCB *cb,
                           _dpsLogWrapper *dpsCB ) ;

   INT32 rtnCreateIDIndexVerify( const bson::BSONObj &args ) ;

   /// SDB_ALTER_DROP_ID_INDEX
   INT32 rtnDropIDIndex( const CHAR *name,
                         const bson::BSONObj &pubArgs,
                         const bson::BSONObj &args,
                         _pmdEDUCB *cb,
                         _dpsLogWrapper *dpsCB ) ;

   INT32 rtnDropIDIndexVerify( const bson::BSONObj &args )
   {
      return SDB_OK ;
   }
}

#endif

