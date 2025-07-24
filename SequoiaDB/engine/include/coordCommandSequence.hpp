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

   Source File Name = coordCommandSequence.hpp

   Descriptive Name = Coordinator Sequence Command

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_COMMAND_SEQUENCE_HPP_
#define COORD_COMMAND_SEQUENCE_HPP_

#include "coordCommandCommon.hpp"
#include "coordCommandData.hpp"
#include "coordFactory.hpp"
#include "rtnCommand.hpp"
#include <string>

namespace engine
{
   // This command is executed in _pmdCoordProcessor::_onQueryReqMsg for request from client
   class _coordCMDInvalidateSequenceCache : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDInvalidateSequenceCache() ;
         virtual ~_coordCMDInvalidateSequenceCache() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) { return SDB_OK ; }
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;
   } ;

   typedef class _coordCMDInvalidateSequenceCache coordCMDInvalidateSequenceCache ;

   // This command is executed in _CoordCB::_processQueryMsg for each coord node
   class _coordInvalidateSequenceCache : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()
   public:
      _coordInvalidateSequenceCache() ;
      virtual ~_coordInvalidateSequenceCache() ;

   public:
      virtual const CHAR *name() { return NAME_INVALIDATE_SEQUENCE_CACHE ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_INVALIDATE_SEQUENCE_CACHE ; }
      virtual INT32 spaceNode () ;
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      bson::BSONObj  _object ;
      const CHAR *   _collection ;
      const CHAR *   _fieldName ;
      const CHAR *   _sequenceName ;
      utilSequenceID _sequenceID ;
      BOOLEAN        _explicitCurrValue ;
      INT64          _currentValue ;
   } ;

   /*
      _coordCMDCreateSequence define
   */
   class _coordCMDCreateSequence : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDCreateSequence() ;
         virtual ~_coordCMDCreateSequence() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDCreateSequence coordCMDCreateSequence ;

   /*
      _coordCMDDropSequence define
   */
   class _coordCMDDropSequence : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDDropSequence() ;
         virtual ~_coordCMDDropSequence() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      private:
         INT32 _extractSeqID( rtnContextCoord *pContext,
                              pmdEDUCB *cb,
                              utilSequenceID &seqID ) ;
   } ;
   typedef _coordCMDDropSequence coordCMDDropSequence ;

   /*
      _coordCMDAlterSequence define
   */
   class _coordCMDAlterSequence : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public :
         _coordCMDAlterSequence() ;
         virtual ~_coordCMDAlterSequence() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      private:
         INT32 _parseArguments( const BSONObj &boQuery,
                                const CHAR **ppAction,
                                BSONObj &options,
                                const CHAR **ppSeqName ) ;

         INT32 _setCurrValueOnCoord( const CHAR *pSeqName,
                                     const INT64 expectValue,
                                     pmdEDUCB *cb,
                                     BOOLEAN &isSet ) ;

         INT32 _extractSeqID( rtnContextCoord *pContext,
                              pmdEDUCB *cb,
                              utilSequenceID &seqID ) ;
   } ;
   typedef _coordCMDAlterSequence coordCMDAlterSequence ;

   /*
      _coordCMDGetSeqCurrentValue define
   */
   class _coordCMDGetSeqCurrentValue : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDGetSeqCurrentValue() ;
         virtual ~_coordCMDGetSeqCurrentValue() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      private:
         INT32 _getCurrValueFromCatalog( const CHAR *pSeqName,
                                         MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &currentValue ) ;
   } ;
   typedef _coordCMDGetSeqCurrentValue coordCMDGetSeqCurrentValue ;
}

#endif /* COORD_COMMAND_SEQUENCE_HPP_ */

