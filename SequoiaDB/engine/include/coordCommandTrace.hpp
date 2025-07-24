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

   Source File Name = coordCommandTrace.hpp

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
#ifndef COORD_COMMAND_TRACE_HPP__
#define COORD_COMMAND_TRACE_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDTraceStart define
   */
   class _coordCMDTraceStart : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceStart() ;
         virtual ~_coordCMDTraceStart() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceStart coordCMDTraceStart ;

   /*
      _coordCMDTraceResume define
   */
   class _coordCMDTraceResume : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceResume() ;
         virtual ~_coordCMDTraceResume() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceResume coordCMDTraceResume ;

   /*
      _coordCMDTraceStop define
   */
   class _coordCMDTraceStop : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceStop() ;
         virtual ~_coordCMDTraceStop() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceStop coordCMDTraceStop ;

   /*
      _coordCMDTraceStatus define
   */
   class _coordCMDTraceStatus : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceStatus() ;
         virtual ~_coordCMDTraceStatus() ;
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceStatus coordCMDTraceStatus ;

}

#endif // COORD_COMMAND_TRACE_HPP__
