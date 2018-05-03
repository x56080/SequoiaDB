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

   Source File Name = rtnFetchBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/


#include "rtnFetchBase.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnFetchBuilder implement
   */
   _rtnFetchBuilder::_rtnFetchBuilder ()
   {
   }

   _rtnFetchBuilder::~_rtnFetchBuilder ()
   {
      _mapFuncs.clear() ;
   }

   INT32 _rtnFetchBuilder::_register( INT32 type, FETCH_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;

      if ( !_mapFuncs.insert( std::make_pair( type, pFunc ) ).second )
      {
         PD_LOG ( PDERROR, "Register fetch [%d] failed", type ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _rtnFetchBase *_rtnFetchBuilder::create( INT32 type )
   {
      MAP_FUNCS::iterator it = _mapFuncs.find( type ) ;
      if ( it != _mapFuncs.end() && NULL != it->second )
      {
         return (*( it->second ) )() ;
      }

      return NULL ;
   }

   void _rtnFetchBuilder::release( _rtnFetchBase *pFetch )
   {
      if ( pFetch )
      {
         SDB_OSS_DEL pFetch ;
      }
   }

   _rtnFetchBuilder *getRtnFetchBuilder ()
   {
      static _rtnFetchBuilder s_fetchBuilder ;
      return &s_fetchBuilder ;
   }

   /*
      _rtnFetchAssit implement
   */
   _rtnFetchAssit::_rtnFetchAssit ( FETCH_NEW_FUNC pFunc )
   {
      if ( pFunc )
      {
         _rtnFetchBase *pFetch = (*pFunc)() ;
         if ( pFetch )
         {
            getRtnFetchBuilder()->_register ( pFetch->getType(), pFunc ) ;
            SDB_OSS_DEL pFetch ;
            pFetch = NULL ;
         }
      }
   }

   _rtnFetchAssit::~_rtnFetchAssit ()
   {
   }


}


