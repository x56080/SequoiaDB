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

   Source File Name = inspectTargetParser.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <iostream>
#include "inspectTargetParser.hpp"
#include "ossIO.hpp"
#include "pd.hpp"

#define INSPECT_NODE_DELIMITER  ','
#define INSPECT_GROUP_DELIMITER ':'
#define INSPECT_DOT             '.'

#define INSPECT_MAGIC_VALUE           "cf9a07bfd0214dfd9537320156fc004b"
#define INSPECT_MAGIC_LEN             ossStrlen(INSPECT_MAGIC_VALUE)
#define INSPECT_ENCRYPTBUFF_SIZE      1024
#define INSPECT_ENCRYPT_KEY           0x86

fileGuard gFileGuard ;

INT32 _fileGuard::isSecretFile( const CHAR *inFilePath, BOOLEAN &result,
                                INT64 *originalSize )
{
   INT32 rc = SDB_OK ;
   OSSFILE inFile ;
   INT64 readLen = 0 ;
   INT64 origFileSize = 0 ;
   CHAR readBuff[ INSPECT_ENCRYPTBUFF_SIZE ] = { 0 } ;

   rc = ossOpen( inFilePath, OSS_READONLY, OSS_RU, inFile ) ;
   PD_RC_CHECK( rc, PDERROR, "Open file[%s] failed: %d", inFilePath, rc ) ;

   // Read the magic value.
   rc = ossRead( &inFile, readBuff, INSPECT_MAGIC_LEN, &readLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Read file[%s] failed: %d", inFilePath, rc ) ;

   if ( (UINT32)readLen < INSPECT_MAGIC_LEN ||
        0 != ossStrncmp( readBuff, INSPECT_MAGIC_VALUE, INSPECT_MAGIC_LEN ) )
   {
      result = FALSE ;
      goto done ;
   }

   rc = ossRead( &inFile, (CHAR *)&origFileSize, sizeof(INT64), &readLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Read original length from file[%s] "
                             "failed: %d", inFilePath, rc ) ;

   result = TRUE ;

   if ( originalSize )
   {
      *originalSize = origFileSize ;
   }

done:
   ossClose( inFile ) ;
   return rc ;
error:
   goto done ;
}

INT32 _fileGuard::encrypt( const CHAR *inFilePath, const CHAR *outFilePath )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( inFilePath && outFilePath, "File path is NULL" ) ;

   OSSFILE inFile ;
   OSSFILE outFile ;
   INT64 inFileSize = 0 ;
   CHAR *buffer = NULL ;
   INT64 readLen = 0 ;
   INT64 writeLen = 0 ;
   rc = ossOpen( inFilePath, OSS_READONLY, OSS_RU, inFile ) ;
   PD_RC_CHECK( rc, PDERROR, "Open file[%s] failed: %d", inFilePath, rc ) ;
   rc = ossGetFileSize( &inFile, &inFileSize ) ;
   PD_RC_CHECK( rc, PDERROR, "Get size of file[%s] failed: %d",
                inFilePath, rc ) ;

   rc = ossOpen( outFilePath, OSS_WRITEONLY | OSS_CREATE,
                 OSS_RU | OSS_WU, outFile ) ;
   PD_RC_CHECK( rc, PDERROR, "Open output file[%d] failed: %d",
                outFilePath, rc ) ;

   buffer = (CHAR *)SDB_OSS_MALLOC( inFileSize + 1 ) ;
   PD_CHECK( buffer, SDB_OOM, error, PDERROR, "Allocate memory for file buffer "
                                              "failed: %d", rc ) ;
   ossMemset( buffer, 0, inFileSize + 1 ) ;
   rc = ossRead( &inFile, buffer, inFileSize, &readLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Read from file[%s] faile: %d", inFilePath, rc ) ;

   rc = ossWrite( &outFile, INSPECT_MAGIC_VALUE,
                  ossStrlen( INSPECT_MAGIC_VALUE ), &writeLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Write to file[%s] failed: %d", outFilePath, rc ) ;
   // Write the original length after the magic value.
   rc = ossWrite( &outFile, (const CHAR *)&inFileSize,
                  sizeof(INT64), &writeLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Write original file size to file[%s] failed: %d",
                outFilePath, rc ) ;
   {
      CHAR encryptBuff[ INSPECT_ENCRYPTBUFF_SIZE ] = { 0 } ;

      INT32 j = 0 ;
      for ( INT64 i = 0; i < readLen; ++i )
      {
         if ( '\r' == buffer[i] )
         {
            PD_LOG( PDERROR, "The file format may be Windows style. Change to "
                             "Unix stype first" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( '\n' == buffer[i] )
         {
            buffer[i] = ',' ;
         }
         encryptBuff[j++] = buffer[i] ^ INSPECT_ENCRYPT_KEY ;
         if ( INSPECT_ENCRYPTBUFF_SIZE == j )
         {
            rc = ossWrite( &outFile, encryptBuff,
                           INSPECT_ENCRYPTBUFF_SIZE, &writeLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Write to file[%s] failed: %d",
                         outFilePath, rc ) ;
            ossMemset( encryptBuff, 0, INSPECT_ENCRYPTBUFF_SIZE ) ;
            j = 0 ;
         }
      }

      // Write last batch of data.
      if ( '\0' != encryptBuff[0] )
      {
         UINT32 actualLen =
               ('\0' != encryptBuff[INSPECT_ENCRYPTBUFF_SIZE - 1] ) ?
               INSPECT_ENCRYPTBUFF_SIZE : ossStrlen(encryptBuff) ;

         rc = ossWrite( &outFile, encryptBuff, actualLen, &writeLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Write to file[%s] failed: %d",
                      outFilePath, rc ) ;
      }
   }

done:
   ossClose( inFile ) ;
   ossClose( outFile ) ;
   if ( buffer )
   {
      SDB_OSS_FREE( buffer ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _fileGuard::decrypt( const CHAR *inFilePath, const CHAR *outFilePath )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( inFilePath && outFilePath, "argument is invalid" ) ;

   CHAR readBuff[ INSPECT_ENCRYPTBUFF_SIZE ] = { 0 } ;
   OSSFILE inFile ;
   OSSFILE outFile ;
   INT64 readLen = 0 ;
   INT64 origFileSize = 0 ;
   rc = ossOpen( inFilePath, OSS_READONLY, OSS_RU, inFile ) ;
   PD_RC_CHECK( rc, PDERROR, "Open file[%s] failed: %d", inFilePath, rc ) ;

   // Read the magic value.
   rc = ossRead( &inFile, readBuff, INSPECT_MAGIC_LEN, &readLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Read file[%s] failed: %d", inFilePath, rc ) ;

   if ( 0 != ossStrncmp( INSPECT_MAGIC_VALUE, readBuff, INSPECT_MAGIC_LEN ) )
   {
      PD_LOG( PDERROR, "Magic value is wrong, may be it's not a encrypted "
                       "filed[%s]", inFilePath ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = ossRead( &inFile, (CHAR *)&origFileSize, sizeof(INT64), &readLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Read original length from file[%s] "
                             "failed: %d", inFilePath, rc ) ;

   rc = ossOpen( outFilePath, OSS_WRITEONLY | OSS_CREATE,
                 OSS_RU | OSS_WU, outFile ) ;
   PD_RC_CHECK( rc, PDERROR, "Open file[%s] failed: %d", outFilePath, rc ) ;


   while ( ( SDB_OK == (rc = ossRead( &inFile, readBuff,
                                      INSPECT_ENCRYPTBUFF_SIZE, &readLen ) ) )
           && readLen > 0 )
   {
      INT64 writeLen = 0 ;
      for ( UINT32 i = 0; i < readLen; ++i )
      {
         readBuff[i] ^= INSPECT_ENCRYPT_KEY ;
         if ( ',' == readBuff[i] )
         {
            readBuff[i] = '\n' ;
         }
      }

      rc = ossWrite( &outFile, readBuff, readLen, &writeLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Write content to file[%s] failed: %d",
                   outFilePath, rc ) ;

      if ( readLen < INSPECT_ENCRYPTBUFF_SIZE )
      {
         // Reach end of file.
         break ;
      }
   }

   if ( SDB_EOF == rc )
   {
      rc = SDB_OK ;
   }

done:
   ossClose( inFile ) ;
   ossClose( outFile ) ;
   return rc ;
error:
   goto done ;
}

INT32 _fileGuard::decrypt( const CHAR *inFilePath, CHAR *buff, UINT32 buffLen )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( inFilePath && buff, "argument is invalid" ) ;

   CHAR readBuff[ INSPECT_ENCRYPTBUFF_SIZE ] = { 0 } ;
   // make sure it's secret file.
   OSSFILE inFile ;
   INT64 readLen = 0 ;
   INT64 origFileSize = 0 ;
   INT64 writePos = 0 ;
   rc = ossOpen( inFilePath, OSS_READONLY, OSS_RU, inFile ) ;
   PD_RC_CHECK( rc, PDERROR, "Open file[%s] failed: %d", inFilePath, rc ) ;

   // Read the magic value.
   rc = ossRead( &inFile, readBuff, INSPECT_MAGIC_LEN, &readLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Read file[%s] failed: %d", inFilePath, rc ) ;

   if ( 0 != ossStrncmp( INSPECT_MAGIC_VALUE, readBuff, INSPECT_MAGIC_LEN ) )
   {
      PD_LOG( PDERROR, "Magic value is wrong, may be it's not a encrypted "
                       "filed[%s]", inFilePath ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = ossRead( &inFile, (CHAR *)&origFileSize, sizeof(INT64), &readLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Read original length from file[%s] "
                             "failed: %d", inFilePath, rc ) ;

   if ( buffLen < origFileSize )
   {
      PD_LOG( PDERROR, "Decrypt buffer too small" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while ( ( SDB_OK == (rc = ossRead( &inFile, readBuff,
                                      INSPECT_ENCRYPTBUFF_SIZE, &readLen ) ) )
           && readLen > 0 )
   {
      for ( UINT32 i = 0; i < readLen; ++i )
      {
         buff[writePos++] = readBuff[i] ^ INSPECT_ENCRYPT_KEY ;
      }

      if ( readLen < INSPECT_ENCRYPTBUFF_SIZE )
      {
         // Reach end of file.
         break ;
      }
   }

   if ( SDB_EOF == rc )
   {
      rc = SDB_OK ;
   }

done:
   ossClose( inFile ) ;
   return rc ;
error:
   goto done ;
}

INT32 _collectionItem::linkGroup( UINT32 groupIndex )
{
   _groupIndexList.insert( groupIndex ) ;

   return SDB_OK ;
}

_inspectTargetParser::_inspectTargetParser()
{

}

_inspectTargetParser::~_inspectTargetParser()
{

}

INT32 _inspectTargetParser::parseByData( const CHAR *data, UINT32 dataLen )
{
   INT32 rc = SDB_OK ;
   INT64 offset = 0 ;
   // Max item size is OSS_MAX_GROUPNAME_SIZE + DMS_COLLECTION_FULLNAME_SZ + 2
   const UINT32 maxItemSize = 384 ;

   SDB_ASSERT( data && dataLen > 0, "data is invalid" ) ;

   while ( offset < dataLen )
   {
      const CHAR *startPos = data + offset ;
      const CHAR *pos = ossStrchr( startPos, INSPECT_NODE_DELIMITER ) ;
      if ( pos )
      {
         *(CHAR *)pos = '\0' ;
         if ( pos - startPos > maxItemSize )
         {
            PD_LOG( PDERROR, "File context length exceeds the threshold. Please "
                             "check it: %s", startPos ) ;
            *(CHAR *)pos = INSPECT_NODE_DELIMITER ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      rc = _parseFileItem( startPos ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Parse file item failed: %s", startPos ) ;
         if ( pos )
         {
            *(CHAR *)pos = INSPECT_NODE_DELIMITER ;
         }
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( pos )
      {
         *(CHAR *)pos = INSPECT_NODE_DELIMITER ;
         offset += (pos - startPos + 1) ;
      }
      else
      {
         // No more ',', so the end is hit.
         break ;
      }
   }

#ifdef _DEBUG
   printDetailInLog() ;
#endif

done:
   return rc ;
error:
   goto done ;
}

INT32 _inspectTargetParser::parseByConfFile( const CHAR *filePath,
                                             const CHAR *coordAddr,
                                             const CHAR *service,
                                             const CHAR *user,
                                             const CHAR *password )
{
   INT32 rc = SDB_OK ;
   OSSFILE fh ;
   INT64 size = 0 ;
   CHAR *buff = NULL ;
   INT64 readSize = 0 ;
   BOOLEAN isEncrypted = FALSE ;

   if ( !filePath )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = gFileGuard.isSecretFile( filePath, isEncrypted, &size )  ;
   PD_RC_CHECK( rc, PDERROR, "Check file type[%s] failed: %d", filePath, rc ) ;

   if ( !isEncrypted )
   {
      rc = ossOpen( filePath, OSS_READONLY, OSS_RU, fh ) ;
      PD_RC_CHECK( rc, PDERROR, "Open list file[%s] failed: %d", filePath, rc ) ;

      rc = ossGetFileSize( &fh, &size ) ;
      PD_RC_CHECK( rc, PDERROR, "Get size of file[%s] failed: %d", filePath, rc ) ;
   }

   buff = (CHAR *)SDB_OSS_MALLOC( size + 1 ) ;
   PD_CHECK( NULL != buff, SDB_OOM, error, PDERROR,
             "Allocate memory for parser buffer failed: %d", rc ) ;
   ossMemset( buff, 0, size + 1 ) ;

   if ( isEncrypted )
   {
      rc = gFileGuard.decrypt( filePath, buff, size ) ;
      PD_RC_CHECK( rc, PDERROR, "Decrypt file content failed: %d", rc ) ;
      readSize = size ;
   }
   else
   {
      rc = ossRead( &fh, buff, size, &readSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Read config file content failed: %d", rc ) ;

      if ( readSize <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "File[%s] is empty", filePath ) ;
         goto error ;
      }
   }

   if ( '\n' == buff[readSize - 1] )
   {
      buff[readSize - 1] = '\0' ;
      --readSize ;
      if ( '\r' == buff[readSize -1] )
      {
         buff[readSize - 1] = '\0' ;
         --readSize ;
      }
   }

   rc = parseByData( buff, readSize ) ;
   PD_RC_CHECK( rc, PDERROR, "Parse file content failed: %d", rc ) ;

done:
   ossClose( fh ) ;
   if ( buff )
   {
      SDB_OSS_FREE( buff ) ;
   }
   return rc ;
error:
   goto done ;
}

BOOLEAN _inspectTargetParser::isTarget( const CHAR *groupName ) const
{
   BOOLEAN result = FALSE ;

   for ( vector<string>::const_iterator citr = _groups.begin();
         citr != _groups.end(); ++citr )
   {
      if ( 0 == ossStrcmp( citr->c_str(), groupName) )
      {
         result = TRUE ;
         break ;
      }
   }

   return result ;
}

BOOLEAN _inspectTargetParser::isTarget( const CHAR *groupName,
                                        const CHAR *clFullName ) const
{
   BOOLEAN result = FALSE ;
   CL_GROUP_MAP_CITR citr = _clGroupMap.find( clFullName ) ;
   if ( citr != _clGroupMap.end() )
   {
      const set<UINT32>& groupIndices = citr->second->getLinkedGroupIndex() ;
      for ( set<UINT32>::const_iterator citr = groupIndices.begin();
            citr != groupIndices.end(); ++citr )
      {
         if ( 0 == ossStrcmp( groupName, _groups[*citr].c_str() ) )
         {
            result = TRUE ;
            break ;
         }
      }
   }

   return result ;
}

#ifdef _DEBUG
void _inspectTargetParser::printDetailInLog() const
{
   for ( CL_GROUP_MAP_CITR itr = _clGroupMap.begin(); itr != _clGroupMap.end();
         ++itr )
   {
      const collectionItem *item = itr->second ;
      const std::set<UINT32>& groups = item->getLinkedGroupIndex() ;
      for ( std::set<UINT32>::const_iterator sItr = groups.begin();
            sItr != groups.end(); ++sItr )
      {
         PD_LOG( PDEVENT, "Parsed collection: %s:%s",
                 _groups[*sItr].c_str(), item->getCLFullName() ) ;
      }
   }
}
#endif

INT32 _inspectTargetParser::_parseFileItem( const CHAR *data )
{
   INT32 rc = SDB_OK ;
   UINT32 dataLen = ossStrlen( data ) ;

   const CHAR *clName = NULL ;
   const CHAR *groupDel = NULL ;
   CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
   const CHAR *dotPos = ossStrchr( data, INSPECT_DOT ) ;
   if ( !dotPos || ( dotPos == data ) || ( dotPos == data + dataLen -1 ) )
   {
      PD_LOG( PDERROR, "File content is invalid: %s", data ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   clName = dotPos + 1 ;

   groupDel = ossStrchr( data, INSPECT_GROUP_DELIMITER ) ;
   if ( groupDel &&
        ( groupDel == data || ((groupDel + 1) == clName) || groupDel > clName ) )
   {
      PD_LOG( PDERROR, "Group is invalid: %s", data ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( groupDel )
   {
      UINT32 groupIndex = 0 ;
      string groupName( data, groupDel ) ;
      rc = _appendToGroupList( groupName, groupIndex ) ;
      PD_RC_CHECK( rc, PDERROR, "Append group[%s] to group list failed: %d",
                   rc ) ;

      ossStrncpy( clFullName, groupDel + 1, DMS_COLLECTION_FULL_NAME_SZ ) ;
      rc = _appendToCLContainer( clFullName, groupIndex ) ;
      PD_RC_CHECK( rc, PDERROR, "Append collection to container failed: %d",
                   rc ) ;
   }
   else
   {
      // TODO: Not implement now.
      // Group is not specified. Need to check on all possible groups.
      // Check on which groups the collections resides. Append all of them into
      // the group list.
      PD_LOG( PDERROR, "Group name is missing: %s", data ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _inspectTargetParser::_appendToGroupList( const string& groupName,
                                                UINT32 &index )
{
   INT32 rc = SDB_OK ;

   // Check the list, if it's already there, return the index.
   for ( UINT32 i = 0; i < _groups.size(); ++i )
   {
      if ( groupName == _groups[i] )
      {
         index = i ;
         goto done ;
      }
   }

   // The group is not in the group list yet. Append it to the end.
   _groups.push_back( groupName ) ;
   index = _groups.size() - 1 ;

done:
   return rc ;
}

INT32 _inspectTargetParser::_appendToCLContainer( const CHAR *fullName,
                                                  UINT32 groupIndex )
{
   INT32 rc = SDB_OK ;
   collectionItem *clItem = NULL ;

   CL_GROUP_MAP_ITR itr = _clGroupMap.find( fullName ) ;
   if ( itr == _clGroupMap.end() )
   {
      clItem = new collectionItem( fullName ) ;
      PD_CHECK( NULL != clItem, SDB_OOM, error, PDERROR,
                "Allocate memory for collection item failed: %d", rc ) ;
      clItem->linkGroup( groupIndex ) ;

      _clGroupMap[ clItem->getCLFullName() ] = clItem ;
   }
   else
   {
      itr->second->linkGroup( groupIndex ) ;
   }

done:
   return rc ;
error:
   if ( clItem )
   {
      SDB_OSS_DEL clItem ;
   }
   goto done ;
}
