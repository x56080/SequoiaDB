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

   Source File Name = omBusinessCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/23/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#include "omCommand.hpp"
#include "omDef.hpp"

using namespace bson;


namespace engine
{
   // ***************** omSetSettingsCommand ****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omSetSettingsCommand ) ;

   INT32 omSetSettingsCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      BSONObj settings ;
      omArgOptions option( _request ) ;
      omDatabaseTool dbTool( _cb ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "j", OM_REST_FIELD_SETTINGS, &settings ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to parse rest arg: rc=%d", rc ) ;
         goto error ;
      }

      if ( settings.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         _errorMsg.setError( TRUE, "invalid settings" ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = dbTool.setSettings( settings ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, "failed to set setting: rc=%d", rc ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      _recordHistory( rc, OM_SET_SETTINGS_REQ, settings ) ;
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   // ***************** omListSettingsCommand ****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omListSettingsCommand ) ;

   INT32 omListSettingsCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      list<BSONObj> settingList ;
      list<BSONObj>::iterator iter ;
      omDatabaseTool dbTool( _cb ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = dbTool.getSettingList( settingList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "failed to set setting: rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = settingList.begin(); iter != settingList.end(); ++iter )
      {
         BSONObj info = *iter ;

         rc = restTool.appendResponeContent( info ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to build respone: rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   // ***************** omGetHistoryNumberCommand ****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omGetHistoryNumberCommand ) ;

   INT32 omGetHistoryNumberCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      omArgOptions option( _request ) ;
      omDatabaseTool dbTool( _cb ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;
      BSONObj matcher ;
      INT64 count = 0 ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "|jj", FIELD_NAME_FILTER, &matcher,
                                       REST_KEY_NAME_MATCHER, &matcher ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      rc = dbTool.countHistory( matcher, count ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "Failed to get history number: rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      try
      {
         BSONObj info = BSON( OM_PUBLIC_FIELD_NUMBER << count ) ;

         rc = restTool.appendResponeContent( info ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to build respone: rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         _errorMsg.setError( TRUE, "failed to build respone: rc=%d", rc ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }

   // ***************** omQueryHistoryCommand ****************************
   IMPLEMENT_OMREST_CMD_AUTO_REGISTER( omQueryHistoryCommand ) ;

   INT32 omQueryHistoryCommand::doCommand()
   {
      INT32 rc = SDB_OK ;
      omArgOptions option( _request ) ;
      omDatabaseTool dbTool( _cb ) ;
      omRestTool restTool( _restSession->socket(), _restAdaptor, _response ) ;
      list<BSONObj> historyList ;
      list<BSONObj>::iterator iter ;
      BSONObj matcher ;
      BSONObj selector ;
      BSONObj order ;
      INT64 numSkip = 0 ;
      INT64 numReturn = -1 ;

      _setFileLanguageSep() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      rc = option.parseRestArg( "|jjjjjLL",
                                  FIELD_NAME_SELECTOR,   &selector,
                                  REST_KEY_NAME_MATCHER, &matcher,
                                  FIELD_NAME_FILTER,     &matcher,
                                  REST_KEY_NAME_ORDERBY, &order,
                                  FIELD_NAME_SORT,       &order,
                                  FIELD_NAME_SKIP,       &numSkip,
                                  REST_KEY_NAME_LIMIT,   &numReturn ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, option.getErrorMsg() ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      if ( 0 > numReturn || numReturn > 1000 )
      {
         numReturn = 1000 ;
      }

      rc = dbTool.queryHistory( matcher, selector, order, numSkip, numReturn,
                                historyList ) ;
      if ( rc )
      {
         _errorMsg.setError( TRUE, "Failed to query history: rc=%d", rc ) ;
         PD_LOG( PDERROR, _errorMsg.getError() ) ;
         goto error ;
      }

      for ( iter = historyList.begin(); iter != historyList.end(); ++iter )
      {
         BSONObj info = *iter ;

         rc = restTool.appendResponeContent( info ) ;
         if ( rc )
         {
            _errorMsg.setError( TRUE, "failed to build respone: rc=%d", rc ) ;
            PD_LOG( PDERROR, _errorMsg.getError() ) ;
            goto error ;
         }
      }

      restTool.sendOkRespone() ;

   done:
      return rc ;
   error:
      restTool.sendResponse( rc, _errorMsg.getError() ) ;
      goto done ;
   }
}

