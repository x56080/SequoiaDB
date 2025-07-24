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

#include <list>

#define MTH_NODE_POOL_DEFAULT_SZ 4 

namespace engine
{
   template<typename T>
   class _mthNodePool : public SDBObject
   {
   public:
      _mthNodePool()
      :_eleSize( 0 )
      {

      }

      ~_mthNodePool()
      {
         clear() ;
      }

   public:
      INT32 allocate( T *&tp )
      {
         INT32 rc = SDB_OK ;
         if ( _eleSize < MTH_NODE_POOL_DEFAULT_SZ )
         {
            tp = &(_static[_eleSize++]) ;
         }
         else
         {
            T *node = SDB_OSS_NEW T ;
            if ( NULL == node )
            {
               rc = SDB_OOM ;
               goto error ;
            }

            _dynamic.push_back( node ) ;
            tp = node ;
            ++_eleSize ;
         }
      done:
         return rc ;
      error:
         goto done ;
      }

      void clear()
      {
         typename std::list<T*>::iterator itr = _dynamic.begin() ;
         for ( ; itr != _dynamic.end(); ++itr )
         {
            SDB_OSS_DEL *itr ;
         }
         _dynamic.clear() ;
         _eleSize = 0 ;
         return ;
      }
   private:
      T _static[MTH_NODE_POOL_DEFAULT_SZ] ;
      std::list<T*> _dynamic ;
      UINT32 _eleSize ;
   } ;
}

#endif

