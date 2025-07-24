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

   Source File Name = sptBsonobjArray.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptBsonobjArray.hpp"

using namespace bson ;

namespace engine
{

   /*
      _sptBsonobjArray implement
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptBsonobjArray, construct)
   JS_DESTRUCT_FUNC_DEFINE( _sptBsonobjArray, destruct)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, size)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, more)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, next)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, pos)
   JS_MEMBER_FUNC_DEFINE( _sptBsonobjArray, getIndex)

   /* static function define */
   JS_STATIC_FUNC_DEFINE(_sptBsonobjArray, help)

   JS_BEGIN_MAPPING( _sptBsonobjArray, "BSONArray" )
     JS_ADD_MEMBER_FUNC( "size", size )
     JS_ADD_MEMBER_FUNC( "more", more )
     JS_ADD_MEMBER_FUNC( "next", next )
     JS_ADD_MEMBER_FUNC( "pos", pos )
     JS_ADD_MEMBER_FUNC( "index", getIndex )
     JS_ADD_CONSTRUCT_FUNC( construct )
     JS_ADD_DESTRUCT_FUNC( destruct )
     /* static function */
     JS_ADD_STATIC_FUNC("help", help)
   JS_MAPPING_END()

   _sptBsonobjArray::_sptBsonobjArray()
   {
      _curPos = 0 ;
   }

   _sptBsonobjArray::_sptBsonobjArray( const vector< BSONObj > &vecObjs )
   {
      for ( UINT32 i = 0 ; i < vecObjs.size() ; ++i )
      {
         _vecObj.push_back( vecObjs[ i ].getOwned() ) ;
      }
      _curPos = 0 ;
   }

   _sptBsonobjArray::~_sptBsonobjArray()
   {
   }

   INT32 _sptBsonobjArray::construct( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "new BSONObjArray is forbidden." ) ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptBsonobjArray::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::help( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      stringstream ss ;
      ss << "BSONArray functions:" << endl
         << " BSONArray(obj).size()" << endl
         << " BSONArray(obj).more()" << endl
         << " BSONArray(obj).next()" << endl
         << " BSONArray(obj).pos()" << endl
         << " BSONArray(obj).toArray()" << endl
         << " BSONArray(obj).toString()" << endl
         << " BSONArray(obj).index()" << endl ;
      rval.setStringVal( "", ss.str().c_str() ) ;
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::size( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      INT32 size = _vecObj.size() ;
      rval.setNativeVal( "",  NumberInt, (const void*)&size ) ;
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::more( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      BOOLEAN hasMore = FALSE ;
      if ( _curPos < _vecObj.size() )
      {
         hasMore = TRUE ;
      }
      rval.setNativeVal( "",  Bool, ( const void *)&hasMore ) ;

      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::next( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      if ( _curPos < _vecObj.size() )
      {
         rval.setBSONObj( "", _vecObj[_curPos] ) ;
         ++_curPos ;
      }

      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::pos( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      if ( _curPos < _vecObj.size() )
      {
         rval.setBSONObj( "", _vecObj[_curPos] ) ;
      }
      return SDB_OK ;
   }

   INT32 _sptBsonobjArray::getIndex( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      rval.setNativeVal( "", NumberInt, (const void *)&_curPos ) ;
      return SDB_OK ;
   }

}

