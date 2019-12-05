/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = tpCommand.hpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef TP_COMMAND_HPP__
#define TP_COMMAND_HPP__

#include "tpCBCommon.hpp"
#include "msgDef.hpp"
#include "tpNode.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   #define DECLARE_TP_CMD_AUTO_REGISTER()                      \
      public:                                                  \
         static tpCommand *newThis() ;                         \

   #define IMPLEMENT_TP_CMD_AUTO_REGISTER( theClass )          \
      tpCommand *theClass::newThis ()                          \
      {                                                        \
         return SDB_OSS_NEW theClass( sdbGetTPCB() ) ;         \
      }                                                        \
      tpCommandAssit theClass##Assit( theClass::newThis ) ;    \

   /*
      _tpCommand define
    */
   class _tpCommand : public utilPooledObject
   {
   public:
      _tpCommand( SDB_TPCB *tpCB ) ;
      virtual ~_tpCommand() ;

   public:
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return TRUE ;
      }

      OSS_INLINE virtual BOOLEAN hasResult() const
      {
         return TRUE ;
      }

      virtual const CHAR *getName() const = 0 ;

   public:
      virtual INT32 initialize( const CHAR *option ) ;
      virtual INT32 finalize() ;
      virtual INT32 doit( bson::BSONObj &result ) = 0 ;

   protected:
      SDB_TPCB * _tpCB ;
   } ;

   typedef class _tpCommand tpCommand ;
   typedef tpCommand *(* TP_CMD_NEW_FUNC)() ;

   /*
      _tpCommandAssit define
    */
   class _tpCommandAssit : public SDBObject
   {
   public:
      _tpCommandAssit( TP_CMD_NEW_FUNC func ) ;
      virtual ~_tpCommandAssit() ;
   } ;

   typedef class _tpCommandAssit tpCommandAssit ;

   /*
      _tpCommandBuilder define
    */
   class _tpCommandBuilder : public SDBObject
   {
      friend class _tpCommandAssit ;

   public:
      _tpCommandBuilder() ;
      ~_tpCommandBuilder() ;

   public:
      tpCommand *createCommand( const CHAR *name ) ;
      void releaseCommand( tpCommand *command ) ;

   protected:
      void _registerCommand( const CHAR *name, TP_CMD_NEW_FUNC func ) ;
      TP_CMD_NEW_FUNC _findCommand( const CHAR * name ) ;

   protected:
      struct _classComp
      {
         bool operator()( const CHAR *lhs, const CHAR *rhs ) const
         {
            return ossStrcmp( lhs, rhs ) < 0 ;
         }
      } ;

      typedef map< const CHAR *, TP_CMD_NEW_FUNC, _classComp > TP_CMD_MAP ;

      TP_CMD_MAP _commandMap ;
   } ;

   typedef class _tpCommandBuilder tpCommandBuilder ;


   /*
      tp command helper functions
    */
   tpCommandBuilder *tpGetCommandBuilder() ;
   INT32 tpGetCommand( const CHAR *name, tpCommand **command ) ;
   INT32 tpInitCommand( tpCommand *command, const CHAR *information ) ;
   INT32 tpRunCommand( tpCommand *command, bson::BSONObj &result ) ;
   INT32 tpReleaseCommand( tpCommand *command ) ;

   /*
      _tpGetTimeCMD define
    */
   class _tpGetTimeCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpGetTimeCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpGetTimeCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_GET_TIME ;
      }

      virtual INT32 doit( bson::BSONObj &result ) ;
   } ;

   typedef class _tpGetTimeCMD tpGetTimeCMD ;

   /*
      _tpGetMetaCMD define
    */
   class _tpGetMetaCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpGetMetaCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpGetMetaCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_GET_META ;
      }

      virtual INT32 doit( bson::BSONObj &result ) ;
   } ;

   typedef class _tpGetMetaCMD tpGetMetaCMD ;

   /*
      _tpGetServersCMD define
    */
   class _tpGetServersCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpGetServersCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpGetServersCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_GET_SERVERS ;
      }

      virtual INT32 doit( bson::BSONObj &result ) ;
   } ;

   typedef class _tpGetServersCMD tpGetServersCMD ;

   /*
      _tpGetSyncClientsCMD define
    */
   class _tpGetSyncClientsCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpGetSyncClientsCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpGetSyncClientsCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_GET_SYNC_CLIENTS ;
      }

      virtual INT32 doit( bson::BSONObj &result ) ;
   } ;

   typedef class _tpGetSyncClientsCMD tpGetSyncClientCMD ;

   /*
      _tpGetSyncStatusCMD define
    */
   class _tpGetSyncStatusCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpGetSyncStatusCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpGetSyncStatusCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_GET_SYNC_STATUS ;
      }

      virtual INT32 doit( bson::BSONObj &result ) ;
   } ;

   typedef class _tpGetSyncStatusCMD tpGetSyncStatusCMD ;

   /*
      _tpGetSyncHistoryCMD define
    */
   class _tpGetSyncHistoryCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpGetSyncHistoryCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpGetSyncHistoryCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_GET_SYNC_HISTORY ;
      }

      virtual INT32 doit( bson::BSONObj &result ) ;

   protected:
      INT32 _sortSources( const TP_SOURCE_MAP &sources,
                          TP_SOURCE_LIST &sourceList ) ;
   } ;

   typedef class _tpGetSyncHistoryCMD tpGetSyncHistoryCMD ;

   /*
      _tpGetConfigCMD define
    */
   class _tpGetConfigCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpGetConfigCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpGetConfigCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_GET_CONFIG ;
      }

      virtual INT32 doit( bson::BSONObj &result ) ;
   } ;

   typedef class _tpGetConfigCMD tpGetConfigCMD ;

   /*
      _tpUpdateConfigCMD define
    */
   class _tpUpdateConfigCMD : public tpCommand
   {
      DECLARE_TP_CMD_AUTO_REGISTER()

   public:
      _tpUpdateConfigCMD( SDB_TPCB *tpCB ) ;
      virtual ~_tpUpdateConfigCMD() ;

   public:
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_TP_UPDATE_CONFIG ;
      }

      virtual INT32 initialize( const CHAR *option ) ;
      virtual INT32 doit( bson::BSONObj &result ) ;

   protected:
      bson::BSONObj _configs ;
   } ;

   typedef class _tpUpdateConfigCMD tpUpdateConfigCMD ;

}

#endif // TP_COMMAND_HPP__
