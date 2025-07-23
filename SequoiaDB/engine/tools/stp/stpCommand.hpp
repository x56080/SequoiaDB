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

   Source File Name = stpCommand.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef STP_COMMAND_HPP__
#define STP_COMMAND_HPP__

#include "stpCBCommon.hpp"
#include "msgDef.hpp"
#include "stpNode.hpp"
#include "pmdEDU.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   // register command with new function
   // declare of new function
   #define DECLARE_STP_CMD_AUTO_REGISTER()                     \
      public:                                                  \
         static stpCommand *newThis() ;                        \

   // implement of new function
   #define IMPLEMENT_STP_CMD_AUTO_REGISTER( theClass )         \
      stpCommand *theClass::newThis ()                         \
      {                                                        \
         return SDB_OSS_NEW theClass( stpGetSTPCB() ) ;        \
      }                                                        \
      stpCommandAssit theClass##Assit( theClass::newThis ) ;   \

   /*
      _stpCommand define
    */
   // _stpCommand is base class for commands of STP service
   class _stpCommand : public utilPooledObject
   {
   public:
      // constructor and destructor
      _stpCommand( STPCB *stpCB ) ;
      virtual ~_stpCommand() ;

   public:
      // check business to run command
      OSS_INLINE virtual BOOLEAN needCheckBusiness() const
      {
         return TRUE ;
      }

      // get name of command
      virtual const CHAR *getName() const = 0 ;

      // check if command needs to run on primary server
      OSS_INLINE virtual BOOLEAN needPrimary() const
      {
         return FALSE ;
      }

      // check if command can redirect to primary if this node is not
      OSS_INLINE virtual BOOLEAN canRedirectPrimary() const
      {
         return FALSE ;
      }

   public:
      // initialize command with given option
      virtual INT32 initialize( const CHAR *option ) ;
      // finalize command
      virtual INT32 finalize() ;
      // run command and output result in BSON format
      // input:
      // - session: service session of STP
      // - message: processing message
      // - result: result in BSON format
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) = 0 ;

   protected:
      // pointer to STPCB
      STPCB * _stpCB ;
   } ;

   typedef class _stpCommand stpCommand ;

   // function type for new command
   typedef stpCommand *(* STP_CMD_NEW_FUNC)() ;

   /*
      _stpCommandAssit define
    */
   // _stpCommandAssit is assistant to create command
   class _stpCommandAssit : public SDBObject
   {
   public:
      // constructor and destructor
      _stpCommandAssit( STP_CMD_NEW_FUNC func ) ;
      virtual ~_stpCommandAssit() ;
   } ;

   typedef class _stpCommandAssit stpCommandAssit ;

   /*
      _stpCommandBuilder define
    */
   // _stpCommandBuilder is builder to create and release command
   class _stpCommandBuilder : public SDBObject
   {
      friend class _stpCommandAssit ;

   public:
      // construtor and destructor
      _stpCommandBuilder() ;
      ~_stpCommandBuilder() ;

   public:
      // create command by name
      stpCommand *createCommand( const CHAR *name ) ;
      // release command
      void releaseCommand( stpCommand *command ) ;

   protected:
      // register command with name and create function
      void _registerCommand( const CHAR *name, STP_CMD_NEW_FUNC func ) ;
      // find command by name
      STP_CMD_NEW_FUNC _findCommand( const CHAR * name ) ;

   protected:
      // compare for command name
      struct _classComp
      {
         bool operator()( const CHAR *lhs, const CHAR *rhs ) const
         {
            return ossStrcmp( lhs, rhs ) < 0 ;
         }
      } ;

      // map to store command's create function
      typedef map< const CHAR *, STP_CMD_NEW_FUNC, _classComp > STP_CMD_MAP ;

      // map of command
      STP_CMD_MAP _commandMap ;
   } ;

   typedef class _stpCommandBuilder stpCommandBuilder ;


   /*
      STP command helper functions
    */
   // get default command builder
   stpCommandBuilder *stpGetCommandBuilder() ;
   // get command by name
   INT32 stpGetCommand( const CHAR *name, stpCommand **command ) ;
   // initialize command with given option
   INT32 stpInitCommand( stpCommand *command, const CHAR *option ) ;
   // run command and output result in BSON format
   INT32 stpRunCommand( stpCommand *command,
                        stpSession *session,
                        MsgHeader *message,
                        bson::BSONObj &result,
                        BOOLEAN &finished ) ;
   // release command
   INT32 stpReleaseCommand( stpCommand *command ) ;

   /*
      _stpGetTimeCMD define
    */
   // _stpGetTimeCMD gets logical time in nanosecond
   class _stpGetTimeCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpGetTimeCMD( STPCB *stpCB ) ;
      virtual ~_stpGetTimeCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_GET_TIME ;
      }

      // initialize with given option
      virtual INT32 initialize( const CHAR *option ) ;
      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;

   protected:
      BSONObj           _options ;
      STP_TIME_FORMAT   _format ;
   } ;

   typedef class _stpGetTimeCMD stpGetTimeCMD ;

   /*
      _stpGetMetaCMD define
    */
   // _stpGetMetaCMD gets meta data
   class _stpGetMetaCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpGetMetaCMD( STPCB *stpCB ) ;
      virtual ~_stpGetMetaCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_GET_META ;
      }

      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;
   } ;

   typedef class _stpGetMetaCMD stpGetMetaCMD ;

   /*
      _stpGetServersCMD define
    */
   // _stpGetServerCMD gets servers
   class _stpGetServersCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpGetServersCMD( STPCB *stpCB ) ;
      virtual ~_stpGetServersCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_GET_SERVERS ;
      }

      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;
   } ;

   typedef class _stpGetServersCMD stpGetServersCMD ;

   /*
      _stpGetSyncClientsCMD define
    */
   class _stpGetSyncClientsCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpGetSyncClientsCMD( STPCB *stpCB ) ;
      virtual ~_stpGetSyncClientsCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_GET_SYNC_CLIENTS ;
      }

      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;

      // check if command needs to run on primary server
      OSS_INLINE virtual BOOLEAN needPrimary() const
      {
         return TRUE ;
      }

      // check if command can redirect to primary if this node is not
      OSS_INLINE virtual BOOLEAN canRedirectPrimary() const
      {
         return TRUE ;
      }
   } ;

   typedef class _stpGetSyncClientsCMD stpGetSyncClientCMD ;

   /*
      _stpGetSyncStatusCMD define
    */
   class _stpGetSyncStatusCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpGetSyncStatusCMD( STPCB *stpCB ) ;
      virtual ~_stpGetSyncStatusCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_GET_SYNC_STATUS ;
      }

      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;
   } ;

   typedef class _stpGetSyncStatusCMD stpGetSyncStatusCMD ;

   /*
      _stpGetSyncHistoryCMD define
    */
   class _stpGetSyncHistoryCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpGetSyncHistoryCMD( STPCB *stpCB ) ;
      virtual ~_stpGetSyncHistoryCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_GET_SYNC_HISTORY ;
      }

      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;

   protected:
      // sort sources by last synchronized time
      INT32 _sortSources( const STP_SOURCE_MAP &sources,
                          STP_SOURCE_LIST &sourceList ) ;
   } ;

   typedef class _stpGetSyncHistoryCMD stpGetSyncHistoryCMD ;

   /*
      _stpGetConfigCMD define
    */
   class _stpGetConfigCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpGetConfigCMD( STPCB *stpCB ) ;
      virtual ~_stpGetConfigCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_GET_CONFIG ;
      }

      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;
   } ;

   typedef class _stpGetConfigCMD stpGetConfigCMD ;

   /*
      _stpUpdateConfigCMD define
    */
   class _stpUpdateConfigCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpUpdateConfigCMD( STPCB *stpCB ) ;
      virtual ~_stpUpdateConfigCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_UPDATE_CONFIG ;
      }

      // initialize with given option
      virtual INT32 initialize( const CHAR *option ) ;
      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;

   protected:
      // new configs to be updated
      bson::BSONObj _configs ;
   } ;

   typedef class _stpUpdateConfigCMD stpUpdateConfigCMD ;

   /*
      _stpStopCMD define
    */
   // stop STP node
   class _stpStopCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpStopCMD( STPCB *stpCB ) ;
      virtual ~_stpStopCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_STOP ;
      }

      // finalize command
      virtual INT32 finalize() ;
      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;
   } ;

   typedef class _stpStopCMD stpStopCMD ;

   /*
      _stpReelectCMD define
    */
   // reelect STP node
   class _stpReelectCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpReelectCMD( STPCB *stpCB ) ;
      virtual ~_stpReelectCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_REELECT ;
      }

      // initialize with given option
      virtual INT32 initialize( const CHAR *option ) ;
      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;

      // check if command needs to run on primary server
      OSS_INLINE virtual BOOLEAN needPrimary() const
      {
         return TRUE ;
      }

      // check if command can redirect to primary if this node is not
      OSS_INLINE virtual BOOLEAN canRedirectPrimary() const
      {
         return TRUE ;
      }

   protected:
      BSONObj        _options ;
      UINT32         _timeout ;
      const CHAR *   _targetHostName ;
   } ;

   typedef class _stpReelectCMD stpReelectCMD ;

   /*
      _stpConvTimeCMD define
    */
   // convert between logical time and real time
   class _stpConvTimeCMD : public stpCommand
   {
      DECLARE_STP_CMD_AUTO_REGISTER()

   public:
      // constructor and destructor
      _stpConvTimeCMD( STPCB *stpCB ) ;
      virtual ~_stpConvTimeCMD() ;

   public:
      // get name of command
      OSS_INLINE virtual const CHAR *getName() const
      {
         return CMD_NAME_STP_CONV_TIME ;
      }

      // initialize with given option
      virtual INT32 initialize( const CHAR *option ) ;
      // run command
      virtual INT32 doit( stpSession *session,
                          MsgHeader *message,
                          bson::BSONObj &result,
                          BOOLEAN &finished ) ;

   protected:
      // parse logical time from element
      INT32 _parseLTime( const bson::BSONElement &element ) ;
      // parse real time from element
      INT32 _parseRTime( const bson::BSONElement &element ) ;

      // convert logical time to BSON object
      INT32 _buildLTime( bson::BSONObjBuilder &builder ) ;
      // convert real time to BSON object
      INT32 _buildRTime( bson::BSONObjBuilder &builder ) ;

   protected:
      // options of convert time command
      BSONObj           _options ;
      // convert direction ( from real time to logical time or reverse )
      BOOLEAN           _fromRTimeToLTime ;
      // indicates whether simple mode
      // simple mode is converting between real time in $timestamp and logical
      // time in $numberLong in BSON types
      BOOLEAN           _simpleMode ;
      // logical time in convert time
      stpHPTime         _logicalTime ;
      // real time in convert time
      stpHPTime         _realTime ;
   } ;

   typedef class _stpConvTimeCMD stpConvTimeCMD ;

}

#endif // STP_COMMAND_HPP__
