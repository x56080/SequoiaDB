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

   Source File Name = rtnCommand.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_COMMAND_HPP_
#define RTN_COMMAND_HPP_

#include "rtnCommandDef.hpp"
#include <string>
#include <vector>
#include "../bson/bson.h"
#include "dms.hpp"
#include "msg.hpp"
#include "migLoad.hpp"
#include "rtnAlterRunner.hpp"
#include "aggrBuilder.hpp"

using namespace bson ;

namespace engine
{

#define DECLARE_CMD_AUTO_REGISTER() \
   public: \
      static _rtnCommand *newThis () ; \

#define IMPLEMENT_CMD_AUTO_REGISTER(theClass) \
   _rtnCommand *theClass::newThis () \
   { \
      return SDB_OSS_NEW theClass() ;\
   } \
   _rtnCmdAssit theClass##Assit ( theClass::newThis ) ; \

   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _rtnCommand : public SDBObject
   {
      public:
         _rtnCommand () ;
         virtual ~_rtnCommand () ;

         void  setFromService( INT32 fromService ) ;
         INT32 getFromService() const { return _fromService ; }

         virtual INT32 spaceNode () ;
         virtual INT32 spaceService () ;

      public:
         virtual const CHAR * name () = 0 ;
         virtual RTN_COMMAND_TYPE type () = 0 ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) = 0 ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) = 0 ;

      protected:
         INT32             _fromService ;

   };

   typedef _rtnCommand* (*CDM_NEW_FUNC)() ;

   class _rtnCmdAssit : public SDBObject
   {
      public:
         _rtnCmdAssit ( CDM_NEW_FUNC pFunc ) ;
         ~_rtnCmdAssit () ;
   };

   struct _cmdBuilderInfo : public SDBObject
   {
   public :
      std::string    cmdName ;
      UINT32         nameSize ;
      CDM_NEW_FUNC   createFunc ;

      _cmdBuilderInfo *sub ;
      _cmdBuilderInfo *next ;
   } ;

   class _rtnCmdBuilder : public SDBObject
   {
      friend class _rtnCmdAssit ;

      public:
         _rtnCmdBuilder () ;
         ~_rtnCmdBuilder () ;
      public:
         _rtnCommand *create ( const CHAR *command ) ;
         void         release ( _rtnCommand *pCommand ) ;

      //protected:
         INT32 _register ( const CHAR * name, CDM_NEW_FUNC pFunc ) ;

         INT32        _insert ( _cmdBuilderInfo * pCmdInfo,
                                const CHAR * name, CDM_NEW_FUNC pFunc ) ;
         CDM_NEW_FUNC _find ( const CHAR * name ) ;

         void _releaseCmdInfo ( _cmdBuilderInfo *pCmdInfo ) ;

         UINT32 _near ( const CHAR *str1, const CHAR *str2 ) ;

      private:
         _cmdBuilderInfo                     *_pCmdInfoRoot ;

   };

   _rtnCmdBuilder * getRtnCmdBuilder () ;


   //Command list
   class _rtnCoordOnly : public _rtnCommand
   {
      protected:
         _rtnCoordOnly () {}
      public:
         virtual ~_rtnCoordOnly () {}
         virtual INT32 spaceNode () { return CMD_SPACE_NODE_COORD ; }
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff )
         { return SDB_RTN_COORD_ONLY ; }
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL )
         { return SDB_RTN_COORD_ONLY ; }
   };

   class _rtnCreateGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateGroup () {}
         virtual ~_rtnCreateGroup () {}
         virtual const CHAR * name () { return NAME_CREATE_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_GROUP ; }
   } ;

   class _rtnRemoveGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnRemoveGroup () {}
         virtual ~_rtnRemoveGroup () {}
         virtual const CHAR * name () { return NAME_REMOVE_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_GROUP ; }
   };

   class _rtnCreateNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateNode () {}
         virtual ~_rtnCreateNode () {}
         virtual const CHAR * name () { return NAME_CREATE_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_NODE ; }
   } ;

   class _rtnRemoveNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnRemoveNode () {}
         virtual ~_rtnRemoveNode () {}
         virtual const CHAR * name () { return NAME_REMOVE_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_NODE ; }
   };

   class _rtnUpdateNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnUpdateNode () {}
         virtual ~_rtnUpdateNode () {}
         virtual const CHAR * name () { return NAME_UPDATE_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_UPDATE_NODE ; }
   } ;

   class _rtnActiveGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnActiveGroup () {}
         virtual ~_rtnActiveGroup () {}
         virtual const CHAR * name () { return NAME_ACTIVE_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ACTIVE_GROUP ; }
   } ;

   class _rtnStartNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnStartNode () {}
         virtual ~_rtnStartNode () {}
         virtual const CHAR * name () { return NAME_START_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_START_NODE ; }
   };

   class _rtnShutdownNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnShutdownNode () {}
         virtual ~_rtnShutdownNode () {}
         virtual const CHAR * name () { return NAME_SHUTDOWN_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SHUTDOWN_NODE ; }
   };

   class _rtnShutdownGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnShutdownGroup () {}
         virtual ~_rtnShutdownGroup () {}
         virtual const CHAR * name () { return NAME_SHUTDOWN_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SHUTDOWN_GROUP ; }
   };

   class _rtnGetConfig : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnGetConfig () {}
         virtual ~_rtnGetConfig () {}
         virtual const CHAR * name () { return NAME_GET_CONFIG ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GET_CONFIG ; }
   } ;

   class _rtnListGroups : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListGroups () {}
         virtual ~_rtnListGroups () {}
         virtual const CHAR * name () { return NAME_LIST_GROUPS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_GROUPS ; }
   };

   class _rtnListProcedures : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListProcedures () {}
         virtual ~_rtnListProcedures () {}
         virtual const CHAR * name () { return NAME_LIST_PROCEDURES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_PROCEDURES ; }
   } ;

   class _rtnCreateProcedure : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateProcedure () {}
         virtual ~_rtnCreateProcedure () {}
         virtual const CHAR * name () { return NAME_CREATE_PROCEDURE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_PROCEDURE ; }
   } ;

   class _rtnRemoveProcedure : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnRemoveProcedure () {}
         virtual ~_rtnRemoveProcedure () {}
         virtual const CHAR * name () { return NAME_REMOVE_PROCEDURE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_PROCEDURE ; }
   } ;
   
   class _rtnListCSInDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListCSInDomain () {}
         virtual ~_rtnListCSInDomain () {}
         virtual const CHAR * name () { return NAME_LIST_CS_IN_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_CS_IN_DOMAIN ; }
   } ;

   class _rtnListCLInDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListCLInDomain () {}
         virtual ~_rtnListCLInDomain () {}
         virtual const CHAR * name () { return NAME_LIST_CL_IN_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_CL_IN_DOMAIN ; }
   } ;

   class _rtnCreateCataGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateCataGroup () {}
         virtual ~_rtnCreateCataGroup () {}
         virtual const CHAR * name () { return NAME_CREATE_CATAGROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_CATAGROUP ; }
   };

   class _rtnCreateDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateDomain () {}
         virtual ~_rtnCreateDomain () {}
         virtual const CHAR * name () { return NAME_CREATE_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_DOMAIN ; }
   } ;

   class _rtnDropDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnDropDomain () {}
         virtual ~_rtnDropDomain () {}
         virtual const CHAR * name () { return NAME_DROP_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_DROP_DOMAIN ; }
   } ;

   class _rtnAlterDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnAlterDomain () {}
         virtual ~_rtnAlterDomain () {}
         virtual const CHAR * name () { return NAME_ALTER_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ALTER_DOMAIN ; }
   } ;

   class _rtnAddDomainGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnAddDomainGroup () {}
         virtual ~_rtnAddDomainGroup () {}
         virtual const CHAR * name () { return NAME_ADD_DOMAIN_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ADD_DOMAIN_GROUP ; }
   };

   class _rtnRemoveDomainGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnRemoveDomainGroup () {}
         virtual ~_rtnRemoveDomainGroup () {}
         virtual const CHAR * name () { return NAME_REMOVE_DOMAIN_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_DOMAIN_GROUP ; }
   };

   class _rtnListDomains : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListDomains () {}
         virtual ~_rtnListDomains () {}
         virtual const CHAR * name () { return NAME_LIST_DOMAINS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_DOMAINS ; }
   };

   class _rtnSnapshotCata : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnSnapshotCata () {}
         virtual ~_rtnSnapshotCata () {}
         virtual const CHAR * name () { return NAME_SNAPSHOT_CATA ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SNAPSHOT_CATA ; }
   };

   class _rtnSnapshotCataIntr : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnSnapshotCataIntr () {}
         virtual ~_rtnSnapshotCataIntr () {}
         virtual const CHAR * name () { return CMD_NAME_SNAPSHOT_CATA_INTR ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SNAPSHOT_CATA ; }
   } ;

   class _rtnWaitTask : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnWaitTask () {}
         virtual ~_rtnWaitTask () {}
         virtual const CHAR * name () { return NAME_WAITTASK ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_WAITTASK ; }
   } ;

   class _rtnListTask : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListTask () {}
         virtual ~_rtnListTask () {}
         virtual const CHAR * name () { return NAME_LIST_TASKS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_TASKS ; }
   } ;

   class _rtnListUsers : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListUsers () {}
         virtual ~_rtnListUsers () {}
         virtual const CHAR * name () { return NAME_LIST_USERS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_USERS ; }
   } ;

   class _rtnGetDCInfo : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnGetDCInfo () {}
         virtual ~_rtnGetDCInfo () {}
         virtual const CHAR * name () { return NAME_GET_DCINFO ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GET_DCINFO ; }
   } ;

   class _rtnBackup : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnBackup () ;
         virtual ~_rtnBackup () ;

         virtual BOOLEAN      writable () { return TRUE ; }
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
      protected:
         const CHAR        *_matherBuff ;

         const CHAR        *_backupName ;
         const CHAR        *_path ;
         const CHAR        *_desp ;
         BOOLEAN           _ensureInc ;
         BOOLEAN           _rewrite ;

   };

   class _rtnCreateCollection : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCreateCollection () ;
         virtual ~_rtnCreateCollection () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      protected:
         const CHAR                 *_collectionName ;
         BSONObj                    _shardingKey ;
         UINT32                     _attributes ;
         UTIL_COMPRESSOR_TYPE       _compressorType ;
   };

   class _rtnCreateCollectionspace : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

     public:
         _rtnCreateCollectionspace () ;
         virtual ~_rtnCreateCollectionspace () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

     protected:
         const CHAR                 *_spaceName ;
         INT32                      _pageSize ;
         INT32                      _lobPageSize ;

   };

   class _rtnCreateIndex : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCreateIndex () ;
         virtual ~_rtnCreateIndex () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      protected:
         const CHAR              *_collectionName ;
         BSONObj                 _index ;
         INT32                   _sortBufferSize ;

   };

   class _rtnDropCollection : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnDropCollection () ;
         virtual ~_rtnDropCollection () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_collectionName ;

   };

   class _rtnDropCollectionspace : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnDropCollectionspace () ;
         virtual ~_rtnDropCollectionspace () ;

         const CHAR *spaceName () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_spaceName ;
   };

   class _rtnDropIndex : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnDropIndex () ;
         virtual ~_rtnDropIndex () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_collectionName ;
         BSONObj              _index ;
   };

   class _rtnGet : public _rtnCommand
   {
      protected:
         _rtnGet () ;
         virtual ~_rtnGet () ;
      public:
         virtual const CHAR * collectionFullName () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_collectionName ;
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         BSONObj              _hintObj ;
         BOOLEAN              _hintExist ;
         INT32                _flags ;

   } ;

   class _rtnGetCount : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetCount () ;
         virtual ~_rtnGetCount () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnGetIndexes : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetIndexes () ;
         virtual ~_rtnGetIndexes () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnGetDatablocks : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetDatablocks () ;
         virtual ~_rtnGetDatablocks () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnGetQueryMeta : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetQueryMeta () ;
         virtual ~_rtnGetQueryMeta () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

   } ;

   class _rtnList : public _rtnCommand
   {
      protected:
         _rtnList () ;
         virtual ~_rtnList () ;
      protected:
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         const CHAR           *_hintBuff ;
         INT32                _flags ;
   };

   class _rtnListCollections : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollections () {}
         virtual ~_rtnListCollections () {}

         virtual const CHAR * name () { return NAME_LIST_COLLECTIONS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_COLLECTIONS ; }
   };

   class _rtnListCollectionspaces : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollectionspaces () {}
         virtual ~_rtnListCollectionspaces () {}

         virtual const CHAR * name () { return NAME_LIST_COLLECTIONSPACES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_COLLECTIONSPACES ; }
   };

   class _rtnListContexts : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContexts () {}
         virtual ~_rtnListContexts () {}

         virtual const CHAR * name () { return NAME_LIST_CONTEXTS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_CONTEXTS ; }
   };

   class _rtnListContextsCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContextsCurrent () {}
         virtual ~_rtnListContextsCurrent () {}

         virtual const CHAR * name () { return NAME_LIST_CONTEXTS_CURRENT ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_CONTEXTS_CURRENT ; }
   };

   class _rtnListSessions : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessions () {}
         virtual ~_rtnListSessions () {}

         virtual const CHAR * name () { return NAME_LIST_SESSIONS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_SESSIONS ; }
   };

   class _rtnListSessionsCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessionsCurrent () {}
         virtual ~_rtnListSessionsCurrent () {}

         virtual const CHAR * name () { return NAME_LIST_SESSIONS_CURRENT ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_SESSIONS_CURRENT ; }
   };

   class _rtnListStorageUnits : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListStorageUnits () {}
         virtual ~_rtnListStorageUnits () {}

         virtual const CHAR * name () { return NAME_LIST_STORAGEUNITS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_STORAGEUNITS ; }
   } ;

   class _rtnListBackups : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public:
         _rtnListBackups () {} ;
         virtual ~_rtnListBackups () {} ;

         virtual const CHAR * name () { return NAME_LIST_BACKUPS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_BACKUPS ; }
   } ;

   class _rtnRenameCollection : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnRenameCollection () ;
         virtual ~_rtnRenameCollection () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_oldCollectionName ;
         const CHAR           *_newCollectionName ;
         const CHAR           *_csName ;
         std::string          _fullCollectionName ;
   };

   class _rtnReorg : public _rtnCommand
   {
      protected:
         _rtnReorg () ;
         virtual ~_rtnReorg () ;
         virtual INT32 spaceNode () ;
      public:
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_collectionName ;
         const CHAR           *_hintBuffer ;

   };

   class _rtnReorgOffline : public _rtnReorg
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnReorgOffline () ;
         virtual ~_rtnReorgOffline () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnReorgOnline : public _rtnReorg
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnReorgOnline () ;
         virtual ~_rtnReorgOnline () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnReorgRecover : public _rtnReorg
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnReorgRecover () ;
         virtual ~_rtnReorgRecover () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnShutdown : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnShutdown () ;
         virtual ~_rtnShutdown () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
   };

   class _rtnSnapshot : public _rtnCommand, public _aggrCmdBase
   {
      protected:
         _rtnSnapshot () ;
         virtual ~_rtnSnapshot () ;
      public:
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      protected:
         virtual BOOLEAN _useContext() { return TRUE ; }

      private:
         virtual const CHAR *getIntrCMDName() = 0 ;

      protected:
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         const CHAR           *_hintBuff ;

         INT32                _flags ;
   };

   class _rtnSnapshotInner : public _rtnSnapshot
   {
      protected:
         _rtnSnapshotInner () ;
         virtual ~_rtnSnapshotInner () ;

      public:
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      private:
         virtual const CHAR *getIntrCMDName() { return "" ; }
   } ;

   class _rtnSnapshotSystem : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotSystem () ;
         virtual ~_rtnSnapshotSystem () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotSystemInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSystemInner() {}
         virtual ~_rtnSnapshotSystemInner() {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   } ;

   class _rtnSnapshotContexts : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContexts () ;
         virtual ~_rtnSnapshotContexts () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotContextsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsInner() {}
         virtual ~_rtnSnapshotContextsInner() {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   } ;

   class _rtnSnapshotContextsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsCurrent () ;
         virtual ~_rtnSnapshotContextsCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotContextsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsCurrentInner() {}
         virtual ~_rtnSnapshotContextsCurrentInner() {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   } ;

   class _rtnSnapshotDatabase : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotDatabase () ;
         virtual ~_rtnSnapshotDatabase () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotDatabaseInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotDatabaseInner () {}
         virtual ~_rtnSnapshotDatabaseInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnSnapshotCollections : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollections () ;
         virtual ~_rtnSnapshotCollections () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotCollectionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionsInner () {}
         virtual ~_rtnSnapshotCollectionsInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnSnapshotCollectionSpaces : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionSpaces () ;
         virtual ~_rtnSnapshotCollectionSpaces () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotCollectionSpacesInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionSpacesInner () {}
         virtual ~_rtnSnapshotCollectionSpacesInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnSnapshotReset : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotReset () ;
         virtual ~_rtnSnapshotReset () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual BOOLEAN _useContext() { return FALSE ; }
   };

   class _rtnSnapshotSessions : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessions () ;
         virtual ~_rtnSnapshotSessions () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotSessionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsInner () {}
         virtual ~_rtnSnapshotSessionsInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnSnapshotSessionsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsCurrent () ;
         virtual ~_rtnSnapshotSessionsCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotSessionsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsCurrentInner () {}
         virtual ~_rtnSnapshotSessionsCurrentInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnSnapshotTransactionsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactionsCurrent () ;
         ~_rtnSnapshotTransactionsCurrent () ;

         virtual BOOLEAN isDumpCurrent() { return TRUE ;}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
         virtual BOOLEAN writable (){ return TRUE ;}

         virtual const CHAR *getIntrCMDName(){ return NULL ;}
   };

   class _rtnSnapshotTransactions : public _rtnSnapshotTransactionsCurrent
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactions () ;
         ~_rtnSnapshotTransactions () ;

         virtual BOOLEAN isDumpCurrent() { return FALSE ;}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnTest : public _rtnCommand
   {
      protected:
         _rtnTest () ;
         virtual ~_rtnTest () ;
      public:
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR        *_collectionName ;
   };

   class _rtnTestCollection : public _rtnTest
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTestCollection () ;
         virtual ~_rtnTestCollection () ;

         virtual const CHAR * collectionFullName () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnTestCollectionspace : public _rtnTest
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTestCollectionspace () ;
         virtual ~_rtnTestCollectionspace () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnSetPDLevel : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSetPDLevel () ;
         virtual ~_rtnSetPDLevel () ;

      public:
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
      protected:
         INT32             _pdLevel ;
   } ;

   class _rtnTraceStart : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceStart () ;
         virtual ~_rtnTraceStart () ;

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
      protected :
         UINT32 _mask ;
         std::vector<UINT32> _tid ;
         std::vector<UINT64> _funcCode ;
         UINT32 _size ;
   };

   class _rtnTraceResume : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceResume () ;
         virtual ~_rtnTraceResume () ;

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
   };

   class _rtnTraceStop : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceStop () ;
         virtual ~_rtnTraceStop () ;

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
      protected :
         const CHAR *_pDumpFileName ;
   };

   class _rtnTraceStatus : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceStatus () ;
         virtual ~_rtnTraceStatus () ;

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
      protected:
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         INT32                _flags ;
   };

   class _rtnLoad : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnLoad () ;
         virtual ~_rtnLoad () ;

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
      protected :
         setParameters _parameters ;
         CHAR     _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR     _csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]   ;
         CHAR     _clName[ DMS_COLLECTION_NAME_SZ + 1 ]   ;
   };

   class _rtnExportConf : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnExportConf(){}
         virtual ~_rtnExportConf(){}

         virtual const CHAR * name () { return NAME_EXPORT_CONFIGURATION ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_EXPORT_CONFIG ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff )
         {
            return SDB_OK ;
         }

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

   } ;

   class _rtnRemoveBackup : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER ()

      public:
         _rtnRemoveBackup () ;
         virtual ~_rtnRemoveBackup () {}

      public:
         virtual const CHAR * name () { return NAME_REMOVE_BACKUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_BACKUP ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      protected:
         const CHAR              *_path ;
         const CHAR              *_backupName ;
         const CHAR              *_matcherBuff ;

   } ;

   class _rtnForceSession : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnForceSession() ;
      virtual ~_rtnForceSession() ;

   public:
      virtual const CHAR * name () { return NAME_FORCE_SESSION ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_FORCE_SESSION ; }
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL  ) ;

   private:
      EDUID _sessionID ;
   } ;
   typedef class _rtnForceSession rtnForceSession ;

   class _rtnListLob : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnListLob() ;
      virtual ~_rtnListLob() ;

   public:
      virtual const CHAR * name () { return NAME_LIST_LOBS ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_LIST_LOB ; }
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL  ) ;
      virtual const CHAR *collectionFullName() ;

   private:
      INT64 _contextID ;
      bson::BSONObj _query ;
      const CHAR *_fullName ;
   } ;

   class _rtnSetSessionAttr : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSetSessionAttr() {}
         virtual ~_rtnSetSessionAttr() {}

      public:
         virtual const CHAR * name () { return NAME_SET_SESSIONATTR ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SET_SESSIONATTR ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
   } ;

   class _rtnTruncate : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnTruncate()
      :_fullName( NULL )
      {

      }

      virtual ~_rtnTruncate() {}

   public:
      virtual const CHAR * name () { return NAME_TRUNCATE ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_TRUNCATE ; }
      virtual BOOLEAN writable()
      {
         return TRUE ;
      }

      virtual const CHAR * collectionFullName()
      {
         return _fullName ;
      }

      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;
   private:
      const CHAR * _fullName ;
   } ;

   class _rtnAlterCollection: public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnAlterCollection() ;
      virtual ~_rtnAlterCollection() ;

   public:
      virtual const CHAR * name () { return NAME_ALTER_COLLECTION ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_ALTER_COLLECTION ; }
      virtual const CHAR * collectionFullName() ;
      virtual BOOLEAN writable() { return TRUE ;}

      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

      OSS_INLINE const _rtnAlterRunner &getRunner() const
      {
         return _runner ;
      }

   private:
      INT32 _handleOldVersion( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                               _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                               INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      /// old version
      BSONObj _alterObj ;

      /// new version
      _rtnAlterRunner _runner ;
   } ;

   class _rtnSyncDB : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnSyncDB() ;
      virtual ~_rtnSyncDB() ;

   public:
      virtual const CHAR * name () { return NAME_SYNC_DB ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_SYNC_DB ; }
      virtual BOOLEAN writable() { return FALSE ;}
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;
   } ;
}

const UINT32 pdGetTraceFunctionListNum();

#endif //RTN_COMMAND_HPP_

