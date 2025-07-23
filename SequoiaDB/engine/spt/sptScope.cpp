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

   Source File Name = sptScope.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptScope.hpp"
#include "pd.hpp"
#include "sptObjDesc.hpp"
#include "ossUtil.hpp"
#include "sptCommon.hpp"
#include "ossIO.hpp"
#include <algorithm>
using namespace std ;
namespace engine
{

   /*
      _sptResultVal implement
   */
   _sptResultVal::_sptResultVal()
   {
   }

   _sptResultVal::~_sptResultVal()
   {
   }

   BOOLEAN _sptResultVal::hasError() const
   {
      return _errStr.empty() ? FALSE : TRUE ;
   }

   const CHAR* _sptResultVal::getErrrInfo() const
   {
      return _errStr.c_str() ;
   }

   void _sptResultVal::setError( const  string &err )
   {
      _errStr = err ;
   }

   /*
      _sptScope implement
   */
   _sptScope::_sptScope()
   {
      _loadMask = 0 ;
   }

   _sptScope::~_sptScope()
   {

   }

   INT32 _sptScope::getLastError() const
   {
      return sdbGetErrno() ;
   }

   const CHAR* _sptScope::getLastErrMsg() const
   {
      return sdbGetErrMsg() ;
   }

   bson::BSONObj _sptScope::getLastErrObj() const
   {
      const CHAR *pObjData = sdbGetErrorObj() ;

      if ( pObjData )
      {
         try
         {
            bson::BSONObj obj( pObjData ) ;
            return obj ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      return bson::BSONObj() ;
   }

   INT32 _sptScope::loadUsrDefObj( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != desc, "desc can not be NULL" ) ;
      SDB_ASSERT( NULL != desc->getJSClassName(),
                  "obj name can not be empty" ) ;
      rc = _loadUsrDefObj( desc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to load object defined by user:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptScope::pushJSFileNameToStack( const string &filename )
   {
      _fileNameStack.push_back( filename ) ;
   }

   void _sptScope::popJSFileNameFromStack()
   {
      _fileNameStack.pop_back() ;
   }

   INT32 _sptScope::getStackSize()
   {
      return _fileNameStack.size() ;
   }

   void _sptScope::addJSFileNameToList( const string &filename )
   {
      _fileNameList.push_back( filename ) ;
   }

   void _sptScope::clearJSFileNameList()
   {
      _fileNameList.clear() ;
   }

   BOOLEAN _sptScope::isJSFileNameExistInStack( const string &filename )
   {
      BOOLEAN isExist = FALSE ;
      if( find( _fileNameStack.begin(), _fileNameStack.end(), filename )
          != _fileNameStack.end() )
      {
         isExist = TRUE ;
      }
      return isExist ;
   }

   BOOLEAN _sptScope::isJSFileNameExistInList( const string &filename )
   {
      BOOLEAN isExist = FALSE ;
      if( find( _fileNameList.begin(), _fileNameList.end(), filename )
          != _fileNameList.end() )
      {
         isExist = TRUE ;
      }
      return isExist ;
   }

   string _sptScope::calcImportPath( const string &filename )
   {
      BOOLEAN isAbsolute = FALSE ;
      const CHAR *pName = filename.c_str() ;
      string prefixPath ;

#if defined (_LINUX)
      if ( '/' == pName[0] )
      {
         isAbsolute = TRUE ;
      }
#else
      if ( '\\' == pName[0] || ( 0 != pName[0] && ':' == pName[1] ) )
      {
         isAbsolute = TRUE ;
      }
#endif // _LINUX

      if ( isAbsolute )
      {
         return filename ;
      }

      if ( _fileNameStack.size() > 0 )
      {
         string lastfile = *_fileNameStack.rbegin() ;
         UINT64 pos1 = lastfile.find_last_of( '/' ) ;
         UINT64 pos = pos1 ;

#if defined (_WINDOWS)
         UINT64 pos2 = lastfile.find_last_of( '\\' ) ;
         if ( pos2 != string::npos && ( string::npos == pos || pos2 > pos ) )
         {
            pos = pos2 ;
         }
#endif // _WINDOWS
         prefixPath = lastfile.substr( 0, pos ) ;
      }
      else
      {
         CHAR szPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossGetCWD( szPath, OSS_MAX_PATHSIZE ) ;
         prefixPath = szPath ;
      }

      if ( !filename.empty() )
      {
         pName = prefixPath.c_str() ;
         UINT32 len = ossStrlen( pName ) ;
         if ( len > 0 && pName[ len -1 ] != OSS_FILE_SEP_CHAR )
         {
            prefixPath += OSS_FILE_SEP_CHAR ;
         }
         prefixPath += filename ;
      }

      return prefixPath ;
   }

}
