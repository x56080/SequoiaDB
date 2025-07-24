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

   Source File Name = inspectTargetParser.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef INSPECT_TARGETPARSER_HPP__
#define INSPECT_TARGETPARSER_HPP__

#include <map>
#include <set>
#include <vector>
#include "client.hpp"
#include "dms.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"


// Secret file format:
// magic value
// original file size
// file contents
class _fileGuard
{
public:
   _fileGuard() {}
   ~_fileGuard() {}

   INT32 isSecretFile( const CHAR *inFilePath, BOOLEAN &result,
                       INT64 *originalSize = NULL ) ;
   INT32 encrypt( const CHAR *inFilePath, const CHAR *outFilePath ) ;
   INT32 decrypt( const CHAR *inFilePath, const CHAR *outFilePath ) ;
   INT32 decrypt( const CHAR *inFilePath, CHAR *buff, UINT32 buffLen ) ;
} ;
typedef _fileGuard fileGuard ;

extern fileGuard gFileGuard ;

class _collectionItem
{
public:
   _collectionItem( const CHAR *clFullName )
   {
      ossMemset( _fullName, 0, sizeof( _fullName ) ) ;
      ossStrncpy( _fullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
   }
   ~_collectionItem() {}

   const CHAR *getCLFullName() const
   {
      return _fullName ;
   }

   INT32 linkGroup( UINT32 groupIndex ) ;
   const std::set<UINT32>& getLinkedGroupIndex() const
   {
      return _groupIndexList ;
   }

private:
   CHAR _fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
   std::set<UINT32> _groupIndexList ;
} ;
typedef _collectionItem collectionItem ;

class _inspectTargetParser
{
   struct cmp_clName
   {
      bool operator()( const CHAR *a, const CHAR *b ) const
      {
         return std::strcmp(a, b) < 0 ;
      }
   } ;
   typedef std::map<const CHAR *, collectionItem *, cmp_clName> CL_GROUP_MAP ;
   typedef CL_GROUP_MAP::iterator CL_GROUP_MAP_ITR ;
   typedef CL_GROUP_MAP::const_iterator CL_GROUP_MAP_CITR ;

public:
   _inspectTargetParser() ;
   ~_inspectTargetParser() ;

   INT32 parseByData( const CHAR *data, UINT32 dataLen ) ;
   INT32 parseByConfFile( const CHAR *filePath, const CHAR *coordAddr,
                          const CHAR *service, const CHAR *user = NULL,
                          const CHAR *password = NULL ) ;

   // Whether this group is included in our target?
   BOOLEAN isTarget( const CHAR *groupName ) const ;

   // Whether this collection on this group is included in our target?
   BOOLEAN isTarget( const CHAR *groupName, const CHAR *clFullName ) const ;

   const vector<string>& getGroups() const
   {
      return _groups ;
   }

#ifdef _DEBUG
   // For debug purpose
   void printDetailInLog() const ;
#endif

private:
   INT32 _parseFileItem( const CHAR *data ) ;
   INT32 _appendToGroupList( const string& groupName, UINT32 &index ) ;
   INT32 _appendToCLContainer( const CHAR *fullName, UINT32 groupIndex ) ;

private:
   vector<string> _groups ;
   CL_GROUP_MAP _clGroupMap ;
} ;
typedef _inspectTargetParser inspectTargetParser ;

#endif /* INSPECT_TARGETPARSER_HPP__ */
