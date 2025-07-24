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

   Source File Name = omContextTransfer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/09/2015  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_CONTEXTTRANSFER_HPP__
#define OM_CONTEXTTRANSFER_HPP__

#include "rtnContext.hpp"
#include "pmdRemoteSession.hpp"
#include "pmdEDU.hpp"
#include "omSdbConnector.hpp"
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   class _omContextTransfer : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _omContextTransfer )
      public:
         _omContextTransfer( INT64 contextID, UINT64 eduID ) ;
         virtual ~_omContextTransfer() ;

         INT32 open( omSdbConnector *conn, MsgHeader *reply ) ;
      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () ;

      protected:
         virtual INT32     _prepareData( _pmdEDUCB *cb ) ;
         INT32             _appendReply( MsgHeader *reply ) ;
         INT32             _getMoreFromRemote( _pmdEDUCB *cb ) ;

      protected:
         omSdbConnector       *_conn ;
         SINT64               _originalContextID ;
   } ;

   typedef _omContextTransfer omContextTransfer ;
}

#endif /* OM_CONTEXTTRANSFER_HPP__ */




