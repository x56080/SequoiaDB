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

   Source File Name = rtnAlterFuncList.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnAlterFuncList.hpp"
#include "dpsLogWrapper.hpp"
#include "rtnAlterFuncs.hpp"

namespace engine
{
   _rtnAlterFuncList::_rtnAlterFuncListInter _rtnAlterFuncList::_fl ;
   _rtnAlterFuncList::_rtnAlterFuncList()
   {

   }

   _rtnAlterFuncList::~_rtnAlterFuncList()
   {

   }

   INT32 _rtnAlterFuncList::getFuncObj( RTN_ALTER_TYPE type,
                                        const CHAR *name,
                                        _rtnAlterFuncObj &obj )
   {
      return _fl.getFuncObj( type, name, obj ) ;
   }

   INT32 _rtnAlterFuncList::getFuncObj( RTN_ALTER_FUNC_TYPE type,
                                        _rtnAlterFuncObj &obj )
   {
      return _fl.getFuncObj( type, obj ) ;
   }

   INT32 _rtnAlterFuncList::_rtnAlterFuncListInter::
         getFuncObj( RTN_ALTER_TYPE type,
                     const CHAR *name,
                     _rtnAlterFuncObj &obj )
   {
      INT32 rc = SDB_OK ;
      string lower ;
      rc = init() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      lower.assign( name ) ;
      boost::algorithm::to_lower( lower ) ;
      for ( FOBJ_LIST::const_iterator itr = _fl.begin();
            itr != _fl.end();
            ++itr )
      {
         if ( type == itr->objType &&
              0 == lower.compare( itr->name ) )
         {
            obj = *itr ;
            goto done ;
         }
      }

      rc = SDB_INVALIDARG ;
      goto error ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnAlterFuncList::_rtnAlterFuncListInter::
         getFuncObj( RTN_ALTER_FUNC_TYPE type,
                     _rtnAlterFuncObj &obj )
   {
      INT32 rc = SDB_OK ;
      rc = init() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      for ( FOBJ_LIST::const_iterator itr = _fl.begin();
            itr != _fl.end();
            ++itr )
      {
         if ( type == itr->type )
         {
            obj = *itr ;
            goto done ;
         }
      }

      rc = SDB_INVALIDARG ;
      goto error ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnAlterFuncList::_rtnAlterFuncListInter::init()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      if ( _inited )
      {
         goto done ;
      }

      _latch.get() ;
      locked = TRUE ;

      if ( _inited )
      {
         goto done ;
      }

      _fl.push_front( _rtnAlterFuncObj( SDB_ALTER_CRT_ID_INDEX,
                                        RTN_ALTER_TYPE_CL,
                                        RTN_ALTER_CL_CRT_ID_IDX,
                                        &rtnCreateIDIndex,
                                        &rtnCreateIDIndexVerify ) ) ;

      _fl.push_front( _rtnAlterFuncObj( SDB_ALTER_DROP_ID_INDEX,
                                        RTN_ALTER_TYPE_CL,
                                        RTN_ALTER_CL_DROP_ID_IDX,
                                        &rtnDropIDIndex,
                                        &rtnDropIDIndexVerify ) ) ;

      _inited = TRUE ;
   done:
      if ( locked )
      {
         _latch.release() ;
      }
      return rc ;
   }

}

