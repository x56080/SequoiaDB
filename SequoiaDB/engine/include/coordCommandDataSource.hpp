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

   Source File Name = coordCommandDataSource.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_CMD_DATASOURCE_HPP__
#define COORD_CMD_DATASOURCE_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "coordCommandCommon.hpp"
#include "rtnCommand.hpp"

namespace engine
{
   class _coordCMDCreateDataSource : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDCreateDataSource() ;
      virtual ~_coordCMDCreateDataSource() ;

      virtual INT32 execute( MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                             rtnContextBuf *buf ) ;

   } ;

   typedef _coordCMDCreateDataSource coordCMDCreateDataSource;

   class _coordCMDDropDataSource : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDDropDataSource();

      virtual ~_coordCMDDropDataSource();

      virtual INT32 execute( MsgHeader *pMsg, pmdEDUCB *cb, INT64& contextID,
                             rtnContextBuf *buf );
   } ;

   typedef _coordCMDDropDataSource coordCMDDropDataSource;

   class _coordCMDAlterDataSource : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDAlterDataSource();

      virtual ~_coordCMDAlterDataSource();

      virtual INT32 execute( MsgHeader *pMsg, pmdEDUCB *cb, INT64& contextID,
                             rtnContextBuf *buf );

   } ;
   typedef _coordCMDAlterDataSource coordCMDAlterDataSource ;

   // This command is executed in _pmdCoordProcessor::_onQueryReqMsg.
   class _coordCMDDataSourceInvalidator : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDDataSourceInvalidator() ;
      virtual ~_coordCMDDataSourceInvalidator() ;

   private:
      BOOLEAN _useContext() ;
      INT32   _onLocalMode( INT32 flag ) ;
      void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
      UINT32  _getControlMask() const ;
   } ;
   typedef _coordCMDDataSourceInvalidator coordCMDDataSourceInvalidator ;

   // This command is executed in _CoordCB::_processQueryMsg for each coord
   // node.
   class _coordInvalidateDataSourceCache : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()
   public:
      _coordInvalidateDataSourceCache() ;

      virtual ~_coordInvalidateDataSourceCache() ;

      virtual const CHAR * name () ;
      virtual RTN_COMMAND_TYPE type () ;

      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;

      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;
   private:
      UINT32 _dsID ;
   } ;
   typedef _coordInvalidateDataSourceCache coordInvalidateDataSourceCache ;
}

#endif /* COORD_CMD_DATASOURCE_HPP__  */
