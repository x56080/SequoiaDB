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
   _rtnQueryModifier::_rtnQueryModifier( BOOLEAN isUpdate,
                                         BOOLEAN isRemove,
                                         BOOLEAN returnNew )
   : _isUpdate( isUpdate ),
   _isRemove ( isRemove ),
   _returnNew ( returnNew )
   {
      SDB_ASSERT( _isUpdate || _isRemove, "neither update nor remove" ) ;
      SDB_ASSERT( _isUpdate != _isRemove, "cannot be true in both" ) ;
   }

   INT32 _rtnQueryModifier::loadUpdator( const BSONObj &updator )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _isUpdate, "not update" ) ;

      _updator = updator ;

      if ( updator.isEmpty() )
      {
         PD_LOG ( PDERROR, "modifier can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         rc = _modifier.loadPattern ( updator,
                                      &_dollarList ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for updator: "
                      "%s", updator.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Invalid pattern is detected for update: %s: %s",
                  updator.toString().c_str(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
