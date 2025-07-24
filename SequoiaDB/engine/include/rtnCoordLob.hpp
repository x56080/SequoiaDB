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

   Source File Name = rtnCoordLob.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2014  YW  Initial Draft
   Last Changed =

*******************************************************************************/
#ifndef RTN_COORDLOB_HPP_
#define RTN_COORDLOB_HPP_

#include "rtnCoordOperator.hpp"

namespace engine
{
   class rtnCoordOpenLob : public rtnCoordOperator
   {
   public:
      rtnCoordOpenLob(){}
      virtual ~rtnCoordOpenLob(){}
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordWriteLob : public rtnCoordOperator
   {
   public:
      rtnCoordWriteLob(){}
      virtual ~rtnCoordWriteLob(){}
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordReadLob : public rtnCoordOperator
   {
   public:
      rtnCoordReadLob(){}
      virtual ~rtnCoordReadLob(){}
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordCloseLob : public rtnCoordOperator
   {
   public:
      rtnCoordCloseLob(){}
      virtual ~rtnCoordCloseLob(){}
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   class rtnCoordRemoveLob : public rtnCoordOperator
   {
   public:
      rtnCoordRemoveLob(){}
      virtual ~rtnCoordRemoveLob(){}
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;
}

#endif

