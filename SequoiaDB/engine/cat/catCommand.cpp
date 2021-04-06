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

   Source File Name = catCommand.cpp

   Descriptive Name = Catalogue commands.

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains catalog command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "catCommand.hpp"
#include "catLevelLock.hpp"
#include "catCommon.hpp"
#include "rtn.hpp"
#include "catTrace.hpp"
#include "ixmUtil.hpp"
#include "catTrace.hpp"

using namespace bson ;

namespace engine
{
   static INT32 catCalcAccessMode( const CHAR *modeStr, INT32 &mode )
   {
      INT32 rc = SDB_OK ;

      mode = DS_ACCESS_DEFAULT ;

      // The default value for accessing mode is 3(READ|WRITE). If it's
      // given explicitly, we need to calculate the result. The rule is
      // that the highest permission will be granted.
      if ( 0 == ossStrlen( modeStr ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Access mode is empty[%d]", rc ) ;
         goto error ;
      }

      // vector is being used, need to handle possible exceptions.
      try
      {
         BOOLEAN readAllowed = FALSE ;
         BOOLEAN writeAllowed = FALSE ;
         vector<string> values ;

         values = utilStrSplit( modeStr, "|" );
         for ( vector<string>::const_iterator itr = values.begin();
               itr != values.end(); ++itr )
         {
            if ( 0 == ossStrcasecmp( itr->c_str(), VALUE_NAME_READ ) )
            {
               readAllowed = TRUE ;
            }
            else if ( 0 == ossStrcasecmp( itr->c_str(),
                                          VALUE_NAME_WRITE ) )
            {
               writeAllowed = TRUE ;
            }
            else if ( 0 == ossStrcasecmp( itr->c_str(), VALUE_NAME_ALL ) )
            {
               readAllowed = TRUE ;
               writeAllowed = TRUE ;
            }
            else if ( 0 != ossStrcasecmp( itr->c_str(),
                                          VALUE_NAME_NONE ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Invalid access mode[%s] for data "
                                    "source[%d]", itr->c_str(), rc ) ;
               goto error ;
            }
         }
         if ( !readAllowed && writeAllowed )
         {
            mode = DS_ACCESS_DATA_WRITEONLY ;
         }
         else if ( !writeAllowed && readAllowed )
         {
            mode = DS_ACCESS_DATA_READONLY ;
         }
         else if ( !( writeAllowed || readAllowed ) )
         {
            mode = DS_ACCESS_DATA_NONE ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 catCalcErrorFilter( const CHAR *filterStr, INT32 &filter )
   {
      INT32 rc = SDB_OK ;

      filter = DS_ERR_FILTER_NONE ;

      // The default value for error filter mask is 0, which means no
      // error will be filterd. So if any mask is given, just add it.
      if ( 0 == ossStrlen( filterStr ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Error filter mask is empty[%d]", rc ) ;
         goto error ;
      }

      try
      {
         vector<string> values = utilStrSplit( filterStr, "|") ;
         for ( vector<string>::const_iterator itr = values.begin();
               itr != values.end(); ++itr )
         {
            if ( 0 == ossStrcasecmp( itr->c_str(), VALUE_NAME_READ ) )
            {
               filter |= DS_ERR_FILTER_READ ;
            }
            else if ( 0 == ossStrcasecmp( itr->c_str(),
                                          VALUE_NAME_WRITE ) )
            {
               filter |= DS_ERR_FILTER_WRITE;
            }
            else if ( 0 == ossStrcasecmp( itr->c_str(), VALUE_NAME_ALL ) )
            {
               filter |= DS_ERR_FILTER_ALL ;
            }
            else if ( 0 != ossStrcasecmp( itr->c_str(), VALUE_NAME_NONE ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Invalid error filter[%s] for data "
                                    "source[%d]", itr->c_str(), rc ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDCreateDataSource )

   _catCMDCreateDataSource::_catCMDCreateDataSource()
   {
   }

   _catCMDCreateDataSource::~_catCMDCreateDataSource()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDCREATEDATASOURCE_INIT, "_catCMDCreateDataSource::init" )
   INT32 _catCMDCreateDataSource::init( const CHAR *pQuery,
                                        const CHAR *pSelector,
                                        const CHAR *pOrderBy,
                                        const CHAR *pHint,
                                        INT32 flags,
                                        INT64 numToSkip,
                                        INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDCREATEDATASOURCE_INIT ) ;

      try
      {
         BSONObj infoObj( pQuery ) ;
         BSONObjIterator itr( infoObj ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            const CHAR* fieldName = e.fieldName() ;
            if ( 0 ==  ossStrcmp( fieldName, FIELD_NAME_NAME ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               _dsInfo._dsName = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ADDRESS ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               _dsInfo._addresses = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_USER ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               _dsInfo._user = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_PASSWD ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               _dsInfo._password = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_TYPE ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               _dsInfo._type = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_VERSION ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               _dsInfo._dsVersion = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ACCESSMODE ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               rc = catCalcAccessMode( e.valuestr(), _dsInfo._accessMode ) ;
               PD_RC_CHECK( rc, PDERROR, "Calculate access mode for data "
                            "source failed, rc: %d", rc ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ERRORFILTERMASK ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               rc =  catCalcErrorFilter( e.valuestr(), _dsInfo._errFilterMask ) ;
               PD_RC_CHECK( rc, PDERROR, "Calculate error filter for data "
                            "source failed, rc: %d", rc ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ERRORCTLLEVEL ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               if ( 0 == ossStrcasecmp( e.valuestr(), VALUE_NAME_HIGH ) )
               {
                  _dsInfo._errCtlLevel = VALUE_NAME_HIGH ;
               }
               else if ( 0 == ossStrcasecmp( e.valuestr(), VALUE_NAME_LOW ) )
               {
                  _dsInfo._errCtlLevel = VALUE_NAME_LOW ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Error control level value[%s] is "
                              "invalid[%d]", e.valuestr(), rc ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( fieldName,
                                      FIELD_NAME_TRANS_PROPAGATE_MODE ) )
            {
               PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not string, rc: %d",
                         fieldName, rc ) ;
               if ( 0 != ossStrcasecmp( e.valuestr(), VALUE_NAME_NEVER ) &&
                    0 != ossStrcasecmp( e.valuestr(), VALUE_NAME_NOT_SUPPORT ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Transaction propagate mode value[%s] "
                              "is invalid[%d]", e.valuestr(), rc ) ;
                  goto error ;
               }
               _dsInfo._transPropagateMode = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( fieldName,
                                      FIELD_NAME_INHERIT_SESSION_ATTR ) )
            {
               PD_CHECK( Bool == e.type(), SDB_INVALIDARG, error, PDERROR,
                         "Type of field[%s] is not bool, rc: %d",
                         fieldName, rc ) ;
               _dsInfo._inheritSessionAttr = e.boolean() ;
            }
            else
            {
               rc = SDB_OPTION_NOT_SUPPORT ;
               PD_LOG( PDERROR, "Invalid data source option[%s]", fieldName ) ;
               goto error ;
            }
         }

         if ( !_dsInfo._type )
         {
            _dsInfo._type = VALUE_NAME_SEQUOIADB ;
         }
         else if ( 0 != ossStrcasecmp( _dsInfo._type, VALUE_NAME_SEQUOIADB ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "The type of data source can only be \'%s\': "
                        "%s", VALUE_NAME_SEQUOIADB, _dsInfo._type ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDCREATEDATASOURCE_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDCREATEDATASOURCE_DOIT, "_catCMDCreateDataSource::doit" )
   INT32 _catCMDCreateDataSource::doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf,
                                        INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDCREATEDATASOURCE_DOIT ) ;
      BSONObj dummyObj ;
      BSONObj record ;
      pmdKRCB *krCB = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krCB->getDMSCB() ;
      SDB_DPSCB *dpsCB = krCB->getDPSCB() ;

      try
      {
         BSONObj selector = BSON( FIELD_NAME_ID << "" ) ;
         BSONObj order = BSON( FIELD_NAME_ID << -1 ) ;

         // Get max id in the SYSDATASOURCES collection.
         rc = catGetOneObjByOrder( CAT_DATASOURCE_COLLECTION, selector,
                                   dummyObj, order, dummyObj, cb, record ) ;
         if ( SDB_DMS_EOC == rc )
         {
            _dsInfo._id = 1 ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get max id from syscollection [%s]: %d",
                     CAT_DATASOURCE_COLLECTION, rc ) ;
            goto error ;
         }
         PD_LOG( PDDEBUG, "Data source with the max id: %s",
                  record.toString().c_str() ) ;
         _dsInfo._id = record.firstElement().number() + 1 ;

         rc = rtnInsert( CAT_DATASOURCE_COLLECTION, _dsInfo.toBson(), 1, 0,
                         cb, dmsCB, dpsCB,
                         sdbGetCatalogueCB()->majoritySize( TRUE ) ) ;
         if ( rc )
         {
            if ( SDB_IXM_DUP_KEY == rc )
            {
               rc = SDB_CAT_DATASOURCE_EXIST ;
               PD_LOG( PDERROR, "Data source[%s] exists already",
                       _dsInfo._dsName ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Insert data source metadata[%s] into "
                       "collection[%s] failed[%d]",
                       _dsInfo.toBson().toString().c_str(),
                       CAT_DATASOURCE_COLLECTION, rc) ;
            }
            goto error ;
         }
         PD_LOG( PDDEBUG, "Create data source[%s] successfully",
                 _dsInfo._dsName ) ;
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDCREATEDATASOURCE_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _catCMDCreateDataSource::name() const
   {
      return CMD_NAME_CREATE_DATASOURCE ;
   }

   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDDropDataSource )
   _catCMDDropDataSource::_catCMDDropDataSource()
   : _name( NULL ),
     _dsID( UTIL_INVALID_DS_UID )
   {
   }

   _catCMDDropDataSource::~_catCMDDropDataSource()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPDATASOURCE_INIT, "_catCMDDropDataSource::init" )
   INT32 _catCMDDropDataSource::init( const CHAR *pQuery,
                                      const CHAR *pSelector,
                                      const CHAR *pOrderBy,
                                      const CHAR *pHint,
                                      INT32 flags,
                                      INT64 numToSkip,
                                      INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDDROPDATASOURCE_INIT ) ;

      try
      {
         BSONObj query( pQuery ) ;
         BSONElement nameEle = query.getField( FIELD_NAME_NAME ) ;
         PD_CHECK( nameEle.type() == String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get data source name from query" ) ;
         _name = nameEle.valuestr() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPDATASOURCE_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPDATASOURCE_DOIT, "_catCMDDropDataSource::doit" )
   INT32 _catCMDDropDataSource::doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf,
                                      INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDDROPDATASOURCE_DOIT ) ;
      BSONObj matcher ;
      BSONObj dummyObj ;
      utilDeleteResult delResult ;
      pmdKRCB *krCB = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krCB->getDMSCB() ;
      SDB_DPSCB *dpsCB = krCB->getDPSCB() ;

      try
      {
         BOOLEAN used = FALSE ;
         BSONObj record ;
         matcher = BSON( FIELD_NAME_NAME << _name ) ;

         rc = catGetOneObj( CAT_DATASOURCE_COLLECTION, dummyObj, matcher,
                            dummyObj, cb, record ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               // Data source of the given name does not exist.
               rc = SDB_CAT_DATASOURCE_NOTEXIST ;
               PD_LOG( PDERROR, "The data source[%s] to be dropped does not "
                       "exist", _name ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Get data source[%s] metadata failed[%d]",
                       _name, rc ) ;
            }
            goto error ;
         }

         _dsID = record.getIntField( FIELD_NAME_ID ) ;

         rc = _checkUsage( used, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check data source[%s] usage failed[%d]",
                      _name, rc ) ;

         if ( used )
         {
            rc = SDB_CAT_DATASOURCE_INUSE ;
            PD_LOG( PDERROR, "Data source can not be dropped when in use" ) ;
            goto error ;
         }
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = rtnDelete( CAT_DATASOURCE_COLLECTION, matcher, dummyObj, 0, cb,
                      dmsCB, dpsCB, sdbGetCatalogueCB()->majoritySize( TRUE ),
                      &delResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete record from "
                   "collection[%s]: %d", CAT_DATASOURCE_COLLECTION, rc ) ;

      if ( 0 == delResult.deletedNum() )
      {
         rc = SDB_CAT_DATASOURCE_NOTEXIST ;
         PD_LOG( PDERROR, "Data source[%s] does not exist", _name ) ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "Drop data source[%s] successfully", _name ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPDATASOURCE_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _catCMDDropDataSource::name() const
   {
      return CMD_NAME_DROP_DATASOURCE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDDROPDATASOURCE__CHECKUSAGE, "_catCMDDropDataSource::_checkUsage" )
   INT32 _catCMDDropDataSource::_checkUsage( BOOLEAN &used, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDDROPDATASOURCE__CHECKUSAGE ) ;
      used = FALSE ;

      try
      {
         INT64 count = 0 ;
         BSONObj dummyObj ;
         BSONObj matcher = BSON( FIELD_NAME_DATASOURCE_ID << _dsID ) ;

         rc = catGetObjectCount( CAT_COLLECTION_SPACE_COLLECTION, dummyObj,
                                 matcher, dummyObj, cb, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collectionspace number of using data "
                      "source[%s] failed[%d]", _name, rc ) ;
         if ( count > 0 )
         {
            used = TRUE ;
            goto done ;
         }

         rc = catGetObjectCount( CAT_COLLECTION_INFO_COLLECTION, dummyObj,
                                 matcher, dummyObj, cb, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection number of using data "
                                   "source[%s] failed[%d]", _name, rc ) ;
         if ( count > 0 )
         {
            used = TRUE ;
            goto done ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDDROPDATASOURCE__CHECKUSAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDAlterDataSource )
   _catCMDAlterDataSource::_catCMDAlterDataSource()
   : _dsID( UTIL_INVALID_DS_UID )
   {

   }

   _catCMDAlterDataSource::~_catCMDAlterDataSource()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERATASOURCE_INIT, "_catCMDAlterDataSource::init" )
   INT32 _catCMDAlterDataSource::init( const CHAR *pQuery,
                                       const CHAR *pSelector,
                                       const CHAR *pOrderBy,
                                       const CHAR *pHint,
                                       INT32 flags,
                                       INT64 numToSkip,
                                       INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERATASOURCE_INIT ) ;
      const CHAR *name = NULL ;

      try
      {
         BSONObj dummyObj ;
         BSONObj currentMeta ;
         BSONElement argEle ;
         BSONObjBuilder builder ;
         BSONObj alterObj( pQuery ) ;

         argEle = alterObj.getField( FIELD_NAME_NAME ) ;
         PD_CHECK( String == argEle.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field[%s] from query when altering data "
                   "source: %s", FIELD_NAME_NAME,
                   alterObj.toString().c_str() ) ;
         name = argEle.valuestr() ;
         argEle = alterObj.getField( FIELD_NAME_OPTIONS ) ;
         if ( argEle.eoo() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "No valid alter data source options" ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "Alter data source options: %s",
                 argEle.toString().c_str() ) ;

         rc = _getDataSourceMeta( name, currentMeta ) ;
         PD_RC_CHECK( rc, PDERROR, "Get metadata of data source[%s] failed[%d]",
                      name, rc ) ;

         {
            BSONObjIterator itr( argEle.embeddedObject() );

            UINT32 newVersion = currentMeta.getIntField( FIELD_NAME_ID ) + 1 ;
            _dsID = currentMeta.getIntField( FIELD_NAME_ID ) ;
            while ( itr.more() )
            {
               BSONElement currEle ;
               BSONElement e = itr.next();
               const CHAR *fieldName = e.fieldName();
               if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ID ) ||
                    0 == ossStrcmp( fieldName, FIELD_NAME_TYPE ) ||
                    0 == ossStrcmp( fieldName, FIELD_NAME_VERSION ) ||
                    0 == ossStrcmp( fieldName, FIELD_NAME_ACCESSMODE_DESC) ||
                    0 == ossStrcmp( fieldName, FIELD_NAME_ERRORFILTERMASK_DESC ) )
               {
                  rc = SDB_OPTION_NOT_SUPPORT ;
                  PD_LOG_MSG( PDERROR, "Data source ID, type, version, access "
                              "mode description and error filter mask "
                              "description are not allow to modify[%d]", rc ) ;
                  goto error ;
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_NAME ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc );
                  currEle = currentMeta.getField( FIELD_NAME_NAME ) ;
                  if ( 0 != ossStrcmp( currEle.valuestr(), e.valuestr() ) )
                  {
                     _optionBuilder.append( e ) ;
                  }

               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ADDRESS ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc );
                  currEle = currentMeta.getField( FIELD_NAME_ADDRESS ) ;
                  if ( 0 != ossStrcmp( currEle.valuestr(), e.valuestr() ) )
                  {
                     _optionBuilder.append( e ) ;
                  }
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_USER ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc );
                  currEle = currentMeta.getField( FIELD_NAME_USER ) ;
                  if ( 0 != ossStrcmp( currEle.valuestr(), e.valuestr() ) )
                  {
                     _optionBuilder.append( e ) ;
                  }
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_PASSWD ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc );
                  currEle = currentMeta.getField( FIELD_NAME_PASSWD ) ;
                  if ( 0 != ossStrcmp( currEle.valuestr(), e.valuestr() ) )
                  {
                     _optionBuilder.append( e ) ;
                  }
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_DSVERSION ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc );
                  currEle = currentMeta.getField( FIELD_NAME_DSVERSION ) ;
                  if ( 0 != ossStrcmp( currEle.valuestr(), e.valuestr() ) )
                  {
                     _optionBuilder.append( e ) ;
                  }
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ACCESSMODE ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc ) ;
                  INT32 accessMode = DS_ACCESS_DEFAULT ;
                  const CHAR *desc = NULL ;
                  rc = catCalcAccessMode( e.valuestr(), accessMode ) ;
                  PD_RC_CHECK( rc, PDERROR, "Calculate access mode for data "
                                            "source failed, rc: %d", rc ) ;
                  _optionBuilder.append( FIELD_NAME_ACCESSMODE, accessMode ) ;
                  DS_ACCESS_MODE_2_DESC( accessMode, desc ) ;
                  _optionBuilder.append( FIELD_NAME_ACCESSMODE_DESC, desc ) ;
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ERRORFILTERMASK ) )
               {
                  INT32 errFilterMask = DS_ERR_FILTER_NONE ;
                  const CHAR *desc = NULL ;
                  rc = catCalcErrorFilter( e.valuestr(), errFilterMask ) ;
                  PD_RC_CHECK( rc, PDERROR, "Calculate error filter for data "
                                            "source failed, rc: %d", rc ) ;
                  _optionBuilder.append( FIELD_NAME_ERRORFILTERMASK,
                                         errFilterMask ) ;
                  DS_ERR_FILTER_2_DESC( errFilterMask, desc ) ;
                  _optionBuilder.append( FIELD_NAME_ERRORFILTERMASK_DESC,
                                         desc ) ;
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_ERRORCTLLEVEL ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc ) ;
                  if ( 0 == ossStrcasecmp( e.valuestr(), VALUE_NAME_HIGH ) )
                  {
                     _optionBuilder.append( FIELD_NAME_ERRORCTLLEVEL,
                                            VALUE_NAME_HIGH ) ;
                  }
                  else if ( 0 == ossStrcasecmp( e.valuestr(), VALUE_NAME_LOW ) )
                  {
                     _optionBuilder.append( FIELD_NAME_ERRORCTLLEVEL,
                                            VALUE_NAME_LOW ) ;
                  }
                  else
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Error control level value[%s] is "
                                          "invalid[%d]", e.valuestr(), rc ) ;
                     goto error ;
                  }
               }
               else if ( 0 == ossStrcmp( fieldName,
                                         FIELD_NAME_TRANS_PROPAGATE_MODE ) )
               {
                  PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not string, rc: %d",
                            fieldName, rc ) ;
                  if ( 0 != ossStrcasecmp( e.valuestr(), VALUE_NAME_NEVER ) &&
                       0 != ossStrcasecmp( e.valuestr(), VALUE_NAME_NOT_SUPPORT ) )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Transaction propagate mode value[%s]"
                                 " is invalid[%d]", e.valuestr(), rc ) ;
                     goto error ;
                  }

                  currEle = currentMeta.getField( FIELD_NAME_TRANS_PROPAGATE_MODE ) ;
                  // For data source of old version, there is no
                  // "TransPropagateMode" field in the meta data. After altering
                  // the field, it will be added into the metadata record.
                  if ( currEle.eoo() ||
                       ( 0 != ossStrcasecmp( currEle.valuestr(),
                                             e.valuestr() ) ) )
                  {
                     _optionBuilder.append( FIELD_NAME_TRANS_PROPAGATE_MODE,
                                            e.valuestr() ) ;
                  }
               }
               else if ( 0 == ossStrcmp( fieldName,
                                         FIELD_NAME_INHERIT_SESSION_ATTR ) )
               {
                  PD_CHECK( Bool == e.type(), SDB_INVALIDARG, error, PDERROR,
                            "Type of field[%s] is not bool, rc: %d",
                            fieldName, rc ) ;
                  _optionBuilder.append( FIELD_NAME_INHERIT_SESSION_ATTR,
                                         e.boolean() ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Invalid alter data source option[%s]",
                          fieldName ) ;
                  goto error ;
               }
            }

            builder.append( FIELD_NAME_VERSION, newVersion ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERATASOURCE_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERATASOURCE_DOIT, "_catCMDAlterDataSource::doit" )
   INT32 _catCMDAlterDataSource::doit( _pmdEDUCB *cb,
                                       rtnContextBuf &ctxBuf,
                                       INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERATASOURCE_DOIT ) ;
      pmdKRCB *krCB = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krCB->getDMSCB() ;
      SDB_DPSCB *dpsCB = krCB->getDPSCB() ;

      try
      {
         BSONObj dummyObj ;
         BSONObj updator ;
         BSONObj matcher = BSON( FIELD_NAME_ID << _dsID ) ;
         if ( _optionBuilder.isEmpty() )
         {
            goto done ;
         }

         updator = BSON( "$set" << _optionBuilder.done() ) ;
         PD_LOG( PDDEBUG, "Data source updator: %s",
                 updator.toString().c_str() ) ;

         rc = rtnUpdate( CAT_DATASOURCE_COLLECTION, matcher, updator, dummyObj,
                         0, cb, dmsCB, dpsCB,
                         sdbGetCatalogueCB()->majoritySize( TRUE ) ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CAT_DATASOURCE_NOTEXIST ;
         }
         else if ( SDB_IXM_DUP_KEY == rc )
         {
            // Name conflict
            rc = SDB_CAT_DATASOURCE_EXIST ;
         }
         PD_RC_CHECK( rc, PDERROR, "Update data source metadata failed[%d]",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERATASOURCE_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _catCMDAlterDataSource::name() const
   {
      return CMD_NAME_ALTER_DATASOURCE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERATASOURCE__GETDATASOURCEMETA, "_catCMDAlterDataSource::_getDataSourceMeta" )
   INT32 _catCMDAlterDataSource::_getDataSourceMeta( const CHAR *name,
                                                     BSONObj &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERATASOURCE__GETDATASOURCEMETA ) ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      SDB_ASSERT( name, "Data source name is NULL" ) ;

      try
      {
         BSONObj dummyObj ;
         BSONObj matcher ;
         matcher = BSON( FIELD_NAME_NAME << name ) ;
         rc = catGetOneObj( CAT_DATASOURCE_COLLECTION, dummyObj, matcher,
                            dummyObj, cb, record ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_CAT_DATASOURCE_NOTEXIST ;
            }
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERATASOURCE__GETDATASOURCEMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDTestCollection )

   _catCMDTestCollection::_catCMDTestCollection()
   : _name( NULL )
   {
   }

   _catCMDTestCollection::~_catCMDTestCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDTESTCOLLECTION_INIT, "_catCMDTestCollection::init" )
   INT32 _catCMDTestCollection::init( const CHAR *pQuery, const CHAR *pSelector,
                                      const CHAR *pOrderBy, const CHAR *pHint,
                                      INT32 flags, INT64 numToSkip,
                                      INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDTESTCOLLECTION_INIT ) ;

      try
      {
         BSONObj query( pQuery ) ;
         BSONElement ele = query.getField( FIELD_NAME_NAME ) ;
         _name = ele.valuestrsafe() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e )  ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDTESTCOLLECTION_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDTESTCOLLECTION_DOIT, "_catCMDTestCollection::doit" )
   INT32 _catCMDTestCollection::doit( pmdEDUCB *cb, rtnContextBuf &ctxBuf,
                                      INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDTESTCOLLECTION_DOIT ) ;

      if ( 0 == ossStrcmp( _name, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         goto done ;
      }

      try
      {
         BSONObj collection ;
         rc = catGetAndLockCollection ( _name, collection, cb, NULL, SHARED ) ;
         // compatible with old version
         if( SDB_DMS_CS_NOTEXIST == rc )
         {
            rc = SDB_DMS_NOTEXIST ;
         }
         PD_RC_CHECK( rc, PDERROR, "cat get collection[%s] failed[%d]",
                      _name, rc ) ;
         ctxBuf = rtnContextBuf( collection ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDTESTCOLLECTION_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_catCMDTestCollection::name() const
   {
      return CMD_NAME_TEST_COLLECTION ;
   }

   /*
      catCMDCreateCS implement
   */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER(_catCMDCreateCS)
   _catCMDCreateCS::_catCMDCreateCS()
   {
      _csInfo.reset() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTCS_INIT, "_catCMDCreateCS::init" )
   INT32 _catCMDCreateCS::init( const CHAR *pQuery,
                                const CHAR *pSelector,
                                const CHAR *pOrderBy,
                                const CHAR *pHint,
                                INT32 flags,
                                INT64 numToSkip,
                                INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTCS_INIT ) ;

      INT32 expected = 0 ;

      try
      {

      BSONObj boQuery( pQuery ) ;
      BSONObjIterator it( boQuery ) ;
      while ( it.more() )
      {
         BSONElement ele = it.next() ;

         // name
         if ( 0 == ossStrcmp( ele.fieldName(), CAT_COLLECTION_SPACE_NAME ) )
         {
            PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_COLLECTION_NAME,
                      ele.type() ) ;
            _csInfo._pCSName = ele.valuestr() ;
            ++expected ;
         }
         // page size
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_PAGE_SIZE_NAME ) )
         {
            PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_PAGE_SIZE_NAME,
                      ele.type() ) ;
            if ( 0 != ele.numberInt() )
            {
               _csInfo._pageSize = ele.numberInt() ;
            }

            // check size value
            PD_CHECK ( _csInfo._pageSize == DMS_PAGE_SIZE4K ||
                       _csInfo._pageSize == DMS_PAGE_SIZE8K ||
                       _csInfo._pageSize == DMS_PAGE_SIZE16K ||
                       _csInfo._pageSize == DMS_PAGE_SIZE32K ||
                       _csInfo._pageSize == DMS_PAGE_SIZE64K, SDB_INVALIDARG,
                       error, PDERROR, "PageSize must be 4K/8K/16K/32K/64K" ) ;
            ++expected ;
         }
         // domain name
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_DOMAIN_NAME ) )
         {
            PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_DOMAIN_NAME,
                      ele.type() ) ;
            _csInfo._domainName = ele.valuestr() ;
            ++expected ;
         }
         // lob page size
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_LOB_PAGE_SZ_NAME ) )
         {
            PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_LOB_PAGE_SZ_NAME,
                      ele.type() ) ;
            if ( 0 != ele.numberInt() )
            {
               _csInfo._lobPageSize = ele.numberInt() ;
            }

            PD_CHECK ( _csInfo._lobPageSize == DMS_PAGE_SIZE4K ||
                       _csInfo._lobPageSize == DMS_PAGE_SIZE8K ||
                       _csInfo._lobPageSize == DMS_PAGE_SIZE16K ||
                       _csInfo._lobPageSize == DMS_PAGE_SIZE32K ||
                       _csInfo._lobPageSize == DMS_PAGE_SIZE64K ||
                       _csInfo._lobPageSize == DMS_PAGE_SIZE128K ||
                       _csInfo._lobPageSize == DMS_PAGE_SIZE256K ||
                       _csInfo._lobPageSize == DMS_PAGE_SIZE512K,
                       SDB_INVALIDARG, error, PDERROR,
                       "PageSize must be 4K/8K/16K/32K/64K/128K/256K/512K" ) ;
            ++expected ;
         }
         // capped option
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_CAPPED_NAME ) )
         {
            PD_CHECK( ele.isBoolean(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", CAT_CAPPED_NAME,
                      ele.type() ) ;
            _csInfo._type = ( true == ele.boolean() ) ?
                           DMS_STORAGE_CAPPED : DMS_STORAGE_NORMAL ;
            ++expected ;
         }
         // Data source option
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DATASOURCE ) )
         {
            PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", FIELD_NAME_DATASOURCE,
                      ele.type() ) ;
            UINT32 len = ossStrlen( ele.valuestr() ) ;
            PD_CHECK( len > 0 && len <= DATASOURCE_MAX_NAME_SZ, SDB_INVALIDARG,
                      error, PDERROR, "Length of data source name should be "
                      "greater than 0 and less than %u",
                      DATASOURCE_MAX_NAME_SZ ) ;
            _csInfo._pDataSourceName = ele.valuestr() ;
            ++expected ;
         }
         // Data source mapping option
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MAPPING ) )
         {
            PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type[%d] error", FIELD_NAME_MAPPING,
                      ele.type() ) ;
            PD_CHECK( ossStrlen( ele.valuestr() ) > 1, SDB_INVALIDARG, error,
                      PDERROR, "Length of mapping should be greater than 0" ) ;
            _csInfo._pDataSourceMapping = ele.valuestr() ;
            ++expected ;
         }
         else
         {
            PD_RC_CHECK ( SDB_INVALIDARG, PDERROR,
                          "Unexpected field[%s] in create collection space "
                          "command", ele.toString().c_str() ) ;
         }
      }

      PD_CHECK( _csInfo._pCSName, SDB_INVALIDARG, error, PDERROR,
                "Collection space name not set" ) ;

      PD_CHECK( boQuery.nFields() == expected, SDB_INVALIDARG, error, PDERROR,
                "unexpected fields exsit." ) ;

      if ( _csInfo._pDataSourceMapping )
      {
         if ( !_csInfo._pDataSourceName )
         {
            // Mapping cannot be set without data source.
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Data source name should be specified when using "
                             "mapping option[%d]", rc ) ;
            goto error ;
         }
      }
      else if ( _csInfo._pDataSourceName )
      {
         // If data source mapping is not set, map to the same collection space.
         _csInfo._pDataSourceMapping = _csInfo._pCSName ;
      }

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTCS_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTCS_DOIT, "_catCMDCreateCS::doit" )
   INT32 _catCMDCreateCS::doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf,
                                INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTCS_DOIT ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      INT16 w = sdbGetCatalogueCB()->majoritySize() ;
      const CHAR *csName = _csInfo._pCSName ;
      const CHAR *domainName = _csInfo._domainName ;
      BOOLEAN isSpaceExist = FALSE ;
      BSONObj spaceObj ;
      BSONObj domainObj ;
      vector< UINT32 >  domainGroups ;
      catCtxLockMgr lockMgr ;
      string strGroupName ;

      PD_TRACE1 ( SDB_CATCMDCRTCS_DOIT, PD_PACK_STRING ( csName ) ) ;

      // name check
      rc = dmsCheckCSName( csName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Check collection space name [%s] failed, rc: %d",
                   csName, rc ) ;

      // check collection space is whether existed or not
      rc = catCheckSpaceExist( csName, isSpaceExist, spaceObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check existence of collection space [%s], rc: %d",
                   csName, rc ) ;
      PD_TRACE1 ( SDB_CATALOGMGR_CREATECS, PD_PACK_INT ( isSpaceExist ) ) ;
      PD_CHECK( FALSE == isSpaceExist,
                SDB_DMS_CS_EXIST, error, PDERROR,
                "Collection space [%s] is already existed",
                csName ) ;

      // check data source is whether existed or not
      if ( _csInfo._pDataSourceName )
      {
         BOOLEAN exist = FALSE ;
         try
         {
            BSONObj obj ;
            rc = catCheckDataSourceExist( _csInfo._pDataSourceName,
                                          exist, obj, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Check existence of data source[%s] "
                         "failed[%d]", _csInfo._pDataSourceName, rc ) ;
            PD_CHECK( TRUE == exist, SDB_CAT_DATASOURCE_NOTEXIST, error,
                      PDERROR, "Data source[%s] dose not exist",
                      _csInfo._pDataSourceName ) ;

            // Get the data source unique id. It will be stored in the metadata
            // of the collection space.
            _csInfo._dsUID = obj.getIntField( FIELD_NAME_ID ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Exception occurred when getting information of "
                    "data source[%s]: %s", _csInfo._pDataSourceName, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      // Lock collection space
      PD_CHECK( lockMgr.tryLockCollectionSpace( csName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock collection space [%s]",
                csName ) ;

      // check domain name
      if ( domainName )
      {
         PD_TRACE1 ( SDB_CATALOGMGR_CREATECS, PD_PACK_STRING ( domainName ) ) ;

         rc = catGetAndLockDomain( domainName, domainObj, cb,
                                   &lockMgr, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get domain [%s] obj, rc: %d",
                      domainName, rc ) ;

         rc = catGetDomainGroups( domainObj, domainGroups ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get domain [%s] groups failed, rc: %d",
                      domainObj.toString().c_str(), rc ) ;

         for ( UINT32 i = 0 ; i < domainGroups.size() ; ++i )
         {
            rc = catGroupID2Name( domainGroups[i], strGroupName, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Group id [%u] to group name failed, "
                         "rc: %d", domainGroups[i], rc ) ;
            // Lock data group in this domain
            PD_CHECK( lockMgr.tryLockGroup( strGroupName, SHARED ),
                      SDB_LOCK_FAILED, error, PDERROR,
                      "Failed to lock group [%s]",
                      strGroupName.c_str() ) ;
         }
      }

      // set CSUniqueHWM, get cs unique id
      rc = catUpdateCSUniqueID( cb, w, _csInfo._csUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Fail to get cs unique id, rc: %d.", rc ) ;

      _csInfo._clUniqueHWM = utilBuildCLUniqueID( _csInfo._csUniqueID, 0 ) ;

      // insert new Collection Space record
      {
         BSONObjBuilder newBuilder ;
         newBuilder.appendElements( _csInfo.toBson() ) ;
         BSONObjBuilder sub1( newBuilder.subarrayStart( CAT_COLLECTION ) ) ;
         sub1.done() ;

         BSONObj newObj = newBuilder.obj() ;

         rc = rtnInsert( CAT_COLLECTION_SPACE_COLLECTION, newObj, 1, 0,
                         cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to insert collection space obj [%s] "
                      "to collection [%s], rc: %d",
                      newObj.toString().c_str(),
                      CAT_COLLECTION_SPACE_COLLECTION, rc ) ;
      }

      PD_LOG( PDDEBUG,
              "Created collection space[name: %s, id: %u] succeed.",
              csName, _csInfo._csUniqueID ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTCS_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   static INT32 catNewCataset( const CHAR* collection,
                               _pmdEDUCB *cb,
                               clsCatalogSet*& pCataSet )
   {
      INT32 rc = SDB_OK ;
      BSONObj boCollection ;

      SDB_ASSERT( NULL == pCataSet, "pCataSet should be null") ;

      pCataSet = SDB_OSS_NEW clsCatalogSet( collection ) ;
      PD_CHECK( pCataSet, SDB_OOM, error, PDERROR, "Allocate failed" ) ;

      rc = catGetCollection( collection, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the collection[%s], rc: %d",
                   collection, rc ) ;

      rc = pCataSet->updateCatSet( boCollection ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse cata info[%s], rc: %d",
                   boCollection.toString().c_str(), rc ) ;

   done:
      return rc ;
   error:
      if ( pCataSet )
      {
         SDB_OSS_DEL pCataSet ;
         pCataSet = NULL ;
      }
      goto done ;
   }

   static void catReleaseCataset( clsCatalogSet*& pCataSet )
   {
      if ( pCataSet )
      {
         SDB_OSS_DEL pCataSet ;
         pCataSet = NULL ;
      }
   }

   static INT32 catCreateCS( const CHAR* csName,
                             const CHAR* domainName,
                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT64 contextID = -1 ;
      rtnContextBuf ctxBuf ;
      BSONObj boQuery ;
      BSONObjBuilder builder ;

      try
      {
         builder.append( FIELD_NAME_NAME, csName ) ;
         if ( domainName && '\0' != domainName[0] )
         {
            builder.append( FIELD_NAME_DOMAIN, domainName ) ;
         }
         boQuery = builder.done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      {
      catCMDCreateCS cmd ;
      rc = cmd.init( boQuery.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init create cs[%s] command, cl: %s, rc: %d",
                   csName, rc ) ;

      rc = cmd.doit( cb, ctxBuf, contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to do create cs[%s] command, cl: %s, rc: %d",
                   csName, rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 catCreateCL( const CHAR* clName,
                             const CHAR* shardingType,
                             const BSONObj& shardingKey,
                             BOOLEAN ensureShardIdx,
                             BOOLEAN autoSplit,
                             _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf ctxBuf ;
      BSONObj boQuery ;
      BSONObjBuilder builder ;

      try
      {
         builder.append( FIELD_NAME_NAME, clName ) ;
         builder.append( FIELD_NAME_SHARDTYPE, shardingType ) ;
         builder.append( FIELD_NAME_SHARDINGKEY, shardingKey ) ;
         builder.appendBool( FIELD_NAME_ENSURE_SHDINDEX, ensureShardIdx ) ;
         builder.appendBool( FIELD_NAME_DOMAIN_AUTO_SPLIT, autoSplit ) ;
         boQuery = builder.done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      {
      catCtxCreateCL catCtx( -1, cb->getID() ) ;
      rc = catCtx.open( boQuery.objdata(), ctxBuf, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to open context, rc: %d",
                   rc ) ;

      while( TRUE )
      {
         rc = catCtx.getMore( -1, ctxBuf, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to Get more from create-cl context, rc: %d",
                      rc ) ;
      }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 catDropCL( const CHAR* clName, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf ctxBuf ;
      BSONObj boQuery ;

      try
      {
         boQuery = BSON( FIELD_NAME_NAME << clName ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      {
      catCtxDropCL catCtx( -1, cb->getID() ) ;
      rc = catCtx.open( boQuery.objdata(), ctxBuf, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to open context, rc: %d",
                   rc ) ;

      while( TRUE )
      {
         rc = catCtx.getMore( -1, ctxBuf, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to Get more from drop-cl context, rc: %d",
                      rc ) ;
      }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 catDropEmptyCS( const CHAR* csName, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf ctxBuf ;
      BSONObj boQuery ;

      try
      {
         boQuery = BSON( FIELD_NAME_NAME << csName <<
                         FIELD_NAME_ENSURE_EMPTY << true ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      {
      catCtxDropCS catCtx( -1, cb->getID() ) ;
      rc = catCtx.open( boQuery.objdata(), ctxBuf, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to open context, rc: %d",
                   rc ) ;

      while( TRUE )
      {
         rc = catCtx.getMore( -1, ctxBuf, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to Get more from drop-cs context, rc: %d",
                      rc ) ;
      }
      }
   done:
      return rc ;
   error:
      goto done ;
   }


   /*
      catCMDIndex implement
   */
   _catCMDIndexHelper::_catCMDIndexHelper( BOOLEAN sysCall )
   : _pCataSet( NULL ),
     _pCollection( NULL ),
     _pIndexName( NULL ),
     _sysCall( sysCall )
   {
   }

   _catCMDIndexHelper::~_catCMDIndexHelper()
   {
      for( VEC_TASKS_IT it = _vecTasks.begin() ; it != _vecTasks.end() ; it++ )
      {
         if ( *it )
         {
            SDB_OSS_DEL *it ;
            *it = NULL ;
         }
      }
      _vecTasks.clear() ;

      if ( _pCataSet )
      {
         catReleaseCataset( _pCataSet ) ;
      }
   }

   INT32 _catCMDIndexHelper::_makeReply( UINT64 taskID, rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder replyBuild ;

      // reply: { TaskID: xxx,
      //          Group: [ { GroupID: 1000, GroupName: "db1" }, ... ] }
      try
      {
         if ( CLS_INVALID_TASKID != taskID )
         {
            replyBuild.append( CAT_TASKID_NAME, (INT64)taskID ) ;
         }

         vector<string> groupNameList ;
         for ( ossPoolSet<ossPoolString>::iterator it = _groupSet.begin() ;
               it != _groupSet.end() ; it++ )
         {
            groupNameList.push_back( it->c_str() ) ;
         }

         sdbGetCatalogueCB()->makeGroupsObj( replyBuild, groupNameList, TRUE ) ;

         ctxBuf = rtnContextBuf( replyBuild.obj() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDIDX_DROPGIDXCL, "_catCMDIndexHelper::_dropGlobalIdxCL" )
   INT32 _catCMDIndexHelper::_dropGlobalIdxCL( const CHAR* clName,
                                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDIDX_DROPGIDXCL ) ;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

      rc = catDropCL( clName, cb ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to drop global index collection[%s], rc: %d",
                   clName, rc ) ;

      rc = rtnResolveCollectionSpaceName( clName,
                                          ossStrlen( clName ),
                                          csName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get cs name from cl name[%s],rc: %d",
                   clName, rc ) ;

      rc = catDropEmptyCS( csName, cb ) ;
      if ( SDB_DMS_CS_NOT_EMPTY == rc || SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to drop global index collection space[%s], "
                   "rc: %d", csName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCMDIDX_DROPGIDXCL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDIDX_CHKTASKCONF, "_catCMDIndexHelper::_checkTaskConflict" )
   INT32 _catCMDIndexHelper::_checkTaskConflict( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB_CATCMDIDX_CHKTASKCONF ) ;

      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      BSONObj dummyObj, matcher ;
      INT64 contextID = -1 ;
      BOOLEAN hasConflictTask = FALSE ;

      // query tasks
      matcher = BSON( FIELD_NAME_NAME << _pCollection <<
                      CAT_STATUS_NAME << BSON( "$ne" <<
                                               CLS_TASK_STATUS_FINISH ) ) ;

      rc = rtnQuery( CAT_TASK_INFO_COLLECTION,
                     dummyObj, matcher, dummyObj, dummyObj,
                     0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to perform query, rc = %d", rc ) ;

      // loop every index task
      while ( !hasConflictTask )
      {
         rtnContextBuf contextBuf ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to retreive record, rc: %d", rc ) ;

         rc = _checkTaskConflict( BSONObj( contextBuf.data() ),
                                  hasConflictTask ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( hasConflictTask )
      {
         rc = SDB_CLS_MUTEX_TASK_EXIST ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_CATCMDIDX_CHKTASKCONF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDIDX_CHKTASKCONF2, "_catCMDIndexHelper::_checkTaskConflict" )
   INT32 _catCMDIndexHelper::_checkTaskConflict( const BSONObj& otherIdxObj,
                                                 BOOLEAN& conflict )
   {
      PD_TRACE_ENTRY( SDB_CATCMDIDX_CHKTASKCONF2 ) ;

      INT32 rc = SDB_OK ;
      clsTask *pOtherTask = NULL ;

      rc = clsNewTask( otherIdxObj, pOtherTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to new task, rc: %d",
                   rc ) ;

      if ( CLS_TASK_STATUS_FINISH == pOtherTask->status() )
      {
         conflict = FALSE ;
         goto done ;
      }

      for( VEC_TASKS_IT it = _vecTasks.begin() ; it != _vecTasks.end() ; ++it )
      {
         clsIdxTask* pCurTask = *it ;
         if ( pCurTask->taskID() == pOtherTask->taskID() ||
              pCurTask->muteXOn( pOtherTask ) ||
              pOtherTask->muteXOn( pCurTask )  )
         {
            conflict = TRUE ;
            PD_LOG_MSG( PDERROR,
                        "Task[%llu] conflicts with an existing task[%s]",
                        pCurTask->taskID(),
                        pOtherTask->toBson().toString().c_str() ) ;
            goto done ;
         }
         else
         {
            conflict = FALSE ;
         }
      }

   done:
      if ( pOtherTask )
      {
         clsReleaseTask( pOtherTask ) ;
      }
      PD_TRACE_EXITRC( SDB_CATCMDIDX_CHKTASKCONF2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      catCMDCreateIndex implement
   */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER(_catCMDCreateIndex)
   _catCMDCreateIndex::_catCMDCreateIndex( BOOLEAN sysCall )
   :_catCMDIndexHelper( sysCall ),
    _sortBufSz( SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ),
    _isPrepareStep( FALSE ),
    _isUnique( FALSE ),
    _isEnforced( FALSE ),
    _isGlobal( FALSE ),
    _globalIdxCLUniqID( UTIL_UNIQUEID_NULL )
   {
      ossMemset( _globalIdxCSName, 0, sizeof( _globalIdxCSName ) ) ;
      ossMemset( _globalIdxCLName, 0, sizeof( _globalIdxCLName ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_INIT, "_catCMDCreateIndex::init" )
   INT32 _catCMDCreateIndex::init( const CHAR *pQuery,
                                   const CHAR *pSelector,
                                   const CHAR *pOrderBy,
                                   const CHAR *pHint,
                                   INT32 flags,
                                   INT64 numToSkip,
                                   INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_INIT ) ;

      /** message formate:
       *  matcher: { Collection: "foo.bar",
       *             Index:{ key: {a:1}, name: 'aIdx', Unique: true,
       *                     Enforced: true, NotNull: true },
       *             SortBufferSize: 1024, Standalone: true, Async: true }
       *  hint: { SortBufferSize: 1024 }
       */
      try
      {
         BSONObj query, hint ;
         if ( pQuery )
         {
            query = BSONObj( pQuery ) ;
         }
         if ( pHint )
         {
            hint = BSONObj( pHint ) ;
         }

         // get collection
         rc = rtnGetStringElement( query, CAT_COLLECTION, &_pCollection ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_COLLECTION, rc ) ;

         // get index
         rc = rtnGetObjElement( query, FIELD_NAME_INDEX, _boIdx ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      FIELD_NAME_INDEX, rc ) ;

         rc = rtnConvertIndexDef( _boIdx ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to convert index definition" ) ;

         BSONObjIterator ii( _boIdx ) ;
         while ( ii.more() )
         {
            BSONElement e = ii.next();
            if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_NAME ) == 0 )
            {
               if ( e.type() != String )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Field[%s] invalid in obj[%s]",
                              IXM_FIELD_NAME_NAME, _boIdx.toString().c_str() ) ;
                  goto error ;
               }
               _pIndexName = e.valuestr() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_KEY ) == 0 )
            {
               if ( e.type() != Object )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Field[%s] invalid in obj[%s]",
                              IXM_FIELD_NAME_KEY, _boIdx.toString().c_str() ) ;
                  goto error ;
               }
               _key = e.Obj() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_UNIQUE ) == 0 )
            {
               if ( e.type() != Bool )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Field[%s] invalid in obj[%s]",
                              IXM_FIELD_NAME_UNIQUE,
                              _boIdx.toString().c_str() ) ;
                  goto error ;
               }
               _isUnique = e.boolean() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_ENFORCED ) == 0 )
            {
               if ( e.type() != Bool )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Field[%s] invalid in obj[%s]",
                              IXM_FIELD_NAME_ENFORCED,
                              _boIdx.toString().c_str() ) ;
                  goto error ;
               }
               _isEnforced = e.boolean() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_GLOBAL ) == 0 )
            {
               if ( e.type() != Bool )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Field[%s] invalid in obj[%s]",
                              IXM_FIELD_NAME_GLOBAL,
                              _boIdx.toString().c_str() ) ;
                  goto error ;
               }
               _isGlobal = e.boolean() ;
            }
         }

         // get sort buffer size
         if ( query.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
         {
            rc = rtnGetIntElement( query, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                   _sortBufSz ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field [%s] from query, rc: %d",
                         IXM_FIELD_NAME_SORT_BUFFER_SIZE, rc ) ;
         }
         else if ( hint.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
         {
            rc = rtnGetIntElement( hint, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                   _sortBufSz ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field [%s] from hint, rc: %d",
                         IXM_FIELD_NAME_SORT_BUFFER_SIZE, rc ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_DOIT, "_catCMDCreateIndex::doit" )
   INT32 _catCMDCreateIndex::doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf,
                                   INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_DOIT ) ;

      rc = _check( cb ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _execute( cb, ctxBuf ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_PDOIT, "_catCMDCreateIndex::postDoit" )
   INT32 _catCMDCreateIndex::postDoit( const clsTask *pTask, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_PDOIT ) ;

      SDB_ASSERT( pTask, "pTask cann't be null" ) ;
      SDB_ASSERT( CLS_TASK_CREATE_IDX == pTask->taskType(),
                  "Invalid task type" ) ;
      SDB_ASSERT( CLS_TASK_STATUS_FINISH == pTask->status(),
                  "Task status should be finished" ) ;

      clsCreateIdxTask *crtTask = (clsCreateIdxTask*)pTask ;
      INT16 w = sdbGetCatalogueCB()->majoritySize() ;

      if ( SDB_OK == crtTask->resultCode() ||
           SDB_IXM_REDEF == crtTask->resultCode() )
      {
         BOOLEAN addNewIdx = FALSE ;
         rc = catAddIndex( crtTask->collectionName(),
                           crtTask->indexDef(), cb, w, &addNewIdx ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add index, rc: %d",
                      rc ) ;

         // If data nodes has the same definition, while catalog hasn't this
         // index, then we should set task status to succeed. When creating
         // index, the master data node crash, which can cause this situation.
         if ( SDB_IXM_REDEF == crtTask->resultCode() && addNewIdx )
         {
            rc = catUpdateTask2Finish( crtTask->taskID(), SDB_OK, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update task, rc: %d",
                         rc ) ;
         }
      }
      else
      {
         const CHAR* globalIdxCL = NULL ;
         UINT64 globalIdxCLUniqID = 0 ;

         (void)crtTask->globalIdxCL( globalIdxCL, globalIdxCLUniqID ) ;

         if ( globalIdxCL )
         {
            (void)_dropGlobalIdxCL( globalIdxCL, cb ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_PDOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_CHK, "_catCMDCreateIndex::_check" )
   INT32 _catCMDCreateIndex::_check( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_CHK ) ;

      // get catalog set first
      rc = catNewCataset( _pCollection, cb, _pCataSet ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get collection[%s]'s cata info, rc: %d",
                   _pCollection, rc ) ;

      // check index key {a:1}
      if ( !ixmIndexCB::isValidKey( _key ) )
      {
         PD_LOG_MSG( PDERROR, "index key is invalid:%s",
                     _boIdx.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // check index name 'aIdx'
      rc = dmsCheckIndexName( _pIndexName, _sysCall ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( _isGlobal )
      {
         // check gloabl index, and generate index cl's name
         rc = _checkGlobalIndex( cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else if ( !dmsIsSysIndexName( _pIndexName ) )
      {
         // check unique index should include sharding key
         rc = _checkUniqueKey( cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      // check index exist and build tasks
      if ( _pCataSet->isMainCL() )
      {
         BOOLEAN mainCLRedef = FALSE ;

         rc = catCheckIndexExist( _pCollection, _pIndexName, _boIdx, cb ) ;
         if ( SDB_IXM_REDEF == rc )
         {
            // ignore error, because sub-collection may not have this index
            rc = SDB_OK ;
            mainCLRedef = TRUE ;
         }
         if ( rc )
         {
            goto error ;
         }

         rc = _buildMainCLTask( cb, mainCLRedef ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = catCheckIndexExist( _pCollection, _pIndexName, _boIdx, cb ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _buildCLTask( cb, _pCollection ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      // check task conflict
      rc = _checkTaskConflict( cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_CHK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _catCMDCreateIndex::_createGlobalIdxCL( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isSpaceExist = FALSE ;
      BSONObj spaceObj, boCollection ;

      // if global index collection's cs doesn't exist
      rc = catCheckSpaceExist( _globalIdxCSName, isSpaceExist, spaceObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check existence of collection space[%s], rc: %d",
                   _globalIdxCSName, rc ) ;

      if ( !isSpaceExist )
      {
         // create global index collection's cs
         rc = catCreateCS( _globalIdxCSName, _domainName.c_str(), cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create collection space[%s], rc: %d",
                      _globalIdxCSName, rc ) ;
      }

      // create global index collection
      rc = catCreateCL( _globalIdxCLName, FIELD_NAME_SHARDTYPE_HASH,
                        _key, FALSE, TRUE, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                      "Failed to create collection[%s], rc: %d",
                      _globalIdxCLName, rc ) ;

      // get global index collection's unique id
      rc = catGetCollection( _globalIdxCLName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the collection[%s], rc: %d",
                   _globalIdxCLName, rc ) ;

      rc = rtnGetNumberLongElement( boCollection, FIELD_NAME_UNIQUEID,
                                    (INT64&)_globalIdxCLUniqID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from collection[%s], rc: %d",
                   FIELD_NAME_UNIQUEID, boCollection.toString().c_str(), rc ) ;

      // create global index collection's index
      try
      {
         BSONObjBuilder builder ;
         const CHAR* indexName = _boIdx.getStringField( IXM_FIELD_NAME_NAME ) ;
         builder.append( IXM_FIELD_NAME_NAME, indexName ) ;
         builder.append( IXM_FIELD_NAME_KEY, _key ) ;
         builder.appendBool( IXM_FIELD_NAME_UNIQUE, _isUnique ) ;
         builder.appendBool( IXM_FIELD_NAME_ENFORCED, _isEnforced ) ;

         rc = catAddIndex( _globalIdxCLName, builder.done(),
                           cb, sdbGetCatalogueCB()->majoritySize() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create index[%s] for cl[%s], rc: %d",
                      indexName, _globalIdxCLName, rc ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catCMDCreateIndex::_addGlobalInfo2Task()
   {
      INT32 rc = SDB_OK ;

      for( VEC_TASKS_IT it = _vecTasks.begin() ; it != _vecTasks.end() ; it++ )
      {
         clsIdxTask *pTask = *it ;
         SDB_ASSERT( pTask && CLS_TASK_CREATE_IDX == pTask->taskType(),
                     "pTask cann't be null or invalid task type" ) ;

         clsCreateIdxTask *crtTask = (clsCreateIdxTask*)pTask ;

         rc = crtTask->addGlobalOpt2Def( _globalIdxCLName,
                                         _globalIdxCLUniqID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add global option to index definition, rc: %d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_EXE, "_catCMDCreateIndex::_execute" )
   INT32 _catCMDCreateIndex::_execute( _pmdEDUCB *cb, rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_EXE ) ;

      if ( _isGlobal )
      {
         rc = _createGlobalIdxCL( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create global index collection[%s], rc: %d",
                      _globalIdxCLName, rc ) ;

         rc = _addGlobalInfo2Task() ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add global information to tasks, rc: %d",
                      rc ) ;
      }

      for( VEC_TASKS_IT it = _vecTasks.begin() ; it != _vecTasks.end() ; it++ )
      {
         clsIdxTask *pTask = *it ;

         // If it is empty main task, we should set task status to
         // finished and add index to SYSINDEXES.
         if ( pTask->isMainTask() && 0 == pTask->countSubTask() )
         {
            pTask->setFinish() ;
            rc = postDoit( pTask, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to post doit, rc: %d", rc ) ;
         }

         // add task
         rc = catAddTask( pTask->toBson(), cb,
                          sdbGetCatalogueCB()->majoritySize() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add task, rc: %d", rc ) ;
      }

      // make reply
      {
         UINT64 taskID = 0 ;
         clsIdxTask* pTask = _vecTasks.back() ;
         if ( pTask )
         {
            taskID = pTask->taskID() ;
         }

         rc = _makeReply( taskID, ctxBuf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _catCMDCreateIndex::_checkGlobalIndex( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR szCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      UINT16 indexType = 0 ;
      utilCLUniqueID clUID = UTIL_UNIQUEID_NULL ;
      utilCSUniqueID csUID = UTIL_UNIQUEID_NULL ;

      if ( !_isGlobal )
      {
         goto done ;
      }

      PD_CHECK( _ixmIndexCB::generateIndexType( _boIdx, indexType ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse indexType:index=%s,rc=%d",
                _boIdx.toString().c_str(), rc ) ;

      PD_CHECK( !IXM_EXTENT_HAS_TYPE(IXM_EXTENT_TYPE_TEXT, indexType ),
                SDB_INVALIDARG, error, PDERROR, "Global index can't support"
                " text index:indexType=%d,rc=%d", indexType, rc ) ;

      PD_CHECK( !_pCataSet->isMainCL(), SDB_INVALIDARG, error, PDERROR,
                "MainCL(%s) could not create global index:rc=%d",
                _pIndexName, rc ) ;

      PD_CHECK( _isUnique, SDB_INVALIDARG, error, PDERROR,
                "Global index must be unique:index=%s,rc=%d",
                _boIdx.toString().c_str(), rc ) ;

      PD_CHECK( _isEnforced, SDB_INVALIDARG, error, PDERROR,
                "Global index's enfored must be true:index=%s,rc=%d",
                _boIdx.toString().c_str(), rc ) ;

      clUID = _pCataSet->clUniqueID() ;
      PD_CHECK( UTIL_IS_VALID_CLUNIQUEID( clUID ), SDB_INVALIDARG, error,
                PDERROR, "Invalid collection unqiueID(%llu):cl=%s,rc=%d",
                clUID, _pIndexName, rc ) ;

      csUID = utilGetCSUniqueID( clUID ) ;

      ossSnprintf( _globalIdxCSName, sizeof( _globalIdxCSName ),
                   "%s%u", IXM_GLOBAL_CS_PREFIX, csUID ) ;

      // _idxName => idxUID after SEQUOIADBMAINSTREAM-5079
      ossSnprintf( _globalIdxCLName, sizeof( _globalIdxCLName ),
                   "%s.%s%llu_%s", _globalIdxCSName,
                   IXM_GLOBAL_CL_PREFIX, clUID, _pIndexName ) ;

      rc = rtnResolveCollectionSpaceName( _pCollection,
                                          ossStrlen( _pCollection ), szCSName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get cs name:cl=%s,rc=%d",
                   _pCollection, rc ) ;

      rc = catGetCSDomain( szCSName, cb, _domainName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get cs domain:cs=%s,rc=%d",
                   szCSName, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_BMCLTASK, "_catCMDCreateIndex::_buildMainCLTask" )
   INT32 _catCMDCreateIndex::_buildMainCLTask( _pmdEDUCB *cb,
                                               BOOLEAN mainCLRedef )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_BMCLTASK ) ;

      UINT64 mainTaskID = CLS_INVALID_TASKID ;
      clsCreateIdxTask* pMainTask = NULL ;
      ossPoolVector<UINT64> subTaskList ;
      CLS_SUBCL_LIST subclList ;

      /// 1. new main task
      mainTaskID = sdbGetCatalogueCB()->getCatlogueMgr()->assignTaskID() ;

      pMainTask = SDB_OSS_NEW clsCreateIdxTask( mainTaskID ) ;
      PD_CHECK( pMainTask, SDB_OOM, error, PDERROR, "malloc failed" ) ;

      /// 2. build subcl's task
      rc = _pCataSet->getSubCLList( subclList );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get sub-collection list of collection[%s], "
                   "rc: %d", _pCataSet->name(), rc ) ;

      for ( CLS_SUBCL_LIST_IT it = subclList.begin() ;
            it != subclList.end() ; ++it )
      {
         const CHAR* subclName = (*it).c_str() ;
         UINT64 subTaskID = CLS_INVALID_TASKID ;

         rc = catCheckIndexExist( subclName, _pIndexName, _boIdx, cb ) ;
         if ( SDB_IXM_REDEF == rc )
         {
            // if the subcl's index already exists, DON'T build the subcl's task
            rc = SDB_OK ;
            PD_LOG( PDWARNING, "Collection[%s] index[%s] already exists",
                    subclName, _pIndexName ) ;
            continue ;
         }
         if ( rc )
         {
            goto error ;
         }

         rc = _buildCLTask( cb, subclName, mainTaskID, &subTaskID ) ;
         if ( rc )
         {
            goto error ;
         }

         try
         {
            subTaskList.push_back( subTaskID ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

      /// 3. If all sub-collection throw SDB_IXM_REDEF, then main-task has no
      ///    sub-task. If main-collection also throw SDB_IXM_REDEF, then just
      ///    throw SDB_IXM_REDEF.
      if ( 0 == subTaskList.size() && mainCLRedef )
      {
         rc = SDB_IXM_REDEF ;
         PD_LOG( PDERROR, "An index with the same definition[%s] and "
                 "name[%s] already exists in collection[%s]",
                 _boIdx.toString().c_str(), _pIndexName, _pCollection ) ;
         goto error ;
      }

      /// 4. build main task
      rc = pMainTask->initMainTask( _pCollection, _boIdx,
                                    _groupSet, subTaskList ) ;
      if ( rc )
      {
         goto error ;
      }

      try
      {
         _vecTasks.push_back( pMainTask ) ;
         pMainTask = NULL ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_BMCLTASK, rc ) ;
      return rc ;
   error:
      if ( pMainTask )
      {
         SDB_OSS_DEL pMainTask ;
         pMainTask = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_BCLTASK, "_catCMDCreateIndex::_buildCLTask" )
   INT32 _catCMDCreateIndex::_buildCLTask( _pmdEDUCB *cb,
                                           const CHAR* collectionName,
                                           UINT64 mainTaskID,
                                           UINT64* pTaskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_BCLTASK ) ;

      clsCreateIdxTask* pTask = NULL ;
      BSONObj boCollection ;
      vector<UINT32> groupIDList ;
      vector<string> groupNameList ;
      UINT64 taskID = sdbGetCatalogueCB()->getCatlogueMgr()->assignTaskID() ;

      // new task
      pTask = SDB_OSS_NEW clsCreateIdxTask( taskID ) ;
      PD_CHECK( pTask, SDB_OOM, error, PDERROR, "malloc failed" ) ;

      // get group info
      rc = catGetCollection( collectionName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the collection[%s], rc: %d",
                   collectionName, rc ) ;

      rc = catGetCollectionGroups( boCollection, groupIDList, groupNameList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to collect groups for collection [%s], rc: %d",
                   collectionName, rc ) ;

      for( vector<string>::const_iterator it = groupNameList.begin() ;
           it != groupNameList.end() ; it++ )
      {
         try
         {
            _groupSet.insert( it->c_str() ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

      // build task
      rc = pTask->initTask( collectionName, _boIdx, groupNameList,
                            _sortBufSz, mainTaskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Initialize index task failed, rc: %d",
                   rc );

      try
      {
         _vecTasks.push_back( pTask ) ;
         pTask = NULL ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      if ( pTaskID )
      {
         *pTaskID = taskID ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_BCLTASK, rc ) ;
      return rc ;
   error:
      if ( pTask )
      {
         SDB_OSS_DEL pTask ;
         pTask = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_CHKUKEY, "_catCMDCreateIndex::_checkUniqueKey" )
   INT32 _catCMDCreateIndex::_checkUniqueKey( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_CHKUKEY ) ;

      BSONObj boCollection ;
      std::set<UINT32> checkedKeyIDs ;
      CLS_SUBCL_LIST_IT it ;
      CLS_SUBCL_LIST subCLLst ;

      if ( dmsIsSysIndexName( _pIndexName ) || _isGlobal )
      {
         goto done ;
      }

      // if this is an unique index
      if ( !_isUnique )
      {
         goto done ;
      }

      // if this collection has sharding key
      if ( !_pCataSet->isSharding() )
      {
         goto done ;
      }

      // unique index should include sharding key of this collection
      rc = _checkUniqueKey( *_pCataSet, checkedKeyIDs ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check index key[%s] on collection[%s], rc: %d",
                   _boIdx.toString().c_str(), _pCollection, rc ) ;

      // if this collection is main-cl, should also check every sub-cl
      if ( !_pCataSet->isMainCL() )
      {
         goto done ;
      }

      rc = _pCataSet->getSubCLList( subCLLst );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get sub-collection list of collection[%s], "
                   "rc: %d", _pCollection, rc ) ;

      for ( it = subCLLst.begin() ; it != subCLLst.end() ; ++it )
      {
         const CHAR* subCL = (*it).c_str() ;
         clsCatalogSet* subCataSet = NULL ;

         rc = catNewCataset( subCL, cb, subCataSet ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get collection[%s]'s cata info, rc: %d",
                      subCL, rc ) ;

         rc = _checkUniqueKey( *subCataSet, checkedKeyIDs ) ;
         if ( rc )
         {
            catReleaseCataset( subCataSet ) ;
            PD_LOG( PDERROR,
                    "Failed to check index key[%s] on collection[%s], rc: %d",
                    _boIdx.toString().c_str(), subCL, rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_CHKUKEY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTIDX_CHKUKEY2, "_catCMDCreateIndex::_checkUniqueKey" )
   INT32 _catCMDCreateIndex::_checkUniqueKey( const clsCatalogSet &cataSet,
                                              std::set<UINT32> &checkedKeyIDs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDCRTIDX_CHKUKEY2 ) ;

      try
      {
         UINT32 skSiteID = cataSet.getShardingKeySiteID() ;
         if ( skSiteID > 0 )
         {
            if ( checkedKeyIDs.count( skSiteID ) > 0 )
            {
               // already checked
               goto done ;
            }
            checkedKeyIDs.insert( skSiteID ) ;
         }

         // check the sharding key
         const BSONObj &shardingKey = cataSet.getShardingKey() ;
         const BSONObj &boKey = _boIdx.getObjectField( IXM_KEY_FIELD ) ;

         BSONObjIterator shardingItr ( shardingKey ) ;
         while ( shardingItr.more() )
         {
            BSONElement sk = shardingItr.next() ;
            if ( boKey.getField( sk.fieldName() ).eoo() )
            {
               PD_LOG( PDERROR, "All fields in sharding key must "
                       "be included in unique index, missing field: %s,"
                       "shardingKey: %s, indexKey: %s, collection: %s",
                       sk.fieldName(), shardingKey.toString().c_str(),
                       boKey.toString().c_str(), cataSet.name() ) ;
               rc = SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDWARNING, "Exception occurred: %s", e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATCMDCRTIDX_CHKUKEY2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      catCMDDropIndex implement
   */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER(_catCMDDropIndex)
   _catCMDDropIndex::_catCMDDropIndex( BOOLEAN sysCall )
   : _catCMDIndexHelper( sysCall )
   {}

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPIDX_INIT, "_catCMDDropIndex::init" )
   INT32 _catCMDDropIndex::init( const CHAR *pQuery,
                                 const CHAR *pSelector,
                                 const CHAR *pOrderBy,
                                 const CHAR *pHint,
                                 INT32 flags,
                                 INT64 numToSkip,
                                 INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDDROPIDX_INIT ) ;

      // message formate:
      // macher: { Collection: "foo.bar", Index: { "": "aIdx" }, Async: true }
      // hint:   { Prepare: true }

      try
      {
         BSONObj query, hint, idxObj ;
         if ( pQuery )
         {
            query = BSONObj( pQuery ) ;
         }
         if ( pHint )
         {
            hint = BSONObj( pHint ) ;
         }

         // get collection name
         rc = rtnGetStringElement( query, CAT_COLLECTION, &_pCollection ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CAT_COLLECTION, rc ) ;

         // get index definition
         rc = rtnGetObjElement( query, FIELD_NAME_INDEX, idxObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_INDEX, rc ) ;

         // get index name
         BSONElement ele = idxObj.firstElement() ;
         PD_CHECK( String == ele.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Invalid index obj type: %s", idxObj.toString().c_str() ) ;
         _pIndexName = ele.valuestr() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPIDX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPIDX_DOIT, "_catCMDDropIndex::doit" )
   INT32 _catCMDDropIndex::doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf,
                                 INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDDROPIDX_DOIT ) ;

      rc = _check( cb ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _execute( cb, ctxBuf ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPIDX_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPIDX_PDOIT, "_catCMDDropIndex::postDoit" )
   INT32 _catCMDDropIndex::postDoit( const clsTask *pTask, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDDROPIDX_PDOIT ) ;

      SDB_ASSERT( pTask, "pTask cann't be null" ) ;
      SDB_ASSERT( CLS_TASK_DROP_IDX == pTask->taskType(),
                  "Invalid task type" ) ;
      SDB_ASSERT( CLS_TASK_STATUS_FINISH == pTask->status(),
                  "Task status should be finished" ) ;

      clsDropIdxTask *dropTask = (clsDropIdxTask*)pTask ;
      const CHAR* collection = dropTask->collectionName() ;
      const CHAR* index = dropTask->indexName() ;

      if ( SDB_OK == dropTask->resultCode() ||
           SDB_IXM_NOTEXIST == dropTask->resultCode() )
      {
         BOOLEAN isGlobalIndex = FALSE ;
         string indexCLName ;
         utilCLUniqueID indexCLUID = UTIL_UNIQUEID_NULL ;
         BOOLEAN removeOldIdx = FALSE ;
         INT16 w = sdbGetCatalogueCB()->majoritySize() ;

         rc = catGetGlobalIndexInfo( collection, index, cb,
                                     isGlobalIndex, indexCLName, indexCLUID ) ;
         if ( SDB_IXM_NOTEXIST == rc )
         {
            // main-collection may hasn't this index, while sub-collection
            // has this index. So main-task will return -47 error.
            rc = SDB_OK ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get global index[%s:%s], rc: %d",
                      collection, index, rc ) ;

         if ( isGlobalIndex )
         {
            (void)_dropGlobalIdxCL( indexCLName.c_str(), cb ) ;
         }

         rc = catRemoveIndex( collection, index, cb, w, &removeOldIdx ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to remove index, rc: %d",
                      rc ) ;

         if ( SDB_IXM_NOTEXIST == dropTask->resultCode() &&
              removeOldIdx )
         {
            rc = catUpdateTask2Finish( dropTask->taskID(), SDB_OK, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update task, rc: %d",
                         rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPIDX_PDOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPIDX_CHK, "_catCMDDropIndex::_check" )
   INT32 _catCMDDropIndex::_check( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDDROPIDX_CHK ) ;

      // get catalog set first
      rc = catNewCataset( _pCollection, cb, _pCataSet ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get collection[%s]'s cata info, rc: %d",
                   _pCollection, rc ) ;

      // cannot drop system index
      if ( !_sysCall && 0 == ossStrcmp ( _pIndexName, IXM_ID_KEY_NAME ) )
      {
         PD_LOG_MSG ( PDERROR,
                      "Cannot drop $id index, use dropIdIndex() instead" ) ;
         rc = SDB_IXM_DROP_ID ;
         goto error ;
      }
      if ( !_sysCall && 0 == ossStrcmp ( _pIndexName, IXM_SHARD_KEY_NAME ) )
      {
         PD_LOG_MSG ( PDERROR, "Cannot drop $shard index, "
                      "use enableSharding()/disableSharding() instead" ) ;
         rc = SDB_IXM_DROP_SHARD ;
         goto error ;
      }

      // check index exist, and build tasks
      if ( _pCataSet->isMainCL() )
      {
         BOOLEAN idxNotExist = FALSE ;

         rc = _checkIndexExist( _pCollection, _pIndexName, cb ) ;
         if ( SDB_IXM_NOTEXIST == rc )
         {
            // ignore error, because sub-collection may have this index
            rc = SDB_OK ;
            idxNotExist = TRUE ;
         }
         if ( rc )
         {
            goto error ;
         }

         rc = _buildMainCLTask( cb, idxNotExist ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = _checkIndexExist( _pCollection, _pIndexName, cb ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _buildCLTask( cb, _pCollection ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      // check task conflict
      rc = _checkTaskConflict( cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPIDX_CHK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPIDX_EXE, "_catCMDDropIndex::_execute" )
   INT32 _catCMDDropIndex::_execute( _pmdEDUCB *cb, rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDDROPIDX_EXE ) ;

      for( VEC_TASKS_IT it = _vecTasks.begin() ; it != _vecTasks.end() ; it++ )
      {
         clsIdxTask* pTask = *it ;

         // If it is empty main task, we should set task status to
         // finished and remove index from SYSINDEXES.
         if ( pTask->isMainTask() && 0 == pTask->countSubTask() )
         {
            pTask->setFinish() ;
            rc = postDoit( pTask, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to post doit, rc: %d", rc ) ;
         }

         // add task
         rc = catAddTask( pTask->toBson(), cb,
                          sdbGetCatalogueCB()->majoritySize() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add task, rc: %d",
                      rc ) ;
      }

      // make reply
      {
         UINT64 taskID = 0 ;
         clsIdxTask* pTask = _vecTasks.back() ;
         if ( pTask )
         {
            taskID = pTask->taskID() ;
         }

         rc = _makeReply( taskID, ctxBuf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPIDX_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _catCMDDropIndex::_checkIndexExist( const CHAR* collection,
                                             const CHAR* indexName,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      rc = catGetIndex( collection, indexName, cb, obj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR,
                 "Failed to get collection[%s]'s index[%s], rc: %d",
                 collection, indexName, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPIDX_BMTASK, "_catCMDDropIndex::_buildMainCLTask" )
   INT32 _catCMDDropIndex::_buildMainCLTask( _pmdEDUCB *cb,
                                             BOOLEAN mainCLIdxNotExist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDDROPIDX_BMTASK ) ;

      UINT64 mainTaskID = CLS_INVALID_TASKID ;
      clsDropIdxTask* pMainTask = NULL ;
      ossPoolVector<UINT64> subTaskList ;
      CLS_SUBCL_LIST subclList ;

      /// 1. new main task
      mainTaskID = sdbGetCatalogueCB()->getCatlogueMgr()->assignTaskID() ;

      pMainTask = SDB_OSS_NEW clsDropIdxTask( mainTaskID ) ;
      PD_CHECK( pMainTask, SDB_OOM, error, PDERROR, "malloc failed" ) ;

      /// 2. build subcl's task
      rc = _pCataSet->getSubCLList( subclList );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get sub-collection list of collection[%s], "
                   "rc: %d", _pCataSet->name(), rc ) ;

      for ( CLS_SUBCL_LIST_IT it = subclList.begin() ;
            it != subclList.end() ; ++it )
      {
         const CHAR* subclName = (*it).c_str() ;
         UINT64 subTaskID = CLS_INVALID_TASKID ;

         rc = _checkIndexExist( subclName, _pIndexName, cb ) ;
         if ( SDB_IXM_NOTEXIST == rc )
         {
            // if the subcl's index already exists, DON'T build the subcl's task
            rc = SDB_OK ;
            PD_LOG( PDWARNING, "Collection[%s] index[%s] doesn't exists",
                    subclName, _pIndexName ) ;
            continue ;
         }
         if ( rc )
         {
            goto error ;
         }

         rc = _buildCLTask( cb, subclName, mainTaskID, &subTaskID ) ;
         if ( rc )
         {
            goto error ;
         }

         try
         {
            subTaskList.push_back( subTaskID ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

      /// 3. If all sub-collection throw SDB_IXM_NOTEXIST, then main-task has no
      ///    sub-task. If main-collection also throw SDB_IXM_NOTEXIST, then just
      ///    throw SDB_IXM_NOTEXIST.
      if ( 0 == subTaskList.size() && mainCLIdxNotExist )
      {
         rc = SDB_IXM_NOTEXIST ;
         PD_LOG( PDERROR, "Index does't exists: [%s:%s]",
                 _pCollection, _pIndexName ) ;
         goto error ;
      }

      /// 4. build main task
      rc = pMainTask->initMainTask( _pCollection, _pIndexName,
                                    _groupSet, subTaskList ) ;
      if ( rc )
      {
         goto error ;
      }

      try
      {
         _vecTasks.push_back( pMainTask ) ;
         pMainTask = NULL ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPIDX_BMTASK, rc ) ;
      return rc ;
   error:
      if ( pMainTask )
      {
         SDB_OSS_DEL pMainTask ;
         pMainTask = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPIDX_BCLTASK, "_catCMDDropIndex::_buildCLTask" )
   INT32 _catCMDDropIndex::_buildCLTask( _pmdEDUCB *cb,
                                         const CHAR* collectionName,
                                         UINT64 mainTaskID,
                                         UINT64* pTaskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDDROPIDX_BCLTASK ) ;

      clsDropIdxTask* pTask = NULL ;
      BSONObj boCollection ;
      vector<UINT32> groupIDList ;
      vector<string> groupNameList ;
      UINT64 taskID = sdbGetCatalogueCB()->getCatlogueMgr()->assignTaskID() ;

      // new task
      pTask = SDB_OSS_NEW clsDropIdxTask( taskID ) ;
      PD_CHECK( pTask, SDB_OOM, error, PDERROR, "malloc failed" ) ;

      // get group info
      rc = catGetCollection( collectionName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the collection[%s], rc: %d",
                   collectionName, rc ) ;

      rc = catGetCollectionGroups( boCollection, groupIDList, groupNameList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to collect groups for collection [%s], "
                   "rc: %d", collectionName, rc ) ;

      for( vector<string>::const_iterator it = groupNameList.begin() ;
           it != groupNameList.end() ; it++ )
      {
         try
         {
            _groupSet.insert( it->c_str() ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

      // build task
      rc = pTask->initTask( collectionName, _pIndexName,
                            groupNameList, mainTaskID ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize index task failed, rc: %d", rc );

      try
      {
         _vecTasks.push_back( pTask ) ;
         pTask = NULL ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      if ( pTaskID )
      {
         *pTaskID = taskID ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPIDX_BCLTASK, rc ) ;
      return rc ;
   error:
      if ( pTask )
      {
         SDB_OSS_DEL pTask ;
         pTask = NULL ;
      }
      goto done ;
   }

   /*
      catCMDReportTaskProgress implement
   */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER(_catCMDReportTaskProgress)
   INT32 _catCMDReportTaskProgress::init( const CHAR *pQuery,
                                          const CHAR *pSelector,
                                          const CHAR *pOrderBy,
                                          const CHAR *pHint,
                                          INT32 flags,
                                          INT64 numToSkip,
                                          INT64 numToReturn )
   {
      if ( pQuery )
      {
         _query = BSONObj( pQuery ) ;
      }
      return SDB_OK ;
   }

   INT32 _catCMDReportTaskProgress::doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf,
                                          INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      CLS_TASK_STATUS status = CLS_TASK_STATUS_END ;

      rc = rtnGetNumberLongElement( _query, FIELD_NAME_TASKID,
                                    (INT64&)taskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   FIELD_NAME_TASKID, _query.toString().c_str(), rc ) ;

      PD_LOG( PDDEBUG, "Received task[%llu]'s progresss report[%s]",
              taskID, _query.toString().c_str() ) ;

      rc = _updateTaskProgress( taskID, cb, status ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update task[%llu] progress, rc: %d",
                   taskID, rc ) ;

      ctxBuf = rtnContextBuf( BSON( FIELD_NAME_STATUS << status ) ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDREPTTASK_UPDTASKP, "_catCMDReportTaskProgress::_updateTaskProgress" )
   INT32 _catCMDReportTaskProgress::_updateTaskProgress( UINT64 taskID,
                                                         _pmdEDUCB *cb,
                                                         CLS_TASK_STATUS &status )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDREPTTASK_UPDTASKP ) ;

      clsTask *pTask = NULL ;
      catCMDBase *pCommand = NULL ;

      // 1. get task object from SYSTASKS
      BSONObj taskObj ;
      rc = catGetTask( taskID, taskObj, cb ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to get task[%llu], rc: %d",
                    taskID, rc ) ;

      // 2. new and init clsTask
      rc = clsNewTask( taskObj, pTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to new task, rc: %d",
                   rc ) ;

      // 3. update task progress
      if ( CLS_TASK_STATUS_FINISH == pTask->status() )
      {
         PD_LOG( PDDEBUG, "Task[%llu] is already finished", taskID ) ;
         goto done ;
      }
      if ( pTask->isMainTask() )
      {
         PD_LOG( PDWARNING, "No need to process main task[%llu]", taskID ) ;
         goto done ;
      }

      {
         BSONObj updator, matcher ;
         rc = pTask->updateTaskInfo( _query.objdata(), updator, matcher ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task[%llu] info by [%s], rc: %d",
                      taskID, _query.toString().c_str(), rc ) ;

         if ( updator.isEmpty() )
         {
            // nothing changed, just goto done
            goto done ;
         }

         rc = catUpdateTask( matcher, updator, cb, 1 ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to update task, rc: %d",
                       rc ) ;
      }

      // 4. if task finish, then we may update metadata
      if ( CLS_TASK_STATUS_FINISH == pTask->status() )
      {
         const CHAR* commandName = pTask->commandName() ;
         if ( commandName )
         {
            rc = getCatCmdBuilder()->create( commandName, pCommand ) ;
            PD_RC_CHECK ( rc, PDERROR,
                          "Failed to create command[%s], rc: %d",
                          commandName, rc ) ;

            rc = pCommand->postDoit( pTask, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to post doit for command[%s], rc: %d",
                         commandName, rc ) ;
         }
      }

      // 5.  process main-task
      if ( pTask->hasMainTask() )
      {
         rc = _updateMainTaskProgress( pTask, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update main-task[%llu] progress, rc: %d",
                      pTask->mainTaskID(), rc ) ;
      }

   done:
      if ( pTask )
      {
         status = pTask->status() ;
      }
      if ( pCommand )
      {
         getCatCmdBuilder()->release( pCommand ) ;
      }
      if ( pTask )
      {
         clsReleaseTask( pTask ) ;
      }
      PD_TRACE_EXITRC( SDB_CATCMDREPTTASK_UPDTASKP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDREPTTASK_UPDMTASKP, "_catCMDReportTaskProgress::_updateMainTaskProgress" )
   INT32 _catCMDReportTaskProgress::_updateMainTaskProgress( clsTask *pSubTask,
                                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCMDREPTTASK_UPDMTASKP ) ;

      clsTask *pMainTask = NULL ;
      UINT64 mainTaskID = pSubTask->mainTaskID() ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      catCMDBase *pCommand = NULL ;
      ossPoolVector<BSONObj> subTaskInfoList ;
      BSONObj updator, matcher1, matcher2, dummyObj, selector ;

      /// 1. get main-task object from SYSTASKS
      BSONObj taskObj ;
      rc = catGetTask( mainTaskID, taskObj, cb ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to get task[%llu], rc: %d",
                    mainTaskID, rc ) ;

      /// 2. new and init main-task
      rc = clsNewTask( taskObj, pMainTask ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to new task, rc: %d",
                   rc ) ;

      /// 3. update main-task progress
      if ( CLS_TASK_STATUS_FINISH == pMainTask->status() )
      {
         PD_LOG( PDDEBUG, "Task[%llu] is already finished", mainTaskID ) ;
         goto done ;
      }

      rc = pMainTask->querySubTasks( _query.objdata(), matcher1, selector ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get subtask's matcher and selector, rc: %d",
                   rc ) ;

      // query sub-task info
      if ( !matcher1.isEmpty() )
      {
         rc = rtnQuery( CAT_TASK_INFO_COLLECTION, selector, matcher1,
                        dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                        contextID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Query collection[%s] failed: matcher=%s, rc=%d",
                      CAT_TASK_INFO_COLLECTION, matcher1.toString().c_str(),
                      rc ) ;
      }

      // get more
      while ( TRUE )
      {
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            subTaskInfoList.push_back( BSONObj( contextBuf.data() ).getOwned() ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

      // update
      rc = pMainTask->updateMainTaskInfo( pSubTask, subTaskInfoList,
                                          updator, matcher2 ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update main task[%llu] info, rc: %d",
                   mainTaskID, rc ) ;

      if ( !updator.isEmpty() )
      {
         rc = catUpdateTask( matcher2, updator, cb, 1 ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to update task, rc: %d",
                       rc ) ;
      }

      /// 4. if task finish, then we may update metadata
      if ( CLS_TASK_STATUS_FINISH == pMainTask->status() )
      {
         const CHAR* commandName = pMainTask->commandName() ;
         if ( commandName )
         {
            rc = getCatCmdBuilder()->create( commandName, pCommand ) ;
            PD_RC_CHECK ( rc, PDERROR,
                          "Failed to create command[%s], rc: %d",
                          commandName, rc ) ;

            rc = pCommand->postDoit( pMainTask, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to post doit for command[%s], rc: %d",
                         commandName, rc ) ;
         }
      }

   done:
      if ( pCommand )
      {
         getCatCmdBuilder()->release( pCommand ) ;
      }
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      if ( pMainTask )
      {
         clsReleaseTask( pMainTask ) ;
      }
      PD_TRACE_EXITRC( SDB_CATCMDREPTTASK_UPDMTASKP, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

