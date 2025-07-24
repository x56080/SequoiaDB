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

   Source File Name = coordCommandBackup.hpp

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
#ifndef COORD_COMMAND_BACKUP_HPP__
#define COORD_COMMAND_BACKUP_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "aggrBuilder.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordBackupBase define
   */
   class _coordBackupBase : public _coordCommandBase
   {
      public:
         _coordBackupBase() ;
         virtual ~_coordBackupBase() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      protected:
         virtual FILTER_BSON_ID  _getGroupMatherIndex () = 0 ;
         virtual NODE_SEL_STY    _nodeSelWhenNoFilter () = 0 ;
         virtual BOOLEAN         _useContext () = 0 ;
         virtual UINT32          _getMask() const = 0 ;

   } ;
   typedef _coordBackupBase coordBackupBase ;

   /*
      _coordRemoveBackup define
   */
   class _coordRemoveBackup : public _coordBackupBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordRemoveBackup() ;
         virtual ~_coordRemoveBackup() ;

      protected:
         virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
         virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
         virtual BOOLEAN         _useContext () ;
         virtual UINT32          _getMask() const ;
   } ;
   typedef _coordRemoveBackup coordRemoveBackup ;

   /*
      _coordBackupOffline define
   */
   class _coordBackupOffline : public _coordBackupBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordBackupOffline() ;
         virtual ~_coordBackupOffline() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      protected:
         virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
         virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
         virtual BOOLEAN         _useContext () ;
         virtual UINT32          _getMask() const ;

         virtual BOOLEAN         _interruptWhenFailed() const ;

      private:
         INT32 _parseGroupList( MsgHeader *pMsg, CoordGroupList &groupLst,
                                pmdEDUCB *cb ) ;
   } ;
   typedef _coordBackupOffline coordBackupOffline ;

}

#endif // COORD_COMMAND_BACKUP_HPP__
