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

   Source File Name = rtnUserCache.hpp

   Descriptive Name = 

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for update
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/25/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_USER_CACHE_HPP__
#define RTN_USER_CACHE_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "ossRWMutex.hpp"
#include "auth.hpp"
#include "pmdEDU.hpp"
#include <boost/smart_ptr.hpp>

namespace engine
{
   class _rtnUserCache : public SDBObject
   {
   public:
      typedef ossPoolString KEY_TYPE;
      typedef boost::shared_ptr< const authAccessControlList > VALUE_TYPE;
      typedef std::map< KEY_TYPE, VALUE_TYPE > DATA_TYPE;

   public:
      _rtnUserCache() {}
      ~_rtnUserCache() {}

      INT32 getACL( pmdEDUCB *cb, const KEY_TYPE &userName, VALUE_TYPE &acl );
      void remove( const KEY_TYPE &userName);
      void clear();

   private:
      VALUE_TYPE _get( const KEY_TYPE &userName );
      std::pair< DATA_TYPE::iterator, bool > _insert( const KEY_TYPE &userName,
                                                      const VALUE_TYPE &acl );
      INT32 _fetch( pmdEDUCB *cb, const KEY_TYPE &userName, VALUE_TYPE &acl );
      INT32 _fetchForCoord( pmdEDUCB *cb,
                            const KEY_TYPE &userName,
                            const CHAR *pMsgBuffer,
                            BSONObj &privsObj );

   private:
      ossSpinSLatch _latch;
      ossSpinXLatch _fetchLatch;
      DATA_TYPE _data;
   };
   typedef _rtnUserCache rtnUserCache;

} // namespace engine

#endif
