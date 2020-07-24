/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = catCommand.hpp

   Descriptive Name = Catalogue commands.

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
#ifndef CAT_COMMAND_HPP__
#define CAT_COMMAND_HPP__
#include "catCMDBase.hpp"
#include "catLevelLock.hpp"
#include "IDataSource.hpp"

namespace engine
{
   struct _catDSInfo
   {
      UINT32      _id ;
      INT32       _version ;
      const CHAR *_dsName;
      const CHAR *_type ;     // Only support "SequoiaDB" for now.
      const CHAR *_dsVersion ;
      const CHAR *_addresses ;
      const CHAR *_user ;
      const CHAR *_password ;
      const CHAR *_errCtlLevel ;
      INT32       _accessMode ;
      INT32       _errFilterMask ;

      _catDSInfo()
      {
         reset() ;
      }

      void reset()
      {
         _id = 0 ;
         _version = 0 ;
         _dsName = NULL ;
         _type = NULL ;
         _dsVersion = NULL ;
         _addresses = NULL ;
         _user = NULL ;
         _password = NULL ;
         _errCtlLevel = VALUE_NAME_HIGH ;
         _accessMode = DS_ACCESS_DEFAULT ;
         _errFilterMask = DS_ERR_FILTER_NONE ;
      }

      BSONObj toBson()
      {
         try
         {
            BSONObjBuilder builder ;
            builder.append( FIELD_NAME_ID, _id );
            builder.append( FIELD_NAME_NAME, _dsName ) ;
            builder.append( FIELD_NAME_TYPE, _type ) ;
            builder.append( FIELD_NAME_VERSION, _version ) ;
            builder.append( FIELD_NAME_DSVERSION, _dsVersion ) ;
            builder.append( FIELD_NAME_ADDRESS, _addresses ) ;
            builder.append( FIELD_NAME_USER, _user ? _user : "" ) ;
            builder.append( FIELD_NAME_PASSWD, _password ? _password : "" ) ;
            builder.append( FIELD_NAME_ERRORCTLLEVEL, _errCtlLevel ) ;
            builder.append( FIELD_NAME_ACCESSMODE, _accessMode ) ;
            const CHAR *desc = NULL ;
            DS_ACCESS_MODE_2_DESC( _accessMode, desc ) ;
            builder.append( FIELD_NAME_ACCESSMODE_DESC, desc ) ;
            builder.append( FIELD_NAME_ERRORFILTERMASK, _errFilterMask ) ;
            DS_ERR_FILTER_2_DESC( _errFilterMask, desc ) ;
            builder.append( FIELD_NAME_ERRORFILTERMASK_DESC, desc ) ;
            return builder.obj() ;
         }
         catch ( std::exception &e )
         {
            return BSONObj() ;
         }
      }
   } ;
   typedef _catDSInfo catDSInfo ;

   /**
    * @brief Create data source command. Metadata of the data source will be
    *        added into the system collection SYSDATASOURCES.
    */
   class _catCMDCreateDataSource : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDCreateDataSource() ;
      virtual ~_catCMDCreateDataSource() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR* name() ;

   private:
      catDSInfo _dsInfo ;
   } ;
   typedef _catCMDCreateDataSource catCMDCreateDataSource ;

   class _catCMDDropDataSource : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDDropDataSource() ;
      virtual ~_catCMDDropDataSource() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR* name() ;

   private:
      /**
       * Check if the data source is being used by any collection space or
       * collection.
       * @param used
       * @return
       */
      INT32 _checkUsage( BOOLEAN &used, pmdEDUCB *cb ) ;

   private:
      const CHAR *_name ;
      UTIL_DS_UID _dsID ;
      catCtxLockMgr _lockMgr ;
   } ;
   typedef _catCMDDropDataSource catCMDDropDataSource ;

   class _catCMDAlterDataSource : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDAlterDataSource() ;
      virtual ~_catCMDAlterDataSource() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR* name() ;

   private:
      INT32 _getDataSourceMeta( const CHAR *name, BSONObj &record ) ;

   private:
      UTIL_DS_UID _dsID ;
      BSONObjBuilder _optionBuilder ;
   } ;
   typedef _catCMDAlterDataSource catCMDAlterDataSource ;

   class _catCMDTestCollection : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDTestCollection() ;
      virtual ~_catCMDTestCollection() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR *name() ;

   private:
      const CHAR *_name ;

   } ;
   typedef _catCMDTestCollection catCMDTestCollection ;
}

#endif /* CAT_COMMAND_HPP__ */
