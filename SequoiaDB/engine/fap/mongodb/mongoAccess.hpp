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

   Source File Name = mongoAccess.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_ACCESS_HPP_
#define _SDB_MONGO_ACCESS_HPP_

#include "pmdAccessProtocolBase.hpp"
#include "sdbInterface.hpp"
#include "ossFeat.hpp"

#define ACCESS_FOR_MONGODB_CLIENT "server for mongodb client"
#define PORT_OFFSET 7

/*
   _mongoAccess define
*/
class _mongoAccess : public engine::IPmdAccessProtocol
{
public:
   _mongoAccess() {}
   virtual ~_mongoAccess() { _release() ; }

   virtual const CHAR *name() const
   {
      return ACCESS_FOR_MONGODB_CLIENT;
   }

   // use bases
   //virtual UINT32 maxConnNum() const ;

public:
   virtual INT32 init( engine::IResource *pResource ) ;
   virtual INT32 active() ;
   virtual INT32 deactive() ;
   virtual INT32 fini() ;

   virtual const CHAR *getServiceName() const ;
   virtual engine::pmdSession *getSession( SOCKET fd ) ;
   virtual void releaseSession( engine::pmdSession *pSession ) ;

private:
   void _release() ;

private:
   engine::IResource *_resource ;
   CHAR _serviceName[ OSS_MAX_SERVICENAME + 1 ] ;
};

typedef _mongoAccess mongoAccess ;

#endif // _SDB_MONGO_ACCESS_HPP_
