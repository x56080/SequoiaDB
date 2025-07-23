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

   Source File Name = rtnQueryModifier.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/3/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnQueryModifier.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   _rtnQueryModifier::_rtnQueryModifier()
   : _returnNew ( FALSE )
   {
   }

   INT32 _rtnQueryModifier::_parseModifyEle( const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( FIELD_NAME_RETURNNEW, ele.fieldName() ) )
      {
         if ( !ele.isBoolean() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Field[%s] in hint should be bool[%d]",
                    FIELD_NAME_RETURNNEW, rc ) ;
            goto error ;
         }
         _returnNew = ele.boolean() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnQueryModifier::_onInit()
   {
      INT32 rc = SDB_OK ;

      if ( RTN_MODIFY_UPDATE != _modifyOp )
      {
         goto done ;
      }

      if ( _opOption.isEmpty() )
      {
         PD_LOG ( PDERROR, "modifier can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         rc = _modifier.loadPattern ( _opOption,
                                      &_dollarList ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for updator: "
                                   "%s", _opOption.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Invalid pattern is detected for update: %s: %s",
                  _opOption.toString().c_str(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
