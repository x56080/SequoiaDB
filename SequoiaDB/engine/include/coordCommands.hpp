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

   Source File Name = coordCommands.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2017  XJH Init
   Last Changed =

*******************************************************************************/
#ifndef COORD_COMMANDS_HPP__
#define COORD_COMMANDS_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDSetSessionAttr define
    */
   class _coordCMDSetSessionAttr : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSetSessionAttr() ;
         virtual ~_coordCMDSetSessionAttr() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDSetSessionAttr coordCMDSetSessionAttr ;

   /*
      _coordCMDGetSessionAttr define
    */
   class _coordCMDGetSessionAttr : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public :
         _coordCMDGetSessionAttr () ;
         virtual ~_coordCMDGetSessionAttr () ;

         virtual INT32 execute ( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;
   } ;

   typedef _coordCMDGetSessionAttr coordCMDGetSessionAttr ;

}

#endif // COORD_COMMANDS_HPP__
