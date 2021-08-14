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

   Source File Name = fapMongoUtil.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ========================================
          11/04/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoUtil.hpp"
#include "mthMatchTree.hpp"
#include "mthModifier.hpp"
#include "fapMongoMessage.hpp"

using namespace bson ;

namespace fap
{

static _mongoErrorObjAssit errorObjAssit ;

INT32 _mongoMsgBuffer::_alloc( const UINT32 size )
{
   INT32 rc = SDB_OK ;

   if( 0 ==  size )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   _pData = ( CHAR *) SDB_OSS_MALLOC( size ) ;
   if ( NULL == _pData )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   _capacity = size ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoMsgBuffer::_realloc( const UINT32 size )
{
   INT32 rc = SDB_OK ;
   CHAR* ptr = NULL ;
   if ( 0 == size )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( size <= _capacity )
   {
      // do nothing
      goto done ;
   }

   ptr = ( CHAR * )SDB_OSS_REALLOC( _pData, size ) ;
   if ( NULL == ptr )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   _pData = ptr ;
   _capacity = size ;
   ossMemset( _pData + _size, 0, _capacity - _size ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoMsgBuffer::write( const CHAR *pIn, const UINT32 inLen,
                              BOOLEAN align , INT32 bytes )
{
   INT32 rc   = SDB_OK ;
   INT32 size = 0 ;        // new size to realloc
   INT32 num  = 0 ;        // number of memory block

   if ( NULL == pIn )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( inLen > _capacity - _size )
   {
      // digit size of memory needed
      num = ( inLen + _size ) / MEMERY_BLOCK_SIZE + 1 ;
      size = num * MEMERY_BLOCK_SIZE ;

      rc = _realloc( size ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   }

   ossMemcpy( _pData + _size, pIn, inLen ) ;
   if ( align )
   {
      _size += ossRoundUpToMultipleX( inLen, bytes );
   }
   else
   {
      _size += inLen ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoMsgBuffer::write( const BSONObj &obj, BOOLEAN align, INT32 bytes )
{
   INT32 rc   = SDB_OK ;
   INT32 size = 0 ;        // new size to realloc
   INT32 num  = 0 ;        // number of memory block
   UINT32 objsize = obj.objsize() ;
   if( objsize > _capacity - _size )
   {
      // digit size of memory needed
      num = ( objsize + _size ) / MEMERY_BLOCK_SIZE + 1 ;
      size = num * MEMERY_BLOCK_SIZE ;

      rc = _realloc( size ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   }

   ossMemcpy( _pData + _size, obj.objdata(), objsize ) ;
   if ( align )
   {
      _size += ossRoundUpToMultipleX( objsize, bytes ) ;
   }
   else
   {
      _size += objsize ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoMsgBuffer::advance( const UINT32 pos )
{
   INT32 rc = SDB_OK ;

   if ( pos > _capacity )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   _size = pos ;

done:
   return rc ;
error:
   goto done ;
}

_mongoErrorObjAssit::_mongoErrorObjAssit()
{
   for ( SINT32 i = -SDB_MAX_ERROR; i <= SDB_MAX_WARNING ; i ++ )
   {
      BSONObjBuilder berror ;
      berror.append ( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
      berror.append ( FAP_MONGO_FIELD_NAME_CODE, i ) ;
      berror.append ( FAP_MONGO_FIELD_NAME_ERRMSG, getErrDesp ( i ) ) ;
      _errorObjsArray[ i + SDB_MAX_ERROR ] = berror.obj() ;
   }
}

// generate a new record based on matcher condition and update condition
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GENERATENEWRECORD, "mongoGenerateNewRecord" )
INT32 mongoGenerateNewRecord( const BSONObj &matcher,
                              const BSONObj &updatorObj,
                              BSONObj &target )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_GENERATENEWRECORD ) ;
   INT32 rc = SDB_OK ;
   engine::mthMatchTree matcherTree ;
   engine::mthModifier modifier ;

   try
   {
      bson::BSONElement setOnInsert ;
      BSONObj source ;

      rc = matcherTree.loadPattern ( matcher ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to load matcher[%s], rc: %d",
                    matcher.toString().c_str(), rc ) ;

      source = matcherTree.getEqualityQueryObject( NULL ) ;

      rc = modifier.loadPattern( updatorObj, NULL, TRUE, NULL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load updator[%s], rc: %d",
                   updatorObj.toString().c_str(), rc ) ;

      rc = modifier.modify( source, target ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate new record, rc: %d",
                   rc ) ;

      // check if have oid
      if ( !target.hasElement( "_id" ) )
      {
         BSONObjBuilder builder ;
         builder.appendOID( "_id", NULL, TRUE ) ;
         builder.appendElements( target ) ;
         target = builder.obj() ;
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "Generate new record exception: %s, rc: %d",
              e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_GENERATENEWRECORD, rc ) ;
   return rc ;
error:
   goto done ;
}

BSONObj mongoGetErrorBson( INT32 errorCode )
{
   return errorObjAssit.getErrorObj( errorCode ) ;
}

CHAR* mongoGetOOMErrResHeader()
{
   static _fapMongoInnerHeader OOMResHeader( SDB_OOM ) ;
   return (CHAR*)&OOMResHeader ;
}

BOOLEAN mongoCheckBigEndian()
{
   BOOLEAN bigEndian = FALSE ;
   union
   {
      unsigned int i ;
      unsigned char s[4] ;
   } c ;

   c.i = 0x12345678 ;
   if ( 0x12 == c.s[0] )
   {
      bigEndian = TRUE ;
   }

   return bigEndian ;
}

INT32 mongoGetIntElement( const BSONObj &obj, const CHAR *pFieldName,
                          INT32 &value )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT ( pFieldName, "field name can't be NULL" ) ;
   BSONElement ele = obj.getField ( pFieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              pFieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be Integer",
              obj.toString().c_str()) ;
   value = ele.numberInt() ;
done :
   return rc ;
error :
   goto done ;
}

INT32 mongoGetStringElement ( const BSONObj &obj, const CHAR *pFieldName,
                              const CHAR *&pValue )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT ( pFieldName && &pValue, "field name and value can't be NULL" ) ;
   BSONElement ele = obj.getField ( pFieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              pFieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be String",
              obj.toString().c_str()) ;
   pValue = ele.valuestr() ;

done :
   return rc ;
error :
   goto done ;
}

INT32 mongoGetArrayElement ( const BSONObj &obj, const CHAR *pFieldName,
                             BSONObj &value )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT ( pFieldName , "field name can't be NULL" ) ;
   BSONElement ele = obj.getField ( pFieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              pFieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be Array",
              obj.toString().c_str()) ;
   value = ele.embeddedObject() ;
done :
   return rc ;
error :
   goto done ;
}

INT32 mongoGetNumberLongElement ( const BSONObj &obj, const CHAR *pFieldName,
                                  INT64 &value )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT ( pFieldName, "field name can't be NULL" ) ;
   BSONElement ele = obj.getField ( pFieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              pFieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be number",
              obj.toString().c_str()) ;
   value = ele.numberLong() ;
done :
   return rc ;
error :
   goto done ;
}

}

