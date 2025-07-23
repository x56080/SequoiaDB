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

   Source File Name = coordCommandProcedure.hpp

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
#ifndef COORD_COMMAND_PROCEDURE_HPP__
#define COORD_COMMAND_PROCEDURE_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{
   class _spdSession ;

   /*
      _coordCMDCrtProcedure define
   */
   class _coordCMDCrtProcedure : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDCrtProcedure() ;
         virtual ~_coordCMDCrtProcedure() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDCrtProcedure coordCMDCrtProcedure ;

   /*
      _coordCMDEval define
   */
   class _coordCMDEval : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDEval() ;
         virtual ~_coordCMDEval() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      private:
         INT32 _buildContext( _spdSession *session,
                              pmdEDUCB *cb,
                              SINT64 &contextID ) ;
   } ;
   typedef _coordCMDEval coordCMDEval ;

   /*
      _coordCMDRmProcedure define
   */
   class _coordCMDRmProcedure : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDRmProcedure() ;
         virtual ~_coordCMDRmProcedure() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDRmProcedure coordCMDRmProcedure ;

}

#endif // COORD_COMMAND_PROCEDURE_HPP__
