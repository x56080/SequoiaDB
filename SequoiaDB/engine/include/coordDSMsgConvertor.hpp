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

   Source File Name = coordDSMsgConvertor.hpp

   Descriptive Name = Data source message convertor.

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
#ifndef COORD_DSMSGCONVERTOR_HPP__
#define COORD_DSMSGCONVERTOR_HPP__

#include "pmdRemoteSession.hpp"
#include "rtnCommandDef.hpp"

namespace engine
{

   #define SDB_OPR_READ          ( 0x00000001 )
   #define SDB_OPR_WRITE         ( 0x00000002 )

   #define SDB_IS_READ(x)        ( (x) & SDB_OPR_READ )
   #define SDB_IS_WRITE(x)       ( (x) & SDB_OPR_WRITE )

   #define SDB_SET_READ(x)       ( (x) = (x) | SDB_OPR_READ )
   #define SDB_SET_WRITE(x)      ( (x) = (x) | SDB_OPR_WRITE )

   class _rtnCommand ;
   /**
    * Data source message convertor. It's managed by the data source manager.
    * It converts messages received by local * coordinator into expected format
    * of the data source(actually, expected format of coordinator).
    */
   class _coordDSMsgConvertor : public _IRemoteMsgConvertor
   {
      public:
         _coordDSMsgConvertor() ;
         virtual ~_coordDSMsgConvertor() ;

      public:
         virtual INT32 filter( _pmdSubSession *pSub,
                               _pmdEDUCB *cb,
                               BOOLEAN &ignore,
                               UINT32 &oprMask ) ;

         virtual INT32 checkPermission( _pmdSubSession *pSub,
                                        UINT32 oprMask,
                                        _pmdEDUCB *cb ) ;

         virtual INT32 convert( _pmdSubSession *pSub,
                                _pmdEDUCB *cb ) ;

         virtual INT32 convertReply( _pmdSubSession *pSub,
                                     _pmdEDUCB *cb,
                                     const pmdEDUEvent &orgEvent,
                                     pmdEDUEvent &newEvent,
                                     BOOLEAN &hasConvert ) ;


      protected:
         INT32       _cmdShouldFilterOut( _pmdSubSession *pSub,
                                          const MsgOpQuery *pQuery,
                                          _pmdEDUCB *cb,
                                          BOOLEAN &ignore,
                                          UINT32 &oprMask ) const ;

         INT32       _lobShouldFilterOut( const MsgHeader *msg,
                                          const MsgHeader *pOrgMsg,
                                          BOOLEAN &ignore,
                                          UINT32 &oprMask ) const ;
      private:
         INT32 _parseQueryMsg( pmdSubSession *pSub, INT32 *flag,
                               const CHAR **cmdName,
                               const CHAR **query,
                               const CHAR **selector,
                               const CHAR **order,
                               const CHAR **hint ) ;

         INT32 _rebuildDataSourceInsertMsg( pmdSubSession *pSub,
                                            _pmdEDUCB *cb ) ;
         INT32 _rebuildDataSourceUpdateMsg( pmdSubSession *pSub,
                                            _pmdEDUCB *cb ) ;
         INT32 _rebuildDataSourceDeleteMsg( pmdSubSession *pSub,
                                            _pmdEDUCB *cb ) ;
         INT32 _rebuildDataSourceQueryMsg( pmdSubSession *pSub,
                                           _pmdEDUCB *cb ) ;
         INT32 _rebuildDataSourceCmdMsg( pmdSubSession *pSub,
                                         _pmdEDUCB *cb ) ;
         INT32 _rebuildDataSourceLobMsg( pmdSubSession *pSub,
                                         _pmdEDUCB *cb ) ;

         INT32 _buildEmptyLobMeta( BSONObj &metaObj ) const ;
         INT32 _buildLowVerLobMeta( const CHAR *pOrgMeta,
                                    BSONObj &newMeta ) const ;
         INT32 _buildNewEvent( const MsgOpReply *pOld,
                               const BSONObj &meta,
                               const pmdEDUEvent &orgEvent,
                               pmdEDUEvent &event ) ;

         /**
          * Some operations need to be done automatically internally, and they
          * should not be affected by the data source configurations.
          */
         BOOLEAN _isInternalOperation( RTN_COMMAND_TYPE type ) const ;
   } ;
   typedef _coordDSMsgConvertor coordDSMsgConvertor ;


   BOOLEAN coordIsSpecialMsg( INT32 opCode ) ;
   BOOLEAN coordIsLobMsg( INT32 opCode ) ;
   BOOLEAN coordIsLocalMappingCmd( _rtnCommand *pCommand ) ;
}

#endif /* COORD_DSMSGCONVERTOR_HPP__ */

