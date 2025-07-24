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

   Source File Name = rtnSubContext.cpp

   Descriptive Name = RunTime Sub-Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/28/2017   David Li  draft

   Last Changed =

*******************************************************************************/
#include "rtnSubContext.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   _rtnOrderKey::_rtnOrderKey ( const BSONObj &orderKey )
   : _ordering( Ordering::make( orderKey ) ),
     _keyBuilder( FALSE )
   {
      _hash.hash = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNORDERKEY_OPLT, "_rtnOrderKey::operator<" )
   BOOLEAN _rtnOrderKey::operator<( const _rtnOrderKey &rhs ) const
   {
      PD_TRACE_ENTRY ( SDB_RTNORDERKEY_OPLT ) ;
      BOOLEAN result = FALSE ;
      INT32 rsCmp = _keyObj.woCompare( rhs._keyObj, _ordering, FALSE ) ;
      if ( ( rsCmp < 0 ) ||
           ( 0 == rsCmp && _hash.hash < rhs._hash.hash ) )
      {
         result = TRUE ;
      }
      PD_TRACE1 ( SDB_RTNORDERKEY_OPLT, PD_PACK_INT(result) );
      PD_TRACE_EXIT ( SDB_RTNORDERKEY_OPLT ) ;
      return result;
   }

   void _rtnOrderKey::clear()
   {
      _hash.columns.hash1 = 0 ;
      _hash.columns.hash2 = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNORDERKEY_GENKEY, "_rtnOrderKey::generateKey" )
   INT32 _rtnOrderKey::generateKey( const BSONObj &record,
                                    _ixmIndexKeyGen *keyGen )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( keyGen != NULL, "keyGen can't be null!" ) ;
      PD_TRACE_ENTRY ( SDB_RTNORDERKEY_GENKEY ) ;
      clear();
      BSONObj keys ;
      BSONElement arrEle ;
      rc = keyGen->getKeys( record, keys, &arrEle, FALSE, FALSE, NULL,
                            &_keyBuilder ) ;
      PD_RC_CHECK( rc, PDERROR,
                  "failed to generate order-key(rc=%d)",
                  rc ) ;
      _keyObj = keys ;
      if ( arrEle.eoo() )
      {
         _hash.hash = 0 ;
      }
      else
      {
         ixmMakeHashValue( arrEle, _hash ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNORDERKEY_GENKEY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnSubContext::_rtnSubContext( const BSONObj& orderBy,
                                   _ixmIndexKeyGen* keyGen,
                                   INT64 contextID )
   : _orderKey( orderBy )
   {
      _isOrderKeyChange = TRUE ;
      _keyGen = keyGen ;
      _contextID = contextID ;
      _startFrom = 0 ;
   }

   _rtnSubContext::~_rtnSubContext()
   {
      _keyGen = NULL ;
      _contextID = -1 ;
   }
}
