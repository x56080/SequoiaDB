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

   Source File Name = mthNodePool.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_NODEPOOL_HPP_
#define MTH_NODEPOOL_HPP_

#include <set>
using namespace std;

#define MTH_NODE_POOL_DEFAULT_SZ 4 

namespace engine
{
   template< typename TYPE, UINT32 nodeSize = MTH_NODE_POOL_DEFAULT_SZ >
   class _mthNodePool : public SDBObject
   {
   public:
      _mthNodePool()
      {
         UINT32 index = 0 ;
         for ( ; index < nodeSize ; index++ )
         {
            _staticIdleSet.insert( &_static[ index ] ) ;
         }
      }

      ~_mthNodePool()
      {
         clear() ;
      }

   public:
      INT32 allocate( TYPE *&tp )
      {
         INT32 rc = SDB_OK ;
         if ( !_staticIdleSet.empty() )
         {
            typename set<TYPE*>::iterator iter = _staticIdleSet.begin() ;
            tp = *iter ;
            _staticIdleSet.erase( iter ) ;
         }
         else
         {
            TYPE *node = SDB_OSS_NEW TYPE ;
            if ( NULL == node )
            {
               rc = SDB_OOM ;
               goto error ;
            }

            _dynamic.insert( node ) ;
            tp = node ;
         }
      done:
         return rc ;
      error:
         goto done ;
      }

      void release( TYPE *tp )
      {
         if ( tp >= &_static[0] && tp <= &_static[ nodeSize -1 ] )
         {
            SDB_ASSERT( _staticIdleSet.find( tp ) == _staticIdleSet.end() ,
                        "must be allocate before!" ) ;
            SDB_ASSERT( ( ( tp - (&_static[0]) ) % sizeof( TYPE ) ) == 0,
                          "address must be align!") ;

            _staticIdleSet.insert( tp ) ;
         }
         else
         {
            _dynamic.erase( tp ) ;
            SDB_OSS_DEL tp ;
         }
      }

      void clear()
      {
         typename set<TYPE*>::iterator iter = _dynamic.begin() ;
         for ( ; iter != _dynamic.end(); ++iter )
         {
            SDB_OSS_DEL *iter ;
         }

         _dynamic.clear() ;
         _staticIdleSet.clear() ;
         return ;
      }

   private:
      TYPE _static[ nodeSize ] ;
      set<TYPE*> _staticIdleSet ;
      set<TYPE*> _dynamic ;
   } ;
}

#endif

