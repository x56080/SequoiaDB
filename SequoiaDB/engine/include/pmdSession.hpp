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

   Source File Name = pmdSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/04/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_SESSION_HPP_
#define PMD_SESSION_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossSocket.hpp"
#include "pmdSessionBase.hpp"
#include "pmdIProcessor.hpp"
#include "pmdExternClient.hpp"
#include "schedTaskMgr.hpp"

#include <string>

namespace engine
{

   class _pmdEDUCB ;
   class _dpsLogWrapper ;

   #define PMD_RETBUILDER_DFT_SIZE              ( 96 )
   /*
      _pmdSession define
   */
   class _pmdSession : public _pmdSessionBase
   {
      public:
         _pmdSession( SOCKET fd ) ;
         virtual ~_pmdSession() ;

         virtual UINT64          identifyID() ;
         virtual MsgRouteID      identifyNID() ;
         virtual UINT32          identifyTID() ;
         virtual UINT64          identifyEDUID() ;

         virtual void*           getSchedItemPtr() ;
         virtual void            setSchedItemVer( INT32 ver ) ;

         virtual void            clear() ;

         virtual const CHAR*     sessionName() const ;
         virtual BOOLEAN         isBusinessSession() const { return TRUE ; }
         virtual IClient*        getClient() { return &_client ; }

         virtual _dpsLogWrapper* getDPSCB() { return _pDPSCB ; }
         virtual _pmdEDUCB*      eduCB () const { return _pEDUCB ; }
         virtual EDUID           eduID () const { return _eduID ; }

         virtual INT32           run() = 0 ;

      public:
         UINT64      sessionID () const { return _eduID ; }
         ossSocket*  socket () { return &_socket ; }

         void        attach( _pmdEDUCB * cb ) ;
         void        detach() ;

         CHAR*       getBuff( UINT32 len ) ;
         INT32       getBuffLen () const { return _buffLen ; }

         INT32       allocBuff( UINT32 len, CHAR **ppBuff,
                                UINT32 *pRealSize = NULL ) ;
         void        releaseBuff( CHAR *pBuff ) ;
         INT32       reallocBuff( UINT32 len, CHAR **ppBuff,
                                  UINT32 *pRealSize = NULL ) ;

         void        disconnect() ;
         INT32       sendData( const CHAR *pData, INT32 size,
                               INT32 timeout = -1,
                               BOOLEAN block = TRUE,
                               INT32 *pSentLen = NULL,
                               INT32 flags = 0 ) ;
         INT32       recvData( CHAR *pData, INT32 size,
                               INT32 timeout = -1,
                               BOOLEAN block = TRUE,
                               INT32 *pRecvLen = NULL,
                               INT32 flags = 0 ) ;
         INT32       sniffData( INT32 timeout = OSS_ONE_SEC ) ;

      protected:
         inline BOOLEAN _isAwaitingHandshake () const
         {
            return _awaitingHandshake ;
         }

         inline void   _setHandshakeReceived ()
         {
            _awaitingHandshake = FALSE ;
         }

         inline void _startOp()
         {
            _lastBegin = ossGetCurrentMicroseconds() ;
            _lastEnd = 0 ;
         }

         inline void _endOp()
         {
            _lastEnd = ossGetCurrentMicroseconds() ;

            /// clear first msg time
            setFirstMsgTime( 0 ) ;
         }

         inline UINT64 _getLastBeginTime() const { return _lastBegin ; }
         inline UINT64 _getLastEndTime() const { return _lastEnd ; }
         inline UINT64 _getLastTimeSpan() const
         {
            UINT64 tmpEnd = ( 0 == _lastEnd ? ossGetCurrentMicroseconds() : _lastEnd ) ;
            if ( tmpEnd > _lastBegin )
            {
               return tmpEnd - _lastBegin ;
            }
            return 0 ;
         }

         void _saveOrSetMsgGlobalID( MsgHeader *pMsg ) ;

      protected:

         _pmdEDUCB                        *_pEDUCB ;
         EDUID                            _eduID ;
         ossSocket                        _socket ;
         std::string                      _sessionName ;
         pmdExternClient                  _client ;
         _dpsLogWrapper                   *_pDPSCB ;
         BOOLEAN                          _awaitingHandshake ;

         schedItem                        _infoItem ;
         UINT64                           _lastBegin ;
         UINT64                           _lastEnd ;

      protected:
         CHAR                             *_pBuff ;
         UINT32                           _buffLen ;

   } ;
   typedef _pmdSession pmdSession ;
}

#endif //PMD_SESSION_HPP_

