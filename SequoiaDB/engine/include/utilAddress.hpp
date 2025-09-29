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

   Source File Name = utilAddress.hpp

   Descriptive Name = Address utility

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ADDRESS_HPP__
#define UTIL_ADDRESS_HPP__

#include "utilArray.hpp"
#include "ossSocket.hpp"

namespace engine
{
   /**
    * Address information, including host and service name.
    */
   class _utilAddrPair : public SDBObject
   {
   public:
      _utilAddrPair() ;
      ~_utilAddrPair() ;

      INT32 setHost( const CHAR *host ) ;
      const CHAR *getHost() const ;
      INT32 setService( const CHAR *service ) ;
      const CHAR *getService() const ;

      BOOLEAN operator==( const _utilAddrPair& r ) const ;

   private:
      CHAR _host[ OSS_MAX_HOSTNAME + 1 ] ;
      CHAR _service[ OSS_MAX_SERVICENAME + 1 ] ;
   } ;
   typedef _utilAddrPair utilAddrPair ;

   /**
    * Service address item container. Each item contains a host name or an IP
    * address and a service name.
    */
   class _utilAddrContainer : public SDBObject
   {
   public:
      _utilAddrContainer() {}
      ~_utilAddrContainer() {}

      INT32 append( const utilAddrPair &addr ) ;

      OSS_INLINE UINT32 size() const
      {
         return _addresses.size() ;
      }

      OSS_INLINE void clear()
      {
         _addresses.clear() ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         return _addresses.empty() ;
      }

      _utilArray<utilAddrPair>& getAddresses()
      {
         return _addresses ;
      }

      BOOLEAN contains( const utilAddrPair &addr ) ;

   private:
      _utilArray<utilAddrPair> _addresses ;
   } ;
   typedef _utilAddrContainer utilAddrContainer ;

   /**
    * @brief Parse an address list into address pair array.
    *
    * @param addrStr A string which contains one or more addresses.
    * @param addrArray Address pair array.
    * @param itemSeparator Separator of address items in the string.
    * @param innerSeparator Separator of host and service in an address.
    *
    * Each address item in the address list should be in the format of
    * <host>:<svcname>.
    */
   INT32 utilParseAddrList( const CHAR *addrStr,
                            utilAddrContainer &addrArray,
                            const CHAR *itemSeparator = ",",
                            const CHAR *innerSeparator = ":" ) ;
}

#endif /* UTIL_ADDRESS_HPP__ */
