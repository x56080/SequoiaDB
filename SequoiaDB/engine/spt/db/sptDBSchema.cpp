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

   Source File Name = sptDBSchema.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBSchema.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

#define SPT_schema_NAME         "SdbSchema"

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBSchema, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBSchema, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBSchema, addColumn )
   JS_MEMBER_FUNC_DEFINE( _sptDBSchema, alterColumn )
   JS_MEMBER_FUNC_DEFINE( _sptDBSchema, renameColumn )
   JS_MEMBER_FUNC_DEFINE( _sptDBSchema, dropColumn )
   JS_MEMBER_FUNC_DEFINE( _sptDBSchema, dropColumnDefault )
   JS_MEMBER_FUNC_DEFINE( _sptDBSchema, setAttributes )
   JS_MEMBER_FUNC_DEFINE( _sptDBSchema, alter )

   JS_BEGIN_MAPPING( _sptDBSchema, SPT_schema_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "addColumn", addColumn )
      JS_ADD_MEMBER_FUNC( "alterColumn", alterColumn )
      JS_ADD_MEMBER_FUNC( "renameColumn", renameColumn )
      JS_ADD_MEMBER_FUNC( "dropColumn", dropColumn )
      JS_ADD_MEMBER_FUNC( "dropColumnDefault", dropColumnDefault )
      JS_ADD_MEMBER_FUNC( "setAttributes", setAttributes )
      JS_ADD_MEMBER_FUNC( "alter", alter )
   JS_MAPPING_END()

   _sptDBSchema::_sptDBSchema( _sdbSchema *pSchema )
   {
      _schema.pSchema = pSchema ;
   }

   _sptDBSchema::~_sptDBSchema()
   {
   }

   INT32 _sptDBSchema::construct( const _sptArguments &arg,
                                  _sptReturnVal &val,
                                  BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "use of new SdbSchema() is forbidden, you should use other "
                                "functions to produce a SdbSchema object") ;
      return SDB_SYS ;
   }

   INT32 _sptDBSchema::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBSchema::addColumn( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      string columnName ;
      BSONObj columnDef ;

      rc = arg.getString( 0, columnName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Column name should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Column name should be a string" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 1, columnDef ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Column definition should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Column definition should be an object" ) ;
         goto error ;
      }

      rc = _schema.addColumn( columnName.c_str(), columnDef ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to add column to informational schema" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSchema::alterColumn( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      string columnName ;
      BSONObj columnOptions ;

      rc = arg.getString( 0, columnName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Column name should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Column name should be a string" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 1, columnOptions ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Column options should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Column options should be an object" ) ;
         goto error ;
      }

      rc = _schema.alterColumn( columnName.c_str(), columnOptions ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to alter column to informational schema" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSchema::renameColumn( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      string columnName ;
      string newColumnName ;

      rc = arg.getString( 0, columnName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Column name should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Column name should be a string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, newColumnName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "New column name should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "New column name should be a string" ) ;
         goto error ;
      }

      rc = _schema.renameColumn( columnName.c_str(), newColumnName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to rename column to informational schema" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSchema::dropColumn( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      string columnName ;

      rc = arg.getString( 0, columnName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Column name should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Column name should be a string" ) ;
         goto error ;
      }

      rc = _schema.dropColumn( columnName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop column to informational schema" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSchema::dropColumnDefault( const _sptArguments &arg,
                                          _sptReturnVal &rval,
                                          BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      string columnName ;

      rc = arg.getString( 0, columnName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Column name should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Column name should be a string" ) ;
         goto error ;
      }

      rc = _schema.dropColumnDefault( columnName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop default to informational schema" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSchema::setAttributes( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options should be an object" ) ;
         goto error ;
      }

      rc = _schema.setAttributes( options ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set attributes to informational schema" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBSchema::alter( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options should be specified" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options should be an object" ) ;
         goto error ;
      }

      rc = _schema.alter( options ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to alter informational schema" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
