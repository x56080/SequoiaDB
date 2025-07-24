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

   Source File Name = coordLobOperator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/
#ifndef COORD_LOB_OPERATOR_HPP__
#define COORD_LOB_OPERATOR_HPP__

#include "coordOperator.hpp"

namespace engine
{
   /*
      _coordOpenLob define
   */
   class _coordOpenLob : public _coordOperator
   {
      public:
         _coordOpenLob() ;
         virtual ~_coordOpenLob() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

   } ;
   typedef _coordOpenLob coordOpenLob ;

   /*
      _coordWriteLob define
   */
   class _coordWriteLob : public _coordOperator
   {
      public:
         _coordWriteLob() ;
         virtual ~_coordWriteLob() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordWriteLob coordWriteLob ;

   /*
      _coordReadLob define
   */
   class _coordReadLob : public _coordOperator
   {
      public:
         _coordReadLob() ;
         virtual ~_coordReadLob() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordReadLob coordReadLob ;

   /*
      _coordLockLob define
   */
   class _coordLockLob : public _coordOperator
   {
      public:
         _coordLockLob() ;
         virtual ~_coordLockLob() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordLockLob coordLockLob ;

   /*
      _coordCloseLob define
   */
   class _coordCloseLob : public _coordOperator
   {
      public:
         _coordCloseLob() ;
         virtual ~_coordCloseLob() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCloseLob coordCloseLob ;

   /*
      _coordRemoveLob define
   */
   class _coordRemoveLob : public _coordOperator
   {
      public:
         _coordRemoveLob() ;
         virtual ~_coordRemoveLob() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordRemoveLob coordRemoveLob ;

   /*
      _coordTruncateLob define
   */
   class _coordTruncateLob : public _coordOperator
   {
      public:
         _coordTruncateLob() ;
         virtual ~_coordTruncateLob() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordTruncateLob coordTruncateLob ;

   /*
      _coordGetLobRTDetail define
   */
   class _coordGetLobRTDetail : public _coordOperator
   {
      public:
         _coordGetLobRTDetail() ;
         virtual ~_coordGetLobRTDetail() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordGetLobRTDetail coordGetLobRTDetail ;

   /*
      _coordCreateLobID define
   */
   class _coordCreateLobID : public _coordOperator
   {
      public:
         _coordCreateLobID() ;
         virtual ~_coordCreateLobID() ;
      public:
         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCreateLobID coordCreateLobID ;
}

#endif // COORD_LOB_OPERATOR_HPP__

