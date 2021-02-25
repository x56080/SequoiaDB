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
#include "catCommand.hpp"
#include "catLevelLock.hpp"
#include "catCommon.hpp"
#include "rtn.hpp"
#include "catTrace.hpp"

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
                         "Type of field[%s] is not boolean, rc: %d",
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
         else if ( 0 != ossStrcmp( _dsInfo._type, VALUE_NAME_SEQUOIADB ) )
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

   const CHAR* _catCMDCreateDataSource::name()
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

   const CHAR* _catCMDDropDataSource::name()
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

   const CHAR* _catCMDAlterDataSource::name()
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
      BOOLEAN exist = FALSE ;

      if ( 0 == ossStrcmp( _name, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         goto done ;
      }

      try
      {
         BSONObj record ;
         rc = catCheckCollectionExist( _name, exist, record, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check collection[%s] existence failed[%d]",
                      _name, rc ) ;
         if ( !exist )
         {
            // Need to check if it's in pure mapping cs.
            BSONObj dummyObj ;
            BSONObj csMetaRecord ;
            BSONObj matcher ;
            CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
            const CHAR *dot = ossStrchr( _name, '.' ) ;
            SDB_ASSERT( dot, "The name is not a full name" ) ;
            ossStrncpy( csName, _name, dot - _name ) ;

            matcher = BSON( FIELD_NAME_NAME << csName ) ;
            rc = catGetOneObj( CAT_COLLECTION_SPACE_COLLECTION, dummyObj,
                               matcher, dummyObj, cb, csMetaRecord ) ;
            if ( SDB_DMS_EOC == rc )
            {
               // For compatible reason, return SDB_DMS_NOTEXIST instead of
               // SDB_DMS_CS_NOTEXIST.
               rc = SDB_DMS_NOTEXIST ;
               goto error ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Get collection space[%s] metadata from "
                                "SYSCOLLECTIONSPACES failed[%d]", csName, rc ) ;
               goto error ;
            }
            if ( !csMetaRecord.hasField( FIELD_NAME_DATASOURCE_ID ) )
            {
               rc = SDB_DMS_NOTEXIST ;
            }
            goto done ;
         }
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

   const CHAR *_catCMDTestCollection::name()
   {
      return CMD_NAME_TEST_COLLECTION ;
   }
}
