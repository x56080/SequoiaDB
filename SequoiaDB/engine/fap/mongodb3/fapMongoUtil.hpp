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

   Source File Name = fapMongoUtil.hpp

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
#ifndef _SDB_MSG_BUFFER_HPP_
#define _SDB_MSG_BUFFER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "../../bson/bson.hpp"
#include "utilCommon.hpp"
#include "fapMongoMessageDef.hpp"
#include "fapMongoMessage.hpp"

#define MEMERY_BLOCK_SIZE 4096

namespace fap
{

class _mongoMsgBuffer : public engine::_utilPooledObject
{
public:
   _mongoMsgBuffer() : _pData( NULL ), _size( 0 ), _capacity( 0 )
   {
      _alloc( MEMERY_BLOCK_SIZE ) ;
   }

   ~_mongoMsgBuffer()
   {
      if ( NULL != _pData )
      {
         SDB_OSS_FREE( _pData ) ;
         _pData = NULL ;
      }
   }

   BOOLEAN empty() const
   {
      return 0 == _size ;
   }

   INT32 write( const CHAR *pIn, const UINT32 inLen,
                BOOLEAN align = FALSE, INT32 bytes = 4 ) ;

   INT32 write( const BSONObj &obj,
                BOOLEAN align = FALSE, INT32 bytes = 4 ) ;

   INT32 advance( const UINT32 pos ) ;

   void zero()
   {
      ossMemset( _pData, 0, _capacity ) ;
      _size = 0 ;
   }

   CHAR *data() const
   {
      return _pData ;
   }

   const UINT32 size() const
   {
      return _size ;
   }

   const UINT32 capacity() const
   {
      return _capacity ;
   }

   void reserve( const UINT32 size )
   {
      if ( size < _capacity )
      {
         return ;
      }

      _realloc( size ) ;
   }

   void doneLen()
   {
      *(SINT32 *)_pData = _size ;
   }

private:
   INT32 _alloc( const UINT32 size ) ;
   INT32 _realloc( const UINT32 size ) ;

private:
   CHAR  *_pData ;
   UINT32 _size ;
   UINT32 _capacity ;
} ;
typedef _mongoMsgBuffer mongoMsgBuffer ;

class _mongoErrorObjAssit : public SDBObject
{
public:
   _mongoErrorObjAssit() ;

   ~_mongoErrorObjAssit(){}

   BSONObj getErrorObj( INT32 errorCode )
   {
      return _errorObjsArray[ SDB_MAX_ERROR + errorCode ] ;
   }

private:
   BSONObj _errorObjsArray[ SDB_MAX_ERROR + SDB_MAX_WARNING + 1 ] ;
};
typedef _mongoErrorObjAssit mongoErrorObjAssit ;

INT32 mongoGenerateNewRecord( const BSONObj &matcher,
                              const BSONObj &updatorObj,
                              BSONObj &target ) ;

BSONObj mongoGetErrorBson( INT32 errorCode ) ;

CHAR* mongoGetOOMErrResHeader() ;

BOOLEAN mongoCheckBigEndian() ;

INT32 mongoGetIntElement( const BSONObj &obj, const CHAR *pFieldName,
                          INT32 &value ) ;

INT32 mongoGetStringElement ( const BSONObj &obj, const CHAR *pFieldName,
                              const CHAR *&pValue ) ;

INT32 mongoGetArrayElement ( const BSONObj &obj, const CHAR *pFieldName,
                             BSONObj &value ) ;

INT32 mongoGetNumberLongElement ( const BSONObj &obj,
                                  const CHAR *pFieldName,
                                  INT64 &value ) ;

}
#endif
