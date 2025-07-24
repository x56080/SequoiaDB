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

   Source File Name = sdbConsistencyInspect.cpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data insert/update/delete. This file does NOT include index logic.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2014  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sdbInspect.hpp"
#include "ossUtil.hpp"
#include "utilParam.hpp"
#include "ossVer.hpp"
#include "client.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "inspectTargetParser.hpp"

using namespace engine;
using namespace bson ;
using namespace sdbclient ;

#define INSPECT_DEFAULT_BFILE_NAME  "out.bin"
#define INSPECT_DEFAULT_TFILE_NAME  "out.txt"
#define INSPECT_LOG_NAME            "sdbinspect.log"

inspectTargetParser* gTargetParser = NULL ;
_IParam* gOptionMgr = NULL ;
BOOLEAN gFastMode = FALSE ;

struct str_compare
{
   bool operator()( const CHAR *a, const CHAR *b ) const
   {
      return std::strcmp(a, b) < 0 ;
   }
} ;

void getBrakeOptions( INT32 &brakeTime, INT32 &brakeStep )
{
   gOptionMgr->getFieldInt( CONSISTENCY_INSPECT_BRAKETIME, brakeTime ) ;
   gOptionMgr->getFieldInt( CONSISTENCY_INSPECT_BRAKESTEP, brakeStep ) ;
}

/**
 * @brief Find the index of the object with the smallest OID.
 * @param doc  Contains a group of records for comparison.
 * @param nodeCount  Node number for consistency check in the replica group.
 * @return The index of the record which has the smallest OID in the array.
 *
 * May be more than one record contain the smallest OID. It's OK. In the
 * comparison of next step, this can be handled correctly.
 */
INT32 getMinObjectIndex( ciBson &doc, const INT32 nodeCount )
{
   bson::BSONElement eMin, ee ;

   INT32 idx = 0 ;
   INT32 minIndex = 0 ;
   // find the first valid object
   while ( doc.objs[idx].isEmpty() )
   {
      ++idx ;
      minIndex = idx ;
   }
   // get the id of object
   if ( doc.objs[0].getObjectID( eMin ) )
   {
   }
   // compare to other object
   for ( ; idx < nodeCount ; ++idx )
   {
      if ( !doc.objs[idx].isEmpty() )
      {
         bson::OID id ;
         if ( doc.objs[idx].getObjectID( ee ) )
         {
            if ( ee < eMin )
            {
               eMin = ee ;
               minIndex = idx ;
            }
         }
      }
   }

   return minIndex ;
}

INT32 dumpCiHeader( const ciHeader *header, CHAR *&buffer,
                    INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc  = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      bufferSize += CI_BUFFER_BLOCK ;
      buffer = ( CHAR *)SDB_OSS_REALLOC( buffer, bufferSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer, size = "
                   << bufferSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   len = 0 ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Tool Name    : sdbInspect"OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Tool Version : %d.%d"OSS_NEWLINE,
                       header->_mainVersion, header->_subVersion ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "------------------------------"
                       OSS_NEWLINE""OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Parameters:"OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Loop        : %d"OSS_NEWLINE,
                       header->_loop ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "action      : %s"OSS_NEWLINE,
                       header->_action ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "coorAddress : %s"OSS_NEWLINE,
                       header->_coordAddr ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "serviceName : %s"OSS_NEWLINE,
                       header->_serviceName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "username    : %s"OSS_NEWLINE,
                       g_username ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "group       : %s"OSS_NEWLINE,
                       header->_groupName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "cs name     : %s"OSS_NEWLINE,
                       header->_csName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "cl name     : %s"OSS_NEWLINE,
                       header->_clName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "file path   : %s"OSS_NEWLINE,
                       header->_filepath ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "output file : %s"OSS_NEWLINE,
                       header->_outfile ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "view        : %s"OSS_NEWLINE,
                       header->_view ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "------------------------------"
                       OSS_NEWLINE""OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiGroupHeader( const ciGroupHeader *header, CHAR *&buffer,
                         INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc  = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      bufferSize += CI_BUFFER_BLOCK ;
      buffer = ( CHAR *)SDB_OSS_REALLOC( buffer, bufferSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer, size = "
                   << bufferSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   len = 0 ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Replica Group:"OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Group ID     : %d"OSS_NEWLINE,
                       header->_groupID ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Group Name   : %s"OSS_NEWLINE,
                       header->_groupName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Nodes count  : %d"OSS_NEWLINE,
                       header->_nodeCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiNodeSimple( ciLinkList< ciNode > &nodes, CHAR *&buffer,
                        INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc     = SDB_OK ;
   ciNode *node = NULL ;
   INT64 len    = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      bufferSize += CI_BUFFER_BLOCK ;
      buffer = ( CHAR *)SDB_OSS_REALLOC( buffer, bufferSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer, size = "
                   << bufferSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   len = 0 ;
   nodes.resetCurrentNode() ;
   node = nodes.getHead() ;
   while ( NULL != node )
   {
      if ( ciNode::STATE_NORMAL != node->_state )
      {
         len += ossSnprintf( buffer + len,
                             bufferSize - len,
                             "    \"%s\" in node( ServiceName : %s )"
                             OSS_NEWLINE OSS_NEWLINE,
                             ciNode::stateDesc[ node->_state],
                             node->_serviceName ) ;
         CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      }
      node = nodes.next() ;
   }
   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiNode( ciLinkList< ciNode > &link, CHAR *&buffer,
                  INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc     = SDB_OK ;
   ciNode *node = NULL ;
   INT64 len    = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      bufferSize += CI_BUFFER_BLOCK ;
      buffer = ( CHAR *)SDB_OSS_REALLOC( buffer, bufferSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer, size = "
                   << bufferSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   len = 0 ;
   link.resetCurrentNode() ;
   node = link.getHead() ;
   while ( NULL != node )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node index       : %d"OSS_NEWLINE,
                          node->_index) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node ID          : %d"OSS_NEWLINE,
                          node->_nodeID ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node HostName    : %s"OSS_NEWLINE,
                          node->_hostname ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node ServiceName : %s"OSS_NEWLINE,
                          node->_serviceName ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    Node State       : %s"OSS_NEWLINE,
                          ciNode::stateDesc[ node->_state ] ) ;
      len += ossSnprintf( buffer + len, bufferSize - len, OSS_NEWLINE ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

      node = link.next() ;
   }

   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiClHeader( const ciClHeader *header, CHAR *&buffer,
                      INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc  = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      bufferSize += CI_BUFFER_BLOCK ;
      buffer = ( CHAR *)SDB_OSS_REALLOC( buffer, bufferSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer, size = "
                   << bufferSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   len = 0 ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Collection Full Name  : %s"OSS_NEWLINE,
                       header->_fullname ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "  Main Collection Name  : %s"OSS_NEWLINE,
                       header->_mainClName ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   if ( header->_recordCount <= 0 )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "    There is no record different"OSS_NEWLINE ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   }
   len += ossSnprintf( buffer + len, bufferSize - len, OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiRecord( ciLinkList< ciNode > &nodes,
                    ciLinkList< ciRecord > &link,
                    CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc        = SDB_OK ;
   INT32 nodeCount = nodes.count() ;
   ciRecord *rd    = NULL ;
   ciNode   *node  = NULL ;
   const CHAR *pst = NULL ;
   INT64 len       = 0 ;
   INT32 brakeTime = 0 ;
   INT32 brakeStep = 0 ;
   INT64 counter = 0 ;
   const CHAR *nodeState[] =
   {
      " 1",
      " 0",
      " x"
   } ;

   getBrakeOptions( brakeTime, brakeStep ) ;

retry:
   if ( bufferSize - 1 <= len )
   {
      bufferSize += CI_BUFFER_BLOCK ;
      buffer = ( CHAR *)SDB_OSS_REALLOC( buffer, bufferSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer, size = "
                   << bufferSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   len = 0 ;
   if ( link.count() > 0 )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "  # Node state 1 means node has the record,"
                          " or 0 means not, and x means node invliad"
                          OSS_NEWLINE
                          "  # The order is ascended by node index."
                          OSS_NEWLINE
                          "    There is [%d] piece of records that haven't been"
                          " synchronized."
                          OSS_NEWLINE""OSS_NEWLINE, link.count() ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   }
   else
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "   There is no record different"
                          OSS_NEWLINE""OSS_NEWLINE ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   }

   link.resetCurrentNode() ;
   rd = link.getHead() ;
   while ( NULL != rd )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "  -record     : %s"OSS_NEWLINE,
                          rd->_bson.toString().c_str() ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "  -Node State : " ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

      nodes.resetCurrentNode() ;
      node = nodes.getHead() ;
      for ( INT32 idx = 0 ; idx < nodeCount ; ++idx )
      {
         ciState st( rd->_state ) ;
         if ( st.hit( idx ) )
         {
            if ( ciNode::STATE_NORMAL != node->_state )
            {
               pst = nodeState[ 2 ] ;
            }
            else
            {
               pst = nodeState[ 0 ] ;
            }
         }
         else
         {
            pst = nodeState[ 1 ] ;
         }
         len += ossSnprintf( buffer + len, bufferSize - len, pst ) ;
         CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

         node = nodes.next() ;
      }
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
      len += ossSnprintf( buffer + len, bufferSize - len, OSS_NEWLINE ) ;
      CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

      rd = link.next() ;

      // Speed controller
      ++counter ;
      if ( brakeTime > 0 && brakeStep > 0 &&
           ( ( counter % brakeStep ) == 0 ) )
      {
         ossSleepmillis( brakeTime ) ;
      }
   }
   len += ossSnprintf( buffer + len, bufferSize - len, OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   validSize = len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpCiTail( ciTail &tail, CHAR *&buffer,
                  INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;
   INT64 len = 0 ;

retry:
   if ( bufferSize - 1 <= len )
   {
      bufferSize += CI_BUFFER_BLOCK ;
      buffer = ( CHAR *)SDB_OSS_REALLOC( buffer, bufferSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer, size = "
                   << bufferSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Inspect result:"OSS_NEWLINE ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total inspected group count       : %u"OSS_NEWLINE,
                       tail._groupCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total inspected collection        : %u"OSS_NEWLINE,
                       tail._clCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total different collections count : %u"OSS_NEWLINE,
                       tail._diffCLCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total different records count     : %lld"OSS_NEWLINE,
                       tail._recordCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;
   len += ossSnprintf( buffer + len, bufferSize - len,
                       "Total time cost                   : %llu ms"OSS_NEWLINE,
                       tail._timeCount ) ;
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   if ( 0 == tail._exitCode )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "Reason for exit : exit with no records "
                          "different"OSS_NEWLINE ) ;
   }
   else if ( 1 == tail._exitCode )
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "Reason for exit : exit with less than 1%% of "
                          "records not synchronized"OSS_NEWLINE ) ;
   }
   else
   {
      len += ossSnprintf( buffer + len, bufferSize - len,
                          "Reason for exit : loop is limited"OSS_NEWLINE ) ;
   }
   CHECK_VALUE( ( bufferSize - 1 <= len ), retry ) ;

   validSize = len ;

done:
   return rc ;
error:
   goto done ;
}

BOOLEAN findCiOffset( ciLinkList< ciOffset > &clOffset, const INT64 offset )
{
   BOOLEAN find = FALSE ;

   clOffset.resetCurrentNode() ;
   ciOffset *off = clOffset.getHead() ;

   while ( NULL != off )
   {
      if ( offset == off->_offset )
      {
         find = TRUE ;
         goto done ;
      }
      off = clOffset.next() ;
   }

done:
   return find ;
}

/**
** read data from file
***/
INT32 readFromFile( OSSFILE &in, INT64 &offset,
                    CHAR *buffer, const INT64 readSize )
{
   INT32 rc       = SDB_OK ;
   ///< read from file
   INT64 restLen  = readSize ;
   INT64 readPos  = 0 ;
   INT64 readLen  = 0 ;

   while( restLen > 0 )
   {
      rc = ossSeekAndRead( &in, offset, buffer + readPos,
                           restLen, &readLen ) ;
      if ( SDB_OK != rc && SDB_INTERRUPT != rc )
      {
         std::cout << "Failed to read data from file" << std::endl ;
         goto error ;
      }

      rc = SDB_OK ;
      restLen -= readLen ;
      readPos += readLen ;
   }

   offset += readSize ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write buffer to file
***/
INT32 writeToFile( OSSFILE &out, const CHAR *buffer, const INT64 bufferSize )
{
   INT32 rc        = SDB_OK ;
   ///< write buffer
   INT64 restLen   = bufferSize ;
   INT64 writePos  = 0 ;
   INT64 writeSize = 0 ;
   while( restLen > 0 )
   {
      rc = ossWrite( &out, buffer + writePos, restLen, &writeSize ) ;
      if ( SDB_OK != rc && SDB_INTERRUPT != rc )
      {
         std::cout << "Failed to write data to file" << std::endl ;
         goto error ;
      }

      rc = SDB_OK ;
      restLen -= writeSize ;
      writePos += writeSize ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 writeToFileHeader( OSSFILE &out,
                         const CHAR *buffer, const INT64 bufferSize )
{
   INT32 rc        = SDB_OK ;
   ///< write buffer
   INT64 restLen   = bufferSize ;
   INT64 writePos  = 0 ;
   INT64 writeSize = 0 ;
   while( restLen > 0 )
   {
      rc = ossSeekAndWrite( &out, 0, buffer + writePos, restLen, &writeSize ) ;
      if ( SDB_OK != rc && SDB_INTERRUPT != rc )
      {
         std::cout << "Failed to write data to file" << std::endl ;
         goto error ;
      }

      rc = SDB_OK ;
      restLen -= writeSize ;
      writePos += writeSize ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciHeader from file
***/
INT32 readCiHeader( OSSFILE &in, ciHeader *header )
{
   INT32 rc          = SDB_OK ;
   INT32 len         = 0 ;
   INT32 mainVersion = 0 ;
   INT32 subVersion  = 0 ;
   INT64 tmpOffset   = 0 ;
   CHAR  eyeCatcher[ CI_EYECATCHER_SIZE ] = { 0 } ;
   CHAR  buffer[ CI_HEADER_SIZE ] = { 0 } ;

   rc = readFromFile( in, tmpOffset, buffer, CI_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // try to copy
   ossMemcpy( eyeCatcher, buffer, CI_EYECATCHER_SIZE ) ;
   if ( 0 != ossStrncmp( CI_HEADER_EYECATCHER,
      eyeCatcher, CI_EYECATCHER_SIZE ) )
   {
      std::cout << "Error: eyeCatcher is invalid" << std::endl ;
      goto error ;
   }
   len += CI_EYECATCHER_SIZE ;

   // try copy main version
   ossMemcpy( &mainVersion, buffer + len, sizeof( INT32 ) ) ;
   if ( mainVersion > header->_mainVersion )
   {
      std::cout << "Error: the main version in file header is larger "
                << "than the main version of current tools" << std::endl ;
      goto error ;
   }
   len += sizeof( INT32 ) ;

   //try copy sub version
   ossMemcpy( &subVersion, buffer + len, sizeof( INT32 ) ) ;
   if ( ( mainVersion == header->_mainVersion ) &&
        ( subVersion > header->_subVersion ) )
   {
      std::cout << "Error: the sub version in file header is larger "
                << "than the sub version of current tools" << std::endl ;
      goto error ;
   }
   len += sizeof( INT32 ) ;

   // copy loop
   ossMemcpy( &header->_loop, buffer + len, sizeof(INT32) ) ;
   if ( 0 >= header->_loop )
   {
      std::cout << "Warning: the value of loop is invalid. "
                << "Modify it to default value(5)." << std::endl ;
      header->_loop = 5 ;
   }
   len += sizeof( INT32 ) ;

   ossMemcpy( &header->_tailSize, buffer + len, sizeof(UINT64) ) ;
   len += sizeof( UINT64 ) ;

   // copy actions
   ossMemcpy( header->_action, buffer + len, CI_ACTION_SIZE ) ;
   if ( 0 != ossStrncmp( CI_ACTION_INSPECT,
                         header->_action, CI_ACTION_SIZE ) &&
        0 != ossStrncmp( CI_ACTION_REPORT,
                         header->_action, CI_ACTION_SIZE ) )
   {
      std::cout << "Error: action is invalid" << std::endl ;
      goto error ;
   }
   len += CI_ACTION_SIZE ;

   // copy coord hostname
   ossMemcpy( header->_coordAddr, buffer + len, CI_HOSTNAME_SIZE + 1 ) ;
   len += CI_HOSTNAME_SIZE + 1 ;
   // copy coord service name
   ossMemcpy( header->_serviceName, buffer + len, CI_SERVICENAME_SIZE + 1 ) ;
   len += CI_SERVICENAME_SIZE + 1 ;
   // copy username and password
   ossMemcpy( g_username, buffer + len, CI_USERNAME_SIZE + 1 ) ;
   len += CI_USERNAME_SIZE + 1 ;
   ossMemcpy( g_password, buffer + len, CI_PASSWD_SIZE + 1 ) ;
   len += CI_PASSWD_SIZE + 1 ;
   // copy group name
   ossMemcpy( header->_groupName, buffer + len, CI_GROUPNAME_SIZE + 1 ) ;
   len += CI_GROUPNAME_SIZE + 1 ;
   // copy collection space name
   ossMemcpy( header->_csName, buffer + len, CI_CS_NAME_SIZE + 1 ) ;
   len += CI_CS_NAME_SIZE + 1 ;
   // copy collection name
   ossMemcpy( header->_clName, buffer + len, CI_CL_NAME_SIZE + 1) ;
   len += CI_CL_NAME_SIZE + 1 ;
   // skip file path
   ossMemcpy( header->_filepath, buffer + len, OSS_MAX_PATHSIZE + 1) ;
   len += OSS_MAX_PATHSIZE + 1 ;
   // skip out file
   ossMemcpy( header->_outfile, buffer + len, OSS_MAX_PATHSIZE + 1) ;
   len += OSS_MAX_PATHSIZE + 1 ;
   // copy view format string
   ossMemcpy( header->_view, buffer + len, CI_VIEWOPTION_SIZE + 1 ) ;
   len += CI_VIEWOPTION_SIZE + 1 ;
   // copy padding? it seems useless
   // skip first

done:
   return rc ;
error:
   rc = HEADER_PARSE_ERROR ;
   goto done ;
}

/**
** copy ciHeader data to buffer
***/
INT32 ciHeaderToBuffer( const ciHeader *header, CHAR *&buffer,
                        INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;
   INT32 pos = 0 ;

   validSize = CI_HEADER_SIZE ;
   if ( validSize > bufferSize )
   {
      buffer = ( CHAR * )SDB_OSS_REALLOC( buffer, validSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer. size = "
            << validSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = validSize ;
   }

   ossMemcpy( buffer + pos, header->_eyeCatcher, CI_EYECATCHER_SIZE ) ;
   pos += CI_EYECATCHER_SIZE ;
   ossMemcpy( buffer + pos, &header->_mainVersion, sizeof(INT32) ) ;
   pos += sizeof( INT32 ) ;
   ossMemcpy( buffer + pos, &header->_subVersion, sizeof(INT32) ) ;
   pos += sizeof( INT32 ) ;
   ossMemcpy( buffer + pos, &header->_loop, sizeof(INT32) ) ;
   pos += sizeof( INT32 ) ;
   ossMemcpy( buffer + pos, &header->_tailSize, sizeof(UINT64) ) ;
   pos += sizeof( UINT64 ) ;
   ossMemcpy( buffer + pos, header->_action, CI_ACTION_SIZE ) ;
   pos += CI_ACTION_SIZE ;
   ossMemcpy( buffer + pos, header->_coordAddr, CI_HOSTNAME_SIZE + 1 ) ;
   pos += CI_HOSTNAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_serviceName, CI_SERVICENAME_SIZE + 1 ) ;
   pos += CI_SERVICENAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, g_username, CI_USERNAME_SIZE + 1 ) ;
   pos += CI_USERNAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, g_password, CI_PASSWD_SIZE + 1 ) ;
   pos += CI_PASSWD_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_groupName, CI_GROUPNAME_SIZE + 1 ) ;
   pos += CI_GROUPNAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_csName, CI_CS_NAME_SIZE + 1 ) ;
   pos += CI_CS_NAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_clName, CI_CL_NAME_SIZE + 1 ) ;
   pos += CI_CL_NAME_SIZE + 1 ;
   ossMemcpy( buffer + pos, header->_filepath, OSS_MAX_PATHSIZE + 1 ) ;
   pos += OSS_MAX_PATHSIZE + 1 ;
   ossMemcpy( buffer + pos, header->_outfile, OSS_MAX_PATHSIZE + 1 ) ;
   pos += OSS_MAX_PATHSIZE + 1 ;
   ossMemcpy( buffer + pos, header->_view, CI_VIEWOPTION_SIZE + 1 ) ;
   pos += CI_VIEWOPTION_SIZE + 1 ;
   ossMemset( buffer + pos, 0, validSize - pos ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciHeader to file
***/
INT32 writeCiHeader( OSSFILE &out, const ciHeader *header,
                     CHAR *&buffer, INT64 &bufferSize,
                     INT64 &validSize, BOOLEAN reWrite = FALSE )
{
   INT32 rc = SDB_OK ;

   rc = ciHeaderToBuffer( header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   if ( reWrite )
   {
      rc = writeToFileHeader( out, buffer, validSize ) ;
   }
   else
   {
      rc = writeToFile( out, buffer, validSize ) ;
   }
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciGroupHeader from file
***/
INT32 readCiGroupHeader( OSSFILE &in, INT64 &offset, ciGroupHeader *header )
{
   INT32 rc  = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_GROUP_HEADER_SIZE ] = { 0 } ;

   rc = readFromFile( in, offset, buffer, CI_GROUP_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   //offset += CI_GROUP_HEADER_SIZE ;

   ossMemcpy( &header->_groupID, buffer + len, sizeof( INT32 ) ) ;
   len += sizeof( INT32 ) ;
   ossMemcpy( &header->_nodeCount, buffer + len, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( &header->_clCount, buffer + len, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( &header->_groupName, buffer + len, CI_GROUPNAME_SIZE ) ;
   len += CI_GROUPNAME_SIZE ;
   ossMemcpy( &header->_maxCompleteLSN, buffer + len, sizeof( UINT64 ) ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciGroupHeader to file
***/
INT32 writeCiGroupHeader( OSSFILE &out, const ciGroupHeader *header )
{
   INT32 rc  = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_GROUP_HEADER_SIZE ] = { 0 } ;

   ossMemcpy( buffer + len, &header->_groupID, sizeof( INT32 ) ) ;
   len += sizeof( INT32 ) ;
   ossMemcpy( buffer + len, &header->_nodeCount, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( buffer + len, &header->_clCount, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( buffer + len, header->_groupName, CI_GROUPNAME_SIZE ) ;
   len += CI_GROUPNAME_SIZE ;
   ossMemcpy( buffer + len, &header->_maxCompleteLSN, sizeof( UINT64 ) ) ;

   rc = writeToFile( out, buffer, CI_GROUP_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciClHeader from file
***/
INT32 readCiClHeader( OSSFILE &in, INT64 &offset, ciClHeader *header )
{
   INT32 rc = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_CL_HEADER_SIZE ] = { 0 } ;

   rc = readFromFile( in, offset, buffer, CI_CL_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   //offset += CI_CL_HEADER_SIZE ;

   ossMemcpy( &header->_recordCount, buffer + len, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( header->_fullname, buffer + len, CI_CL_FULLNAME_SIZE ) ;
   len += CI_CL_FULLNAME_SIZE ;
   ossMemcpy( header->_mainClName, buffer + len, CI_CL_FULLNAME_SIZE ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciClHeader to file
***/
INT32 writeCiClHeader( OSSFILE &out, const ciClHeader *header )
{
   INT32 rc  = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[ CI_CL_HEADER_SIZE ] = { 0 } ;

   ossMemcpy( buffer + len, &header->_recordCount, sizeof( UINT32 ) ) ;
   len += sizeof( UINT32 ) ;
   ossMemcpy( buffer + len, header->_fullname, CI_CL_FULLNAME_SIZE ) ;
   len += CI_CL_FULLNAME_SIZE ;
   ossMemcpy( buffer + len, header->_mainClName, CI_CL_FULLNAME_SIZE ) ;

   rc = writeToFile( out, buffer, CI_CL_HEADER_SIZE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

BOOLEAN normalNodes( ciLinkList< ciNode > &nodes )
{
   BOOLEAN normal = TRUE ;
   ciNode *curNode = NULL ;

   nodes.resetCurrentNode() ;
   curNode = nodes.getHead() ;
   while ( NULL != curNode )
   {
      if ( ciNode::STATE_NORMAL != curNode->_state )
      {
         normal = FALSE ;
         break ;
      }
      curNode = nodes.next() ;
   }

   return normal ;
}

/**
** read ciNode from file
***/
INT32 readCiNode( OSSFILE &in, INT64 &offset,
                  const ciGroupHeader &header, ciLinkList< ciNode > &nodes )
{
   INT32 rc                    = SDB_OK ;
   CHAR buffer[ CI_NODE_SIZE ] = { 0 } ;
   INT32 pos                   = 0 ;
   UINT32 idx                  = 0 ;
   while ( idx < header._nodeCount )
   {
      pos = 0 ;
      ossMemset( buffer, 0, CI_NODE_SIZE ) ;

      rc = readFromFile( in, offset, buffer, CI_NODE_SIZE ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      //offset += CI_NODE_SIZE ;
      ciNode *node = nodes.createNode() ;
      if ( NULL == node )
      {
         std::cout << "Error: failed to allocate ciNode" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( &node->_index, buffer + pos, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( &node->_nodeID, buffer + pos, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( &node->_state, buffer + pos, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( node->_hostname, buffer + pos, CI_HOSTNAME_SIZE + 1) ;
      pos += CI_HOSTNAME_SIZE + 1 ;
      ossMemcpy( node->_serviceName, buffer + pos, CI_SERVICENAME_SIZE + 1 );

      nodes.add( node ) ;
      ++idx ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** copy ciNode data to buffer
***/
INT32 ciNodeToBuffer( ciLinkList< ciNode > &nodes, CHAR *&buffer,
                      INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc        = SDB_OK ;
   INT64 pos       = 0 ;
   ciNode *curNode = NULL ;
   UINT32 count    = nodes.count() ;
   INT32 unitLen   = CI_NODE_SIZE ;

   validSize = count * unitLen ;

   if ( validSize > bufferSize )
   {
      buffer = ( CHAR * )SDB_OSS_REALLOC( buffer, validSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer. size = "
                   << validSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = validSize ;
   }

   nodes.resetCurrentNode() ;
   curNode = nodes.getHead() ;
   while ( NULL != curNode )
   {
      ossMemcpy( buffer + pos, &curNode->_index, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( buffer + pos, &curNode->_nodeID, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( buffer + pos, &curNode->_state, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      ossMemcpy( buffer + pos, curNode->_hostname, CI_HOSTNAME_SIZE + 1) ;
      pos += CI_HOSTNAME_SIZE + 1 ;
      ossMemcpy( buffer + pos, curNode->_serviceName,
                 CI_SERVICENAME_SIZE + 1 ) ;
      pos += CI_SERVICENAME_SIZE + 1 ;
      curNode = nodes.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciNode to file
***/
INT32 writeCiNode( OSSFILE &out, ciLinkList< ciNode > &nodes,
                   CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;

   rc = ciNodeToBuffer( nodes, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
 * @brief Prepare the next group of records for comparison.
 * @param st  Last comparison status.
 * @param cursors  Cursors for the current collection on each node.
 * @param docs  Group of records fetched by this round.
 * @param inited
 *
 * It may fetch the next record from one or more nodes in the group, depending
 * on the last comparison result.
 * If the records of the last group are exactly the same, move forward on all
 * nodes. Otherwise, only move forward on the group(s) with the smallest OID.
 */
INT32 getNext( ciState &st, ciLinkList< ciCursor > &cursors,
               ciBson &docs, BOOLEAN inited = TRUE )
{
   INT32 rc = SDB_OK ;
   cursors.resetCurrentNode() ;
   ciCursor *cursor = cursors.getHead() ;
   INT32 idx = 0 ;
   while ( NULL != cursor )
   {
      // Move forward on all nodes, or just the nodes with the smallest OID.
      if ( st.hit( ALL_THE_SAME_BIT ) || ( st.hit( idx ) ) )
      {
         if ( NULL != cursor->_cursor )
         {
            rc = cursor->_cursor->next( docs.objs[ idx ] ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  std::cout << "Error: get record failed, rc = "
                            << rc << std::endl ;
                  rc = CI_INSPECT_ERROR ; // redo inspect from begin
                  goto error ;
               }
               else
               {
                  docs.objs[ idx ] = sdbclient::_sdbStaticObject ;
                  rc = SDB_OK ; // need reset the return value
               }
            }
         }
         else
         {
            if ( !inited )
            {
               docs.objs[ idx ] = sdbclient::_sdbStaticObject ;
            }
         }
      }
      ++idx ;
      cursor = cursors.next() ;
   }

   if ( !inited )
   {
      for ( ; idx < MAX_NODE_COUNT ; ++idx )
      {
         docs.objs[ idx ] = sdbclient::_sdbStaticObject ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/**
** get the cursor of records, the collection full name specified
***/
INT32 getCiCursor( ciLinkList< ciNode > &nodes, const CHAR* clName,
                   ciLinkList< ciCursor > &cursors, bson::BSONObj &con,
                   BOOLEAN orderCon = FALSE,
                   const bson::BSONObj &selector = sdbclient::_sdbStaticObject )
{
   INT32 rc = SDB_OK ;

   nodes.resetCurrentNode() ;
   ciNode *curNode = nodes.getHead() ;
   while ( NULL != curNode )
   {
      ciCursor *cursor = cursors.createNode() ;
      if ( NULL == cursor )
      {
         std::cout << "Error: failed to allocate ciCursor" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      cursor->_nodeID = curNode->_nodeID ;
      cursor->_index = curNode->_index ;


      sdbclient::sdb *db = curNode->_db ;
      sdbclient::sdbCollection cl ;
      sdbclient::sdbCursor *cr = NULL ;

      if ( NULL == db )
      {
         db = new sdbclient::sdb() ;
         if ( NULL == db )
         {
            std::cout << "Error: failed to allocate sdbclient::sdb"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         curNode->_db = db;
         rc = db->connect( curNode->_hostname, curNode->_serviceName,
                           g_username, g_password ) ;
         if ( SDB_OK != rc )
         {
            curNode->_state = ciNode::STATE_DISCONN ;
         }
      }

      if ( ciNode::STATE_DISCONN != curNode->_state )
      {
         curNode->_state = ciNode::STATE_NORMAL ;
         rc = db->getCollection( clName, cl ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            curNode->_state = ciNode::STATE_CLNOTEXIST ;
         }
         else if ( SDB_OK != rc )
         {
            curNode->_state = ciNode::STATE_CLFAILED ;
         }
      }

      if ( ciNode::STATE_NORMAL == curNode->_state )
      {
         cr = new sdbclient::sdbCursor() ;
         if ( NULL == cr )
         {
            std::cout << "Error: failed to allocate sdbclient::sdbCursor"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }

         // success to get cl
         if ( orderCon )
         {
            rc = cl.query( *cr, sdbclient::_sdbStaticObject,
                           selector, con ) ;
         }
         else
         {
            rc = cl.query( *cr, con, selector ) ;
         }
         if ( SDB_OK != rc )
         {
            DELETE_PTR(cr);
            curNode->_state = ciNode::STATE_CUSURFAILED ;
         }
      }
      cursor->_db = db ;
      cursor->_cursor = cr ;
      rc = SDB_OK ;

      cursors.add( cursor ) ;
      curNode = nodes.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** query bson record in each cursor
***/
BOOLEAN recordQuery( ciLinkList< ciNode > &nodes,
                     ciLinkList< ciCursor > &cursors,
                     ciState &state, bson::BSONObj &obj, INT32 &rc )
{
   BOOLEAN same = FALSE ;
   INT32 nodeCount = nodes.count() ;
   ciNode *node = NULL ;
   ciBson docs ;
   BSONElement baseOIDEle ;

   state.reset() ;
   // it's a trick to make sure that all cursors can get next.
   state.set( ALL_THE_SAME_BIT ) ;
   rc = getNext( state, cursors, docs, FALSE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), done ) ;

   nodes.resetCurrentNode() ;
   node = nodes.getHead() ;
   state.reset() ;

   if ( gFastMode )
   {
      obj.getObjectID( baseOIDEle ) ;
   }

   for ( INT32 idx = 0 ; idx < nodeCount ; ++idx, node = nodes.next() )
   {
      // the collection exists but is not available in node,
      // we assume the record exists
      if ( ciNode::STATE_NORMAL != node->_state &&
           ciNode::STATE_CLNOTEXIST != node->_state )
      {
         state.set( idx ) ;
      }
      else if ( ciNode::STATE_NORMAL == node->_state )
      {
         if ( gFastMode )
         {
            // Only compare OID
            BSONElement oidEle ;
            if ( docs.objs[idx].getObjectID( oidEle ) &&
                 oidEle.valuesEqual( baseOIDEle ) )
            {
               state.set( idx ) ;
            }
         }
         else if ( docs.objs[idx].equal( obj ) )
         {
            state.set( idx ) ;
         }
      }
   }

   if ( state._state == ( ( 1 << nodeCount ) - 1 ) || state._state == 0 )
   {
      same = TRUE ;
   }

done:
   return same ;
}

/**
** read ciRecord from file
***/
INT32 readCiRecord( OSSFILE &in, INT64 &offset,
                    ciLinkList< ciNode > &nodes,
                    const ciClHeader &header,
                    ciLinkList< ciRecord > &records, BOOLEAN dump = FALSE,
                    BOOLEAN repaire = FALSE )
{
   INT32 rc         = SDB_OK ;
   CHAR *bsonBuffer = NULL ;
   INT32 bufferLen  = 0 ;
   INT32 brakeTime = 0 ;
   INT32 brakeStep = 0 ;

   getBrakeOptions( brakeTime, brakeStep ) ;

   UINT32 idx = 0 ;
   // Check again for all the different records on all nodes in the group.
   while ( idx < header._recordCount )
   {
      INT32 recordLen = 0 ;
      CHAR  state ;
      CHAR  origState = 0 ;
      rc = readFromFile( in, offset, (CHAR *)&recordLen, sizeof( INT32 ) ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      if ( recordLen > bufferLen )
      {
         bsonBuffer = (CHAR *)SDB_OSS_REALLOC( bsonBuffer, recordLen ) ;
         if ( NULL == bsonBuffer )
         {
            std::cout << "Error: failed to allocate buffer. size = "
                      << recordLen << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         bufferLen = recordLen ;
      }
      // read bson
      rc = readFromFile( in, offset, ( CHAR * )bsonBuffer, recordLen ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // read state
      rc = readFromFile( in, offset, ( CHAR * )&state, sizeof( CHAR ) ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      rc = readFromFile( in, offset, ( CHAR * )&origState, sizeof( CHAR ) ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // make a condition of query
      bson::BSONObj obj( bsonBuffer ) ;
      bson::BSONElement e ;
      obj.getObjectID( e ) ;
      bson::BSONObj con = bob().append( e ).obj() ;

      ciLinkList< ciCursor > cursors ;
      if ( !dump )
      {
         rc = getCiCursor( nodes, header._fullname, cursors, con ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      }

      ciState st ;
      if ( dump || !recordQuery( nodes, cursors, st, obj, rc ) )
      {
         ciRecord *record = records.createNode() ;
         if ( NULL == record )
         {
            std::cout << "Error: failed to allocate ciRecord "
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         record->_bson = obj.copy() ;
         record->_len = obj.objsize() ;
         record->_state = dump ? state : st._state ;
         record->_origState = origState ;
         records.add( record ) ;
      }
      ++idx ;
      // Speed controller
      if ( brakeTime > 0 && brakeStep > 0 &&
           ( ( idx % brakeStep ) == 0 ) )
      {
         ossSleepmillis( brakeTime ) ;
      }
   }

done:
   if ( NULL != bsonBuffer )
   {
      SDB_OSS_FREE( bsonBuffer ) ;
      bsonBuffer = NULL ;
   }
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 repairConsistency( OSSFILE &in, INT64 &offset,
                         ciLinkList<ciNode> &nodes,
                         const ciClHeader &header,
                         ciLinkList< ciRecord > &records )
{
   INT32 rc = SDB_OK ;
   UINT32 index = 0 ;
   CHAR *bsonBuffer = NULL ;
   INT32 bufferLen = 0 ;

   PD_LOG( PDEVENT, "Begin processing consistency for collection: %s",
           header._fullname ) ;

   while ( index < header._recordCount )
   {
      INT32 recordLen = 0 ;
      CHAR state = 0 ;
      CHAR origState = 0 ;
      rc = readFromFile( in, offset, (CHAR *)&recordLen, sizeof(INT32) ) ;
      PD_RC_CHECK( rc, PDERROR, "Get record from file at offset[%lld] failed: "
                                "%d", offset, rc ) ;

      if ( recordLen > bufferLen )
      {
         bsonBuffer = (CHAR *)SDB_OSS_REALLOC( bsonBuffer, recordLen ) ;
         PD_CHECK( NULL != bsonBuffer, SDB_OOM, error, PDERROR,
                   "Allocate memory for record failed: %d", rc ) ;
         bufferLen = recordLen ;
      }

      // read the record from the file.
      rc = readFromFile( in, offset, (CHAR *)bsonBuffer, recordLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Read record from file at offset[%lld] failed: "
                                "%d", offset, rc) ;

      // read the state of this record from the file
      rc = readFromFile( in, offset, &state, sizeof(CHAR) ) ;
      PD_RC_CHECK( rc, PDERROR, "Read record status from file at offset[%lld] "
                                "failed: %d", offset, rc ) ;

      rc = readFromFile( in, offset, &origState, sizeof(CHAR) ) ;
      PD_RC_CHECK( rc, PDERROR, "Read record original status from file at "
                                "offset[%lld] failed: %d", offset, rc ) ;

      // Query the different OID on all nodes in one group. If it's not there,
      // insert it.
      BSONObj obj( bsonBuffer ) ;
      BSONElement e ;
      obj.getObjectID( e ) ;
      BSONObj queryCond = bob().append( e ).obj() ;
      ciState origStateObj( origState ) ;

      ciLinkList<ciCursor> cursors ;
      rc = getCiCursor( nodes, header._fullname, cursors, queryCond ) ;
      PD_RC_CHECK( rc, PDERROR, "Get cursors failed: %d", rc ) ;

      BSONObj record ;     // Hold the full record from server.
      BOOLEAN existOnMaster = TRUE ;
      vector<sdbclient::sdb *> pendingDBs ;  // Nodes who dose not have
                                             // the record.
      for ( ciCursor *cursor = cursors.getHead(); cursor != NULL;
            cursor = cursors.next() )
      {
         BSONObj tmpRecord ;
         rc = cursor->_cursor->next( tmpRecord ) ;
         if ( SDB_DMS_EOC == rc )
         {
            pendingDBs.push_back( cursor->_db ) ;
            if ( cursor == cursors.getHead() )
            {
               existOnMaster = FALSE ;
            }
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            // In case of other unexpected error, just skip this record this
            // time.
            break ;
         }
         else if ( record.isEmpty() )
         {
            record = tmpRecord ;
         }
      }

      if ( SDB_OK == rc && !record.isEmpty() && pendingDBs.size() > 0 )
      {
         // There are two scenarios that we need to repair some node:
         // (1) The primary node always has this record, so other pending nodes
         //     may have lost the record.
         // (2) The primary node dosen't have the record for a long time(from
         //     the first round of inspecting). If some one else dose, then
         //     maybe it's the primary node who has lost the record.
         if ( ( origStateObj.hit(0) && existOnMaster ) ||
              ( !( origStateObj.hit(0) || existOnMaster ) ) )
         {
            // Insert on pending nodes.
            for ( vector<sdb*>::iterator itr = pendingDBs.begin();
                  itr != pendingDBs.end(); ++itr )
            {
               sdbCollection cl ;
               rc = (*itr)->getCollection( header._fullname, cl ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Get collection handle for[%] failed: %d",
                          header._fullname, rc ) ;
                  continue ;
               }
               // Don't want to log in the log file. Just ignore.
               (void)cl.insert( record ) ;
            }
         }
      }

      {
         ciRecord *record = records.createNode() ;
         PD_CHECK( NULL != record, SDB_OOM, error, PDERROR,
                   "Allocate memory for record failed: %d", rc ) ;
         // Repaire the record, but don't change the status. Wait for next
         // inspect.
         record->_bson = obj.copy() ;
         record->_len = obj.objsize() ;
         record->_state = state ;
         record->_origState = origState ;
         records.add( record ) ;
      }
      ++index ;
   }

done:
   if ( bsonBuffer )
   {
      SDB_OSS_FREE( bsonBuffer ) ;
   }
   return rc ;
error:
   goto done ;
}

/**
** copy record data to buffer
***/
INT32 ciRecordToBuffer( ciLinkList< ciRecord > &records, CHAR *&buffer,
                        INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc            = SDB_OK ;
   INT64 pos           = 0 ;
   ciRecord *curRecord = NULL ;

   validSize = 0 ;

   // Calculate the total size of all the records.
   records.resetCurrentNode() ;
   curRecord = records.getHead() ;
   while ( NULL != curRecord )
   {
      validSize += sizeof( INT32 ) ;
      validSize += curRecord->_len ;
      validSize += ( sizeof( CHAR ) * 2 ) ;  // For _state and _origState.
      curRecord = records.next() ;
   }

   if ( validSize > bufferSize )
   {
      buffer = ( CHAR * )SDB_OSS_REALLOC( buffer, validSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer. size = "
                   << validSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = validSize ;
   }

   records.resetCurrentNode() ;
   curRecord = records.getHead() ;
   while ( NULL != curRecord )
   {
      ossMemcpy( buffer + pos, &curRecord->_len, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;
      INT32 size = curRecord->_bson.objsize() ;
      ossMemcpy( buffer + pos, curRecord->_bson.objdata(), size ) ;
      pos += size ;
      ossMemcpy( buffer + pos, &curRecord->_state, sizeof( CHAR ) ) ;
      pos += 1 ;
      ossMemcpy( buffer + pos, &curRecord->_origState, sizeof( CHAR ) ) ;
      pos += 1 ;

      curRecord = records.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciRecord to file
***/
INT32 writeCiRecord( OSSFILE &out, ciLinkList< ciRecord > &records,
                     CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;

   rc = ciRecordToBuffer( records, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciOffset from file
***/
INT32 readCiOffset( OSSFILE &in, INT64 &offset,
                    const INT32 count, ciLinkList< ciOffset > &clo )
{
   INT32 rc = SDB_OK ;
   INT32 idx = 0 ;
   INT32 pos = 0 ;
   CHAR *buffer = NULL ;

   INT32 readSize = count * sizeof( INT64 ) ;
   buffer = ( CHAR * )SDB_OSS_MALLOC( readSize ) ;
   if ( NULL == buffer )
   {
      std::cout << "Error: failed to allocate memory. size = "
                << readSize << std::endl ;
      rc = SDB_OOM ;
      goto error ;
   }

   rc = readFromFile( in, offset, buffer, readSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   while ( idx < count )
   {
      ciOffset *clOff = clo.createNode() ;
      if ( NULL == clOff )
      {
         std::cout << "Error: failed to allocate ciClOffset" << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( &clOff->_offset, buffer + pos, sizeof( INT64 ) ) ;
      pos += sizeof( INT64 ) ;

      clo.add( clOff ) ;
      ++idx ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** copy ciClOffset data to buffer
***/
INT32 ciOffsetToBuffer( ciLinkList< ciOffset > &offsets,
                        CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc            = SDB_OK ;
   INT64 pos           = 0 ;
   ciOffset *curNode = NULL ;
   UINT32 count        = offsets.count() ;
   INT32 unitLen       = sizeof( INT64 ) ;

   validSize = count * unitLen ;

   if ( validSize > bufferSize )
   {
      buffer = ( CHAR * )SDB_OSS_REALLOC( buffer, validSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer. size = "
                   << validSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = validSize ;
   }

   //ossMemcpy( buffer + pos, &count, sizeof( INT32 ) ) ;
   //pos += sizeof( INT32 ) ;
   offsets.resetCurrentNode() ;
   curNode = offsets.getHead() ;
   while ( NULL != curNode )
   {
      ossMemcpy( buffer + pos, &curNode->_offset, sizeof( INT64 ) ) ;
      pos += sizeof( INT64 ) ;
      curNode = offsets.next() ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciClOffset to file
***/
INT32 writeCiClOffset( OSSFILE &out, ciLinkList< ciOffset > &offsets,
                       CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;

   rc = ciOffsetToBuffer( offsets, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 readMainSubCl( OSSFILE &in, INT64 &offset,
                     const INT32 count, mainCl &mainCls )
{
   INT32 rc     = SDB_OK ;
   INT32 idx    = 0 ;
   INT32 subCount  = 0 ;
   CHAR buffer[ CI_CL_FULLNAME_SIZE + 1 ] = { 0 } ;

   mainCls.clear() ;

   while ( idx < count )
   {
      rc = readFromFile( in, offset, buffer, CI_CL_FULLNAME_SIZE ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      std::string mainClName = std::string( buffer ) ;
      rc = readFromFile( in, offset, (CHAR *)&subCount, sizeof( INT32 ) ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      INT32 subIdx = 0 ;
      while( subIdx < subCount )
      {
         rc = readFromFile( in, offset, buffer, CI_CL_FULLNAME_SIZE ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         std::string subClName = std::string( buffer ) ;
         subCl &subCls = mainCls[ mainClName ] ;
         subCls.push_back( subClName ) ;

         ++subIdx ;
      }
      ++idx ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}
INT32 writeMainSubCl( OSSFILE &out, const mainCl &mainCls, CHAR *&buffer,
                      INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc       = SDB_OK ;
   INT64 pos      = 0 ;
   INT64 totalLen = 0 ;

   mainCl::const_iterator it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      totalLen += CI_CL_FULLNAME_SIZE ;
      totalLen += sizeof( INT32 ) ;

      subCl::const_iterator sub_it = it->second.begin() ;
      for ( ; sub_it != it->second.end() ; ++sub_it )
      {
         totalLen += CI_CL_FULLNAME_SIZE ;
      }
   }

   validSize = totalLen ;

   if ( validSize > bufferSize )
   {
      buffer = ( CHAR * )SDB_OSS_REALLOC( buffer, validSize ) ;
      if ( NULL == buffer )
      {
         std::cout << "Error: failed to allocate buffer. size = "
            << validSize << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = validSize ;
   }

   it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      ossMemcpy( buffer + pos, it->first.c_str(), CI_CL_FULLNAME_SIZE ) ;
      pos += CI_CL_FULLNAME_SIZE ;

      INT32 count = it->second.size() ;
      ossMemcpy( buffer + pos, ( CHAR * )&count, sizeof( INT32 ) ) ;
      pos += sizeof( INT32 ) ;

      subCl::const_iterator sub_it = it->second.begin() ;
      for ( ; sub_it != it->second.end() ; ++sub_it )
      {
         ossMemcpy( buffer + pos, sub_it->c_str(), CI_CL_FULLNAME_SIZE ) ;
         pos += CI_CL_FULLNAME_SIZE ;
      }
   }

   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** read ciTail from file
***/
INT32 readCiTail( OSSFILE &in, const INT64 offset, ciTail *tail )
{
   INT32 rc        = SDB_OK ;
   INT64 tmpOffset = offset ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_exitCode, sizeof( INT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_groupCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_clCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_diffCLCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_mainClCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_recordCount, sizeof( INT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readFromFile( in, tmpOffset,
                      ( CHAR * )&tail->_timeCount, sizeof( UINT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readMainSubCl( in, tmpOffset, tail->_mainClCount, tail->_mainCls ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = readCiOffset( in, tmpOffset, tail->_groupCount, tail->_groupOffset );
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** write ciTail to file
***/
INT32 writeCiTail( OSSFILE &out, ciTail *tail,
                   CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc = SDB_OK ;
   INT32 len = 0 ;

   SDB_ASSERT( ( tail->_groupCount == tail->_groupOffset.count() ),
      "count of group is valid" ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_exitCode, sizeof( INT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( INT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_groupCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_clCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_diffCLCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_mainClCount, sizeof( UINT32 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT32 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_recordCount, sizeof( INT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( INT64 ) ;

   rc = writeToFile( out,
                     ( const CHAR * )&tail->_timeCount, sizeof( UINT64 ) ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += sizeof( UINT64 ) ;

   rc = writeMainSubCl( out, tail->_mainCls,
                        buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   len += validSize ;

   rc = writeCiClOffset( out, tail->_groupOffset,
                         buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   validSize += len ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 dumpOneCl( OSSFILE &in, OSSFILE &out, ciOffset *groupOffset,
                 ciLinkList< ciOffset > &clOffsets, const CHAR *clName,
                 CHAR *&buffer, INT64 &bufferSize, INT64 &validSize )
{
   INT32 rc     = SDB_OK ;
   UINT32 idx   = 0 ;
   INT64 offset = 0 ;
   ciGroupHeader header ;
   ciLinkList< ciNode > nodes ;

   if ( NULL == groupOffset )
   {
      goto done ;
   }

   offset = groupOffset->_offset ;
   // read group
   rc = readCiGroupHeader( in, offset, &header ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // read global nodes info
   nodes.clear() ;
   rc = readCiNode( in, offset, header, nodes ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   while ( idx < header._clCount )
   {
      ciClHeader clHeader ;
      ciLinkList< ciRecord > records ;
      ciLinkList< ciNode > nodesForCL ;

      if ( !findCiOffset ( clOffsets, offset ) )
      {
         INT64 clOffset = offset ;
         rc = readCiClHeader( in, offset, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciClHeader" << std::endl ;
            goto error ;
         }

         rc = readCiNode( in, offset, header, nodesForCL ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciNode" << std::endl ;
            goto error ;
         }

         if ( NULL == clName || 0 == ossStrncmp( clName, clHeader._fullname,
                                                 CI_CL_NAME_SIZE ) )
         {
            // find and remember the offset
            ciOffset *cl = clOffsets.createNode() ;
            if ( NULL == cl )
            {
               std::cout << "Error: failed to allocate ciOffset"
                  << std::endl;
               rc = SDB_OOM ;
               goto error ;
            }
            cl->_offset = clOffset ;
            clOffsets.add( cl ) ;

            // dump
            rc = dumpCiClHeader( &clHeader, buffer, bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = writeToFile( out, buffer, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            // dump group and node, if view option is "collection"
            // dump group
            rc = dumpCiGroupHeader( &header, buffer, bufferSize, validSize );
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = writeToFile( out, buffer, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            // dump nodes
            rc = dumpCiNode( nodesForCL, buffer, bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = writeToFile( out, buffer, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            if ( clHeader._recordCount > 0 )
            {
               records.clear() ;

               rc = readCiRecord( in, offset, nodesForCL,
                                  clHeader, records, TRUE ) ;
               CHECK_VALUE( ( SDB_OK != rc ), error ) ;
               rc = dumpCiRecord( nodesForCL, records,
                                  buffer, bufferSize, validSize ) ;
               CHECK_VALUE( ( SDB_OK != rc ), error ) ;
               rc = writeToFile( out, buffer, validSize ) ;
               CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            }

            rc = dumpOneCl( in, out, groupOffset->_next, clOffsets,
                            clHeader._fullname, buffer,
                            bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            // collection found and dumped, then exit
            //goto done ;
         }
      }

      ++idx ;
   }
done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** initialize header with file specified
***/
INT32 initialize( ciHeader *header )
{
   INT32 rc                            = SDB_OK ;
   INT64 fileSize                      = 0 ;
   BOOLEAN opened                      = FALSE ;
   OSSFILE file ;
   ciHeader oldheader ;

   rc = ossOpen( header->_filepath, OSS_READONLY, OSS_RU, file ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file specified" << std::endl ;
      goto error ;
   }
   opened = TRUE ;

   rc = ossGetFileSize( &file, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: fail to get file size" << std::endl ;
      goto error ;
   }

   if ( SDB_OK != rc )
   {
      std::cout << "Error: file size is less than " << CI_HEADER_SIZE
                << std::endl ;
      goto error ;
   }

   rc = readCiHeader( file, &oldheader ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // skip eye catcher
   // skip main version
   // skip sub version
   // copy loop
   ossMemcpy( &header->_loop, &oldheader._loop, sizeof(INT32) ) ;
   // copy actions
   ossMemcpy( header->_action, oldheader._action, CI_ACTION_SIZE ) ;
   // copy coord hostname
   ossMemcpy( header->_coordAddr,
              oldheader._coordAddr, CI_HOSTNAME_SIZE + 1 ) ;
   // copy coord service name
   ossMemcpy( header->_serviceName,
              oldheader._serviceName, CI_SERVICENAME_SIZE + 1 ) ;
   // copy user name and password
   //ossMemcpy( header->_user, oldheader._user, CI_USERNAME_SIZE + 1 ) ;
   //ossMemcpy( header->_psw, oldheader._psw, CI_PASSWD_SIZE + 1 ) ;

   // copy group name
   ossMemcpy( header->_groupName,
              oldheader._groupName, CI_GROUPNAME_SIZE + 1 ) ;
   // copy collection space name
   ossMemcpy( header->_csName, oldheader._csName, CI_CS_NAME_SIZE + 1 ) ;
   // copy collection name
   ossMemcpy( header->_clName, oldheader._clName, CI_CL_NAME_SIZE + 1) ;
   // skip file path
   // skip out file
   // copy view format string
   ossMemcpy( header->_view, oldheader._view, CI_VIEWOPTION_SIZE + 1 ) ;

done:
   if ( opened )
   {
      ossClose( file ) ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
 * @brief Get all main and sub collections information by using snapshot.
 * @param coord Connection to coordinator.
 * @param mainCls [out] Main collections, with all their sub collections
 *        together.
 */
INT32 getMainAndSubCl( sdbclient::sdb *coord, mainCl &mainCls )
{
   INT32 rc = SDB_OK ;
   sdbclient::sdbCursor cursor ;
   bson::BSONObj record ;

   if ( NULL == coord )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !coord->isValid() )
   {
      rc = SDB_NETWORK ;
      goto error ;
   }

   rc = coord->getSnapshot( cursor, 8 ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   rc = cursor.next( record ) ;
   while ( SDB_DMS_EOC != rc )
   {
      if ( SDB_OK != rc )
      {
         std::cout << "Error: fail to get next record in cursor"
                   << std::endl ;
         goto error ;
      }
      else
      {
         bson::BSONElement mainCL ;
         mainCL = record.getField( "MainCLName" ) ;
         if ( !mainCL.eoo() )
         {
            std::string mainName = mainCL.String() ;
            bson::BSONElement subClName ;
            subClName = record.getField( "Name" ) ;
            std::string subName = subClName.String() ;
            subCl &subCls = mainCls[ mainName ] ;
            subCls.push_back( subName ) ;
         }
      }
      rc = cursor.next( record ) ;
   }

   rc = SDB_OK ;

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
 * @brief Get group list by 'listReplicaGroups' interface from coordinator.
 * @param coord Connection to coordinator.
 * @param groupName Group name specified by the user, or NULL.
 * @param groupList [out] The groups need to be checked.
 *
 * Get group list by 'listReplicaGroups' interface from coordinator. If group is
 * specified by the user, just get that group. Otherwise, get all groups except
 * "SYS" groups.
 */
INT32 getCiGroup( sdbclient::sdb *coord, const vector<string>& groupNames,
                  ciLinkList< ciGroup > &groupList )
{
   INT32 rc = SDB_OK ;

   BOOLEAN hasGroup = groupNames.size() > 0 ? TRUE : FALSE ;
   bson::BSONObj obj ;
   sdbclient::sdbCursor cursor ;

   if ( NULL == coord )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !coord->isValid() )
   {
      rc = SDB_NETWORK ;
      goto error ;
   }

   rc = coord->listReplicaGroups( cursor ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to list replica groups" << std::endl ;
      goto error ;
   }

   rc = cursor.next( obj ) ;
   while( SDB_DMS_EOC != rc )
   {
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get current record." << std::endl ;
         goto error ;
      }
      else
      {
         bson::BSONElement name = obj.getField( "GroupName" ) ;
         std::string beginWith = name.String().substr( 0, 3 ) ;
         if ( 0 != ossStrncmp( beginWith.c_str(),
                               "SYS", ossStrlen( "SYS") ) )
         {
            // If no group is specified, all data groups(except sys groups) will
            // be inspected.
            if ( !hasGroup || ( groupNames.end() != find( groupNames.begin(),
                                                          groupNames.end(),
                                                          name.String() ) ) )
            {
               ciGroup *group = groupList.createNode() ;
               if ( NULL == group )
               {
                  std::cout << "Error: failed to allocate ciGroup"
                            << std::endl ;
                  rc = SDB_OOM ;
                  goto error ;
               }

               ossMemcpy( group->_groupName, name.String().c_str(),
                          CI_GROUPNAME_SIZE ) ;
               group->_groupID = obj.getField( "GroupID" ).Int() ;

               groupList.add( group ) ;
            }
         }
      }
      rc = cursor.next( obj ) ;
   }

   if ( 0 >= groupList.count() )
   {
      std::cout << "Error: Cannot get replica group" << std::endl;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = SDB_OK ;
done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 getCompleteLSN( sdb &conn, UINT64 &completeLSN )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj result ;
   BSONObj cond = bob().append("CompleteLSN", "").obj() ;

   rc = conn.getSnapshot( cursor, SDB_SNAP_DATABASE, _sdbStaticObject, cond ) ;
   PD_RC_CHECK( rc, PDERROR, "Get snapshot database failed: %d", rc ) ;
   rc = cursor.next( result ) ;
   PD_RC_CHECK( rc, PDERROR, "Get snapshot database result from cursor "
                             "failed: %d", rc ) ;
   completeLSN = result.firstElement().Long() ;

done:
   cursor.close() ;
   return rc ;
error:
   goto done ;
}

/*
 * Get all nodes in the specified group. Master node is the first one in the
 * list.
 */
INT32 getCiNode( sdbclient::sdb *coord, ciGroup *group,
                 ciGroupHeader &header, ciLinkList<ciNode> &nodeList )
{
   INT32 rc = SDB_OK ;

   if ( NULL == coord )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL != group )
   {
      INT32 index = 0 ;
      // fill member of group
      header._groupID = group->_groupID ;
      ossMemset( header._groupName, 0, CI_GROUPNAME_SIZE ) ;
      ossMemcpy( header._groupName, group->_groupName, CI_GROUPNAME_SIZE ) ;

      // query replica group
      if ( !coord->isValid() )
      {
         rc = SDB_NETWORK ;
         goto error ;
      }

      sdbclient::sdbReplicaGroup rg ;
      rc = coord->getReplicaGroup( group->_groupName, rg ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get replica group: "
                   << group->_groupName << std::endl ;
         goto error ;
      }

      //get master node to make sure master is the head node of list
      sdbclient::sdbNode master ;
      rc = rg.getMaster( master ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get master node of group: "
                   << rg.getName() << std::endl ;
         goto error ;
      }

      ciNode *masterNode = nodeList.createNode() ;
      if ( NULL == masterNode )
      {
         std::cout << "Error: failed to allocate ciNode"
                   << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemcpy( masterNode->_hostname, master.getHostName(),
                 CI_HOSTNAME_SIZE ) ;
      ossMemcpy( masterNode->_serviceName, master.getServiceName(),
                 CI_SERVICENAME_SIZE ) ;
      ++index ;
      masterNode->_index = index ;
      nodeList.add( masterNode ) ;

      {
         sdb conn ;
         rc = master.connect( conn ) ;
         PD_RC_CHECK( rc, PDERROR, "Connect to master node[%s:%s] failed: %d",
                      masterNode->_hostname, masterNode->_serviceName, rc ) ;

         rc = getCompleteLSN( conn, header._maxCompleteLSN ) ;
         PD_RC_CHECK( rc, PDERROR, "Get completeLSN by snapshot for node[%s:%s]"
                                   " failed: %d",
                      masterNode->_hostname, masterNode->_serviceName, rc ) ;
      }

      // query slave nodes of group
      bson::BSONObj result ;
      rc = rg.getDetail( result ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to get detail of replica group: "
                   << rg.getName() << std::endl ;
         goto error ;
      }

      bson::BSONElement eGroup = result.getField("Group") ;
      if( bson::Array == eGroup.type() )
      {
         std::vector<bson::BSONElement> nodes = eGroup.Array() ;
         std::vector<bson::BSONElement>::iterator cit = nodes.begin() ;
         while ( nodes.end() != cit )
         {
            bson::BSONObj bsonNode ;
            cit->Val( bsonNode ) ;

            ciNode *node = nodeList.createNode() ;
            if ( NULL == node )
            {
               std::cout << "Error: failed to allocate ciNode" << std::endl ;
               rc = SDB_OOM ;
               goto error ;
            }
            // get hostname of node
            std::string hostname = bsonNode.getField( "HostName" ).String() ;
            // get servicename of node
            std::vector<bson::BSONElement> service ;
            service = bsonNode.getField( "Service" ).Array() ;
            std::string servicename = service[0][ "Name" ].String() ;
            INT32 nodeID = bsonNode.getField("NodeID").Int() ;

            if ( ( 0 == ossStrncmp( master.getHostName(),
                                    hostname.c_str(),
                                    CI_HOSTNAME_SIZE ) ) &&
               ( 0 == ossStrncmp( master.getServiceName(),
                                  servicename.c_str(),
                                  CI_SERVICENAME_SIZE ) ) )
            {
               masterNode->_nodeID = nodeID ;
               ++cit ;
               continue ;
            }

            // not master node
            ossMemcpy( node->_hostname, hostname.c_str(),
                       CI_HOSTNAME_SIZE ) ;
            ossMemcpy( node->_serviceName, servicename.c_str(),
                       CI_SERVICENAME_SIZE ) ;
            {
               sdbclient::sdb db ;
               rc = db.connect( node->_hostname, node->_serviceName,
                                g_username, g_password ) ;
               if ( SDB_OK != rc )
               {
                  node->_state = ciNode::STATE_DISCONN ;
                  rc = SDB_OK ;
               }
               else
               {
                  UINT64 completeLSN = 0 ;
                  rc = getCompleteLSN( db, completeLSN ) ;
                  PD_RC_CHECK( rc, PDERROR, "Get completeLSN by snapshot "
                                            "failed: %d", rc ) ;
                  if ( completeLSN > header._maxCompleteLSN )
                  {
                     header._maxCompleteLSN = completeLSN ;
                  }
               }
            }
            node->_nodeID = nodeID ;
            ++index ;
            node->_index = index ;
            // add to group
            nodeList.add( node ) ;
            ++cit ;
         }
         header._nodeCount = nodeList.count() ;
      }
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/*
 * Check if collection named subClName is a sub collection in the main
 * collection named mainClName.
 */
BOOLEAN isInMainSubCl( const CHAR *mainClName,
                       const CHAR *subClName, const mainCl &mainCls )
{
   BOOLEAN in = FALSE ;

   mainCl::const_iterator it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      if ( 0 == it->first.compare( mainClName ) )
      {
         subCl::const_iterator sub_it = it->second.begin() ;
         for ( ; sub_it != it->second.end() ; ++sub_it )
         {
            if ( 0 == sub_it->compare( subClName ) )
            {
               in = TRUE ;
               break ;
            }
         }
      }
   }

   return in ;
}

const CHAR* getMainClName( const mainCl &mainCls, const CHAR *subClName )
{
   const CHAR *pName = NULL ;
   mainCl::const_iterator it = mainCls.begin() ;
   for ( ; it != mainCls.end() ; ++it )
   {
      subCl::const_iterator sub_it = it->second.begin() ;
      for ( ; sub_it != it->second.end() ; ++sub_it )
      {
         if ( 0 == sub_it->compare( subClName ) )
         {
            pName = it->first.c_str() ;
            break ;
         }
      }
   }

   return pName ;
}

// Get all the collections we need to inspect from MASTER node.
// The flow is as follows:
// (1) Connect to master node directly, and list all the collections.
// (2) Tranverse all these collections to check if they are what we want:
//     a> If no collectionspace is specified, all collections need to be
//       inspected.
//     b> If cs is specified, but not the collection, all collections in the
//       matching collectionspace need to be inspected.
//     c> If cs and cl are specified, only this collection needs to be inspected.
INT32 getCiCollection( ciNode *master, const CHAR *groupName,
                       const CHAR *csName, const CHAR *clName,
                       ciLinkList< ciCollection > &collections,
                       const mainCl &mainCls )
{
   INT32 rc              = SDB_OK ;
   BOOLEAN hasCollection = FALSE ;  // Whether collection is specified.
   BOOLEAN hasCs         = FALSE ;  // Whether cs is specified.
   sdbclient::sdb db ;
   sdbclient::sdbCursor cursor ;
   bson::BSONObj collection ;
   CHAR fullName[ CI_CL_FULLNAME_SIZE + 1 ] = { 0 } ;

   SDB_ASSERT( NULL != master, "Error: master node cannot be NULL" ) ;

   hasCs = ( 0 != ossStrncmp( "", csName, CI_CS_NAME_SIZE ) ) ;
   hasCollection = ( 0 != ossStrncmp( "", clName, CI_CL_NAME_SIZE ) ) ;
   if ( hasCollection )
   {
      ossSnprintf( fullName, sizeof( fullName ), "%s.%s", csName, clName ) ;
   }

   // get collections from master node
   rc = db.connect( master->_hostname, master->_serviceName,
                    g_username, g_password ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to connect to master node: "
                << master->_hostname << ":"
                << master->_serviceName ;
      if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
      {
         std::cout << "user: " << g_username
                   << "   password: " << g_password;
      }
      std::cout << std::endl ;
      goto error ;
   }

   rc = db.listCollections( cursor ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to list collections" << std::endl ;
      goto error ;
   }

   // Check all the collections on master node one by one, to see it any of them
   // needs to be inspected according to the rule.
   rc = cursor.next( collection ) ;
   while ( SDB_DMS_EOC != rc )
   {
      if ( SDB_OK != rc )
      {
         std::cout << "Waring: failed to get record in cursor" << std::endl ;
         if ( collections.count() > 0)
         {
            // inspect with collections already exist.
            goto done ;
         }
         else
         {
            goto error ;
         }
      }
      else
      {
         std::string cs ;
         std::string cl ;
         BOOLEAN csMatch     = FALSE ;
         BOOLEAN allMatch    = FALSE ;  // Both cs and cl match the parameters.
         BOOLEAN inMainSubCl = FALSE ;
         std::string name    = collection.getField( "Name" ).String() ;
         std::size_t dot     = name.find( '.' ) ;
         if ( std::string::npos == dot )
         {
            std::cout << "Error: cannot split collection fullname: "
                      << name << std::endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         cs = name.substr( 0, dot ) ;
         cl = name.substr( dot + 1 ) ;
         if ( gTargetParser )
         {
            allMatch = gTargetParser->isTarget( groupName, name.c_str() ) ;
            if ( !allMatch )
            {
               rc = cursor.next( collection ) ;
               continue ;
            }
         }
         else
         {
            // no cl name input and cs name match
            csMatch = ( !hasCollection &&
                        ( 0 == ossStrncmp( csName, cs.c_str(),
                                           CI_CS_NAME_SIZE ) ) ) ;
            allMatch = ( hasCs && hasCollection &&
                         ( 0 == ossStrncmp( csName, cs.c_str(),
                                            CI_CS_NAME_SIZE ) ) &&
                         ( 0 == ossStrncmp( clName, cl.c_str(),
                                            CI_CL_NAME_SIZE ) ) ) ;
         }

         // The collection specified by the user may be a main collection.
         // Check if the collection referred by the cursor is a sub collection
         // of the main collection. If yes, the sub collection needs to be
         // inspected.
         inMainSubCl = ( hasCs && hasCollection &&
                         isInMainSubCl( fullName, name.c_str(), mainCls ) ) ;
         if ( !hasCs || csMatch || allMatch || inMainSubCl )
         {
            ciCollection *cl = collections.createNode() ;
            if ( NULL == cl )
            {
               std::cout << "Error: failed to allocate ciCollection"
                  << std::endl ;
               rc = SDB_OOM ;
               goto error ;
            }
            ossMemcpy( cl->_clName, name.c_str(), CI_CL_FULLNAME_SIZE ) ;
            if ( inMainSubCl )
            {
               ossMemcpy( cl->_mainClName, fullName, CI_CL_FULLNAME_SIZE ) ;
            }
            else
            {
               const CHAR *mainClName = getMainClName( mainCls, name.c_str() ) ;
               if ( NULL != mainClName )
               {
                  ossMemcpy( cl->_mainClName,
                             mainClName, CI_CL_FULLNAME_SIZE ) ;
               }
            }
            PD_LOG( PDDEBUG, "Add collection [%s] to process list",
                    cl->_clName ) ;
            collections.add( cl ) ;
         }
      }
      rc = cursor.next( collection ) ;
   }

   if ( SDB_DMS_EOC == rc )
   {
      rc = SDB_OK ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
 * @brief Whether hit the ends of all the cursors.
 * @param doc  Current record.
 * @param nodeCount Node number for comparison in the replica groups.
 * @return TRUE if all cursors hit the end.
 */
BOOLEAN reachEnd( const ciBson &doc, const INT32 nodeCount )
{
   BOOLEAN end = TRUE ;
   for ( INT32 idx = 0 ; idx < nodeCount ; ++idx )
   {
      if ( !doc.objs[idx].isEmpty() )
      {
         end = FALSE ;
         break;
      }
   }

   return end ;
}

// Objects(Actually two records) with the same fields and values are equal even
// their field orders are different.
BOOLEAN _objSortCmp( const BSONObj &left, const BSONObj &right )
{
   BOOLEAN equal = FALSE ;

   try
   {
      BSONObjIteratorSorted itrLeft( left ) ;
      BSONObjIteratorSorted itrRight( right ) ;

      while ( itrLeft.more() && itrRight.more() )
      {
         if ( itrLeft.next() == itrRight.next() )
         {
            continue ;
         }
         else
         {
            equal = FALSE ;
            goto done ;
         }
      }

      // Any one has more elements, they do not equal.
      equal = ( itrLeft.more() || itrRight.more() ) ? FALSE : TRUE ;
   }
   catch ( std::exception &e )
   {
      std::cerr << "Unexpected exception: " << e.what() << std::endl ;
   }
done:
   return equal ;
}

// Compare one group of records, each from one node in the replica group.
// obj is the record who has the smallest OID among them. All the records are
// compared with it one by one. If they are the same, the state bit according to
// the index will be set to 1. Otherwise, the state bit will remain as 0. In
// this case, we can use the state bits to known which ones are the same, and
// which ones are not.
BOOLEAN compare( ciLinkList< ciNode > &nodes,
                 const bson::BSONObj &obj,
                 ciState &state, ciBson &doc )
{
   BOOLEAN equal = FALSE ;
   INT32 nodeCount = nodes.count() ;
   nodes.resetCurrentNode() ;
   ciNode *node = nodes.getHead() ;
   BSONElement baseOIDEle ;
   state.reset() ;

   if ( gFastMode )
   {
      obj.getObjectID( baseOIDEle ) ;
   }

   for ( INT32 idx = 0 ; idx < nodeCount ; ++idx, node = nodes.next() )
   {
      // the collection exists but is not available in node,
      // we assume the record exists
      if ( ciNode::STATE_NORMAL != node->_state &&
           ciNode::STATE_CLNOTEXIST != node->_state )
      {
         state.set( idx ) ;
      }
      else if ( ciNode::STATE_NORMAL == node->_state )
      {
         if ( gFastMode )
         {
            // Only compare OID
            BSONElement oidEle ;
            if ( doc.objs[idx].getObjectID( oidEle ) &&
                 oidEle.valuesEqual( baseOIDEle ) )
            {
               state.set( idx ) ;
            }
         }
         else if ( doc.objs[idx].equal( obj ) ||
                   _objSortCmp( doc.objs[idx], obj ) )
         {
            state.set( idx ) ;
         }
      }
   }

   if ( state._state == ( ( 1 << nodeCount ) - 1 ) || 0 == state._state )
   {
      state.reset() ;
      equal = TRUE ;
      // for next round
      state.set( ALL_THE_SAME_BIT ) ;
   }

   return equal ;
}

/**
 * @brief Compare all records for one collection and get the different ones.
 * @param nodes  Nodes in the same replica groups.
 * @param cursors  cursors of the collection on each node.
 * @param records [out]  List of records which are not the same on the nodes. It
 *                       contains the whole record, and the state info(from
 *                       which we can know on which nodes it's different).
 */
INT32 getCiRecord( ciLinkList< ciNode > &nodes,
                   ciLinkList< ciCursor > &cursors,
                   ciLinkList< ciRecord > &records )
{
   INT32 rc        = SDB_OK ;
   BOOLEAN equal   = FALSE ;
   INT32 min       = 0 ;
   INT32 nodeCount = 0 ;
   INT64 counter   = 0 ;
   INT32 brakeTime = 0 ;
   INT32 brakeStep = 0 ;
   ciBson record ;
   ciState state ;
   state.reset() ;

   getBrakeOptions( brakeTime, brakeStep ) ;
   cursors.resetCurrentNode() ;

   nodeCount = cursors.count() ;
   // get first record and compare
   state.set( ALL_THE_SAME_BIT ) ;
   rc = getNext( state, cursors, record, FALSE ) ;
   while ( !reachEnd( record, nodeCount ) )
   {
      // brake here
      min = getMinObjectIndex( record, nodeCount ) ;
      equal = compare( nodes, record.objs[ min ], state, record ) ;
      if ( !equal )
      {
         ciRecord *rd = records.createNode() ;
         if ( NULL == rd )
         {
            std::cout << "Error: failed to allocate ciRecord"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }
         rd->_bson = record.objs[min].copy() ;
         rd->_state = state._state ;
         rd->_origState = state._state ;
         rd->_len = rd->_bson.objsize() ;
         records.add( rd ) ;
      }

      // Speed controller
      ++counter ;
      if ( brakeTime > 0 && brakeStep > 0 &&
           ( ( counter % brakeStep ) == 0 ) )
      {
         ossSleepmillis( brakeTime ) ;
      }
      if ( counter % 1000000 == 0 )
      {
         PD_LOG( PDDEBUG, "Processed number: %lld", counter ) ;
      }

      rc = getNext( state, cursors, record ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   }

done:
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

/**
** make tmpFile path from outFile
***/
void makeTmpFileName( const CHAR *outFile, UINT32 loopIndex, CHAR *tmpFile, UINT32 len )
{
   SDB_ASSERT( NULL != outFile && NULL != tmpFile, "outFile & tmpFile can't be NULL" ) ;

   ossMemset( tmpFile, 0, OSS_MAX_PATHSIZE ) ;
   ossSnprintf( tmpFile, len, "%s"CI_TMP_FILE_SUFFIX, outFile, loopIndex ) ;
}

/**
 * @brief Check max complete LSN of each group and write to the file.
 * @param out
 * @param tail
 * @return
 */
INT32 refreshGroupMaxCompleteLSN( sdbclient::sdb *coord,
                                  OSSFILE &out,
                                  const vector<string>& targetGroups,
                                  ciLinkList< ciGroup > &groupList,
                                  ciTail &tail )
{
   INT32 rc = SDB_OK ;
   ciGroup *curGroup = NULL ;
   ciGroupHeader groupHeader ;
   ciLinkList< ciNode > nodeList ;
   std::map<const CHAR *, UINT64, str_compare> groupLSNMap ;

   groupList.resetCurrentNode() ;
   curGroup = groupList.getHead() ;
   while ( curGroup )
   {
      if ( targetGroups.end() != find( targetGroups.begin(), targetGroups.end(),
                                       curGroup->_groupName ) )
      {
         rc = getCiNode( coord, curGroup, groupHeader, nodeList ) ;
         PD_RC_CHECK( rc, PDERROR, "Get node info for group[%s] failed: %d",
                      curGroup->_groupName, rc ) ;
         groupLSNMap[ curGroup->_groupName ] = groupHeader._maxCompleteLSN ;
      }
      curGroup = groupList.next() ;
   }

   // Traverse all the group header, and update the completeLSN.
   {
      ciOffset *offsetItem ;
      ciLinkList<ciOffset> *groupOffsets = &(tail._groupOffset) ;
      groupOffsets->resetCurrentNode() ;
      offsetItem = groupOffsets->getHead() ;
      while ( offsetItem )
      {
         ciGroupHeader header ;
         INT64 groupOffset = offsetItem->_offset ;
         // This function will change the second parameter! So don't pass
         // offsetItem->_offset.
         readCiGroupHeader( out, groupOffset, &header ) ;
         std::map<const CHAR *, UINT64, str_compare>::iterator itr =
               groupLSNMap.find( header._groupName ) ;
         if ( itr != groupLSNMap.end() )
         {
            header._maxCompleteLSN = itr->second ;
            ossSeek( &out, offsetItem->_offset, OSS_SEEK_SET ) ;
            writeCiGroupHeader( out, &header ) ;
            PD_LOG( PDDEBUG, "Update max complete LSN for group %s to %lld",
                    header._groupName, itr->second ) ;
         }

         offsetItem = groupOffsets->next() ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/**
 * @brief Inspect the targets using a complete flow. Without file means without
 *        any intermedia files.
 * @param coord  Connection to coordinator.
 * @param header Header to write into file.
 * @param outFile Output file name.
 * @param count [out] Total different record number.
 *
 * The structure of the intermediate file is as follows:
 *                 _________________________________
 *                |           file header           |
 *                |---------------------------------|
 *                |          group1 header          |
 *                |---------------------------------|
 *                |           node11 info           |
 *                |           node12 info           |
 *                |           node13 info           |
 *                |---------------------------------|
 *                |          diff record1           |
 *                |          diff record1           |
 *                |          diff record1           |
 *                |          diff record1           |
 *                |          diff record1           |
 *                |---------------------------------|
 *                |          group2 header          |
 *                |---------------------------------|
 *                |           node21 info           |
 *                |           node22 info           |
 *                |           node23 info           |
 *                |---------------------------------|
 *                |          diff record1           |
 *                |          diff record1           |
 *                |          diff record1           |
 *                |          diff record1           |
 *                |          diff record1           |
 *                |---------------------------------|
 *                |                                 |
 *                |               ...               |
 *                |                                 |
 *                |---------------------------------|
 *                |           file tail             |
 *                |       maintain offset of        |
 *                |       each group header         |
 *                |_________________________________|
 *
 */
INT32 inspectWithoutFile( _IParam *options, sdbclient::sdb *coord,
                          ciHeader *header, const CHAR *outFile,
                          UINT64 &count )
{
   INT32 rc                                 = SDB_OK ;
   BOOLEAN hasGroup                         = FALSE ;
   BOOLEAN opened                           = FALSE ;
   ciGroup *curGroup                        = NULL ;
   ciCollection *curCollection              = NULL ;
   CHAR *buffer                             = NULL ;
   INT64 offset                             = 0 ;  // Write position in the
                                                   // output file.
   INT64 bufferSize                         = 0 ;
   INT64 validSize                          = 0 ;
   ciLinkList< ciGroup > groupList ;
   ciLinkList< ciNode > nodeList ;
   ciLinkList< ciCollection > collections ;
   ciLinkList< ciCursor > cursors ;
   ciLinkList< ciRecord > records ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciTail tail ;
   OSSFILE file ;
   ossTimestamp beginTime ;
   ossTimestamp endTime ;
   vector<string> groupNames ;   // Target group names.

   ossGetCurrentTime( beginTime ) ;
   PD_LOG( PDEVENT, "Begin inspect without any existing file..." ) ;

   count = 0 ;
   rc = ossOpen( outFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, file ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file, rc = " << rc << std::endl ;
      goto error ;
   }
   opened = TRUE ;

   // write header to file
   rc = writeCiHeader( file, header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   offset += validSize ;

   if ( gTargetParser )
   {
      // Loop and get all groups.
      groupNames = gTargetParser->getGroups() ;
   }
   else if ( ossStrlen( header->_groupName ) > 0 )
   {
      groupNames.push_back( header->_groupName ) ;
   }

   rc = getCiGroup( coord, groupNames, groupList ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // Get all main and sub collections information.
   rc = getMainAndSubCl( coord, tail._mainCls ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tail._mainClCount = tail._mainCls.size() ;

   hasGroup = groupNames.size() > 0 ? TRUE : FALSE ;

   // Inspect on all the target groups one by one. And on each group, inspect
   // all the target collections. Only collections which are available on the
   // group master node will be inspected.
   groupList.resetCurrentNode() ;
   curGroup = groupList.getHead() ;
   while( NULL != curGroup )
   {
      BOOLEAN match = FALSE ;
      // Only when no group/cs/cl is specified that the gTargetParser will be
      // initialized.
      if ( gTargetParser )
      {
         match = gTargetParser->isTarget( curGroup->_groupName ) ;
      }
      else if ( !hasGroup || 0 == ossStrncmp( curGroup->_groupName,
                                              header->_groupName,
                                              CI_GROUPNAME_SIZE ) )
      {
         match = TRUE ;
      }

      if ( match )
      {
         PD_LOG( PDEVENT, "Begin to inspect group %s...",
                 curGroup->_groupName ) ;

         nodeList.clear() ;
         collections.clear() ;

         // Get all nodes in the current group.
         rc = getCiNode( coord, curGroup, groupHeader, nodeList ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         // Get the list of collections which need to be inspected on this
         // group.
         rc = getCiCollection( nodeList.getHead(), curGroup->_groupName,
                               header->_csName, header->_clName, collections,
                               tail._mainCls ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         groupHeader._clCount = collections.count() ;
         tail._clCount += collections.count() ;

         // remember the offset
         ciOffset *off = tail._groupOffset.createNode() ;
         if ( NULL == off )
         {
            std::cout << "Error: failed to allocate ciClOffset"
                      << std::endl ;
            rc = SDB_OOM ;
            goto error ;
         }

         // Remember the offset of the beginning of the group header in the
         // tail.
         off->_offset = offset ;
         tail._groupOffset.add( off ) ;
         ++tail._groupCount ;

         // write group header to file
         rc = writeCiGroupHeader( file, &groupHeader ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         offset += CI_GROUP_HEADER_SIZE ;

         curCollection = collections.getHead() ;

         // write the global nodes info
         rc = writeCiNode( file, nodeList, buffer, bufferSize, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         offset += validSize ;

         // Inspect all target collections in this group.
         // One round of loop processes one collection. This is single thread,
         while ( NULL != curCollection )
         {
            PD_LOG( PDEVENT, "Begin to inspect collection %s on group %s",
                    curCollection->_clName, curGroup->_groupName ) ;
            cursors.clear() ;
            bson::BSONObj order = bob().append("_id", 1).obj();
            bson::BSONObj selector = sdbclient::_sdbStaticObject ;
            if ( gFastMode )
            {
               selector = bob().append("_id", "").obj() ;
            }
            rc = getCiCursor( nodeList, curCollection->_clName,
                              cursors, order, TRUE, selector ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;

            ossMemset( clHeader._fullname, 0, CI_CL_FULLNAME_SIZE ) ;
            ossMemcpy( clHeader._fullname, curCollection->_clName,
                       CI_CL_FULLNAME_SIZE ) ;
            ossMemcpy( clHeader._mainClName, curCollection->_mainClName,
                       CI_CL_FULLNAME_SIZE ) ;

            records.clear() ;
            // Get all inconsistent records for the collection.
            rc = getCiRecord( nodeList, cursors, records ) ;
            if ( SDB_OK != rc )
            {
               std::cout << "Error: failed to re record among nodes"
                         << std::endl ;
               goto error ;
            }

            clHeader._recordCount = records.count() ;
            tail._recordCount += records.count() ;
            if ( !normalNodes( nodeList ) || records.count() > 0 )
            {
               tail._diffCLCount++ ;
            }

            // 1. write collection header
            rc = writeCiClHeader( file, &clHeader ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            offset += CI_CL_HEADER_SIZE ;

            // 2. write nodes info
            // write the nodes info from perspective of each collection
            rc = writeCiNode( file, nodeList, buffer, bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            offset += validSize ;

            // 3. write diff-records info
            if ( clHeader._recordCount > 0 )
            {
               count += clHeader._recordCount ;
               rc = writeCiRecord( file, records, buffer,
                                   bufferSize, validSize ) ;
               CHECK_VALUE( ( SDB_OK != rc ), error ) ;
               offset += validSize ;
            }

            PD_LOG( PDEVENT, "Inspect collection %s on group %s done",
                    curCollection->_clName, curGroup->_groupName ) ;
            curCollection = collections.next() ;
         }
      }

      curGroup = groupList.next() ;
   }

   {
      //BOOLEAN hasCS = ( 0 != ossStrncmp( "", header->_csName,
      //                                       CI_CS_NAME_SIZE ) ) ;
      BOOLEAN hasCL = ( 0 != ossStrncmp( "", header->_clName,
         CI_CL_NAME_SIZE ) ) ;
      if ( 0 >= tail._clCount )
      {
         std::cout << "Not found any "
                   << ( hasCL ? "match " : "" )
                   << "collection" << std::endl ;
         rc = CI_INSPECT_CL_NOT_FOUND ;
         goto done ;
      }
   }

   ossGetCurrentTime( endTime ) ;
   {
      UINT64 begin = beginTime.time * 1000000 + beginTime.microtm ;
      UINT64 end   = endTime.time * 1000000 + endTime.microtm ;
      tail._timeCount += ( end - begin ) / 1000 ;
   }

   if ( count == 0 )
   {
      tail._exitCode = 0 ; // no records
   }
   else
   {
      tail._exitCode = 2 ;// loop count over
   }

   // append tail
   rc = writeCiTail( file, &tail, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   //offset += validSize ;

   // remember the tail size
   header->_tailSize = validSize ;
   // update file header to file
   rc = writeCiHeader( file, header, buffer, bufferSize, validSize, TRUE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // Update all group max completeLSN after compare all records.
   rc = refreshGroupMaxCompleteLSN( coord, file, groupNames, groupList, tail ) ;
   PD_RC_CHECK( rc, PDERROR, "Refresh group max complete LSN failed: %d", rc ) ;

done:
   // close file
   if ( opened )
   {
      ossClose( file ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}


INT32 ensureNodeConnection( ciNode &node, BOOLEAN reconnect = FALSE )
{
   INT32 rc = SDB_OK ;
   sdb *db = node._db ;
   if ( !db )
   {
      db = new sdb() ;
      PD_CHECK( db, SDB_OOM, error, PDERROR,
                "Allocate memory for node connection failed: %d", rc ) ;
      node._db = db ;
      rc = db->connect( node._hostname, node._serviceName,
                        g_username, g_password ) ;
      if ( rc )
      {
         // In case of connection error, keep the connection object.
         node._state = ciNode::STATE_DISCONN ;
         PD_LOG( PDERROR, "Connect to node[%s:%s] failed: %d",
                 node._hostname, node._serviceName, rc ) ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/**
 * @brief Whether all nodes have reached or exceeded the max complete LSN
 *        recorded in the first round. This is to avoid repair normal records
 *        which have not been synchronized between nodes.
 * @param maxCompleteLSN  Maximum complete LSN of nodes in this group in the
 *                        first round.
 * @param nodes           Nodes to be checked.
 * @return Return true if all nodes are checked correctly and all their complete
 *         LSN match the condition.
 *
 * In case of any error, such as node down, query exception, need to return
 * false.
 */
BOOLEAN allNodesCatchUp( UINT64 maxCompleteLSN,
                         ciLinkList<ciNode>& nodes )
{
   INT32 rc = SDB_OK ;
   BOOLEAN result = FALSE ;
   ciNode *node = NULL ;

   if ( ( 0 == maxCompleteLSN ) || ( nodes.count() == 0 ) )
   {
      goto done ;
   }

   nodes.resetCurrentNode() ;
   node = nodes.getHead() ;
   while ( NULL != node )
   {
      UINT64 completeLSN = 0 ;
      rc = ensureNodeConnection( *node ) ;
      PD_RC_CHECK( rc, PDERROR, "Connection error: %d", rc ) ;

      rc = getCompleteLSN( *node->_db, completeLSN ) ;
      PD_RC_CHECK( rc, PDERROR, "Get completeLSN failed: %d", rc ) ;
      if ( completeLSN < maxCompleteLSN )
      {
         PD_LOG( PDWARNING, "Complete LSN of node[%s:%s] is %llu. Start point "
                            "is %llu. Collections on this group will not be "
                            "repaired in this round. Wait for next round to "
                            "check",
                            node->_hostname, node->_serviceName, completeLSN,
                            maxCompleteLSN ) ;
         result = FALSE ;
         goto done ;
      }
      node = nodes.next() ;
   }
   result = TRUE ;

done:
   return result ;
error:
   goto done ;
}

/**
** inspect node with file specified
***/
INT32 inspectWithFile( ciHeader *header, const CHAR *inFile,
                       const CHAR *outFile, UINT64 &count, BOOLEAN &finish,
                       BOOLEAN tryToRepair = FALSE )
{
   INT32 rc           = SDB_OK ;
   BOOLEAN inOpened   = FALSE ;
   BOOLEAN outOpened  = FALSE ;
   CHAR *buffer       = NULL ;
   INT64 bufferSize   = 0 ;
   INT64 validSize    = 0 ;
   UINT64 totalRecord = 0 ;
   INT64 fileSize     = 0 ;
   INT64 offset       = 0 ;
   INT64 tailOffset   = 0 ;
   INT64 writeOffset  = 0 ;
   ciLinkList< ciNode > ciNodes ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciHeader tmpHeader;
   ciTail tail ;
   OSSFILE in ;
   OSSFILE out ;
   ossTimestamp beginTime ;
   ossTimestamp endTime ;

   PD_LOG( PDDEBUG, "Begin inspect with existing file: %s", inFile ) ;

   ossGetCurrentTime( beginTime ) ;

   // open in file
   rc = ossOpen( inFile, OSS_RO, OSS_RU | OSS_RG, in ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << inFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   inOpened = TRUE ;
   // open out file
   rc = ossOpen( outFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, out ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << outFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   outOpened = TRUE ;

   rc = ossGetFileSize( &in, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to get file size" << std::endl ;
      goto error ;
   }

   if ( SDB_OK != rc )
   {
      std::cout << "Error: filesize is lt " << CI_HEADER_SIZE << std::endl ;
      goto error ;
   }

   rc = readCiHeader( in, &tmpHeader ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   tailOffset = fileSize - tmpHeader._tailSize ;
   rc = readCiTail( in, tailOffset, &tail ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tail._groupCount = 0 ;
   tail._groupOffset.clear() ;
   tail._recordCount = 0 ;
   tail._diffCLCount = 0 ;

   rc = writeCiHeader( out, header, buffer, bufferSize, validSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to write header to file" << std::endl ;
      goto error ;
   }
   writeOffset = CI_HEADER_SIZE ;

   //skip 65536 bytes
   offset = CI_HEADER_SIZE ;
   while ( offset < tailOffset )
   {
      rc = readCiGroupHeader( in, offset, &groupHeader ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // remember the offset
      ciOffset *off = tail._groupOffset.createNode() ;
      if ( NULL == off )
      {
         std::cout << "Error: failed to allocate ciOffset"
                   << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      off->_offset = writeOffset ;
      tail._groupOffset.add( off ) ;
      ++tail._groupCount ;

      // write to out file
      rc = writeCiGroupHeader( out, &groupHeader ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to write ciGroupHeader to file"
                   << std::endl ;
         goto error ;
      }

      writeOffset += CI_GROUP_HEADER_SIZE ;
      // read nodes
      ciNodes.clear() ;
      rc = readCiNode( in, offset, groupHeader, ciNodes ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // write the global nodes info
      rc = writeCiNode( out, ciNodes, buffer, bufferSize, validSize ) ;
      if ( SDB_OK != rc )
      {
         std::cout << "Error: failed to write ciNodes to file" << std::endl ;
         goto error ;
      }
      writeOffset += validSize ;

      UINT32 idx = 0 ;
      while ( idx < groupHeader._clCount )
      {
         ciClHeader clHeader ;
         ciLinkList< ciRecord > records ;
         ciLinkList< ciNode > nodesForEachCL ;

         // 1. read collection header
         rc = readCiClHeader( in, offset, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciClHeader" << std::endl ;
            goto error ;
         }

         // 2. read nodes-info for each collection
         rc = readCiNode( in, offset, groupHeader, nodesForEachCL ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciNode" << std::endl ;
            goto error ;
         }

         // 3. read diff-records info
         if ( clHeader._recordCount > 0 )
         {
            records.clear() ;

            // Only when the completeLSN of all the nodes in the group have
            // catched up with the original largest completeLSN, then we can
            // start repairing. Otherwise, just do inspect. All nodes should
            // be in normal status.
            if ( tryToRepair && allNodesCatchUp( groupHeader._maxCompleteLSN,
                                                 ciNodes ) )
            {
               rc = repairConsistency( in, offset, ciNodes, clHeader, records ) ;
            }
            else
            {
               rc = readCiRecord( in, offset, ciNodes, clHeader, records ) ;
            }
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         }

         // 4. count the final result
         clHeader._recordCount = records.count() ;
         tail._recordCount += records.count() ;
         if ( !normalNodes( ciNodes ) || records.count() > 0 )
         {
            tail._diffCLCount++ ;
         }

         // 5. write collection header
         rc = writeCiClHeader( out, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to write ciClHeader to file"
                      << std::endl ;
            goto error ;
         }
         writeOffset += CI_CL_HEADER_SIZE ;

         // 6. write nodes-info for each collection
         rc = writeCiNode( out, ciNodes, buffer, bufferSize, validSize ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to write ciNodes to file" << std::endl ;
            goto error ;
         }
         writeOffset += validSize ;

         // 7. write diff-records info
         if ( records.count() > 0 )
         {
            totalRecord += records.count() ;

            rc = writeCiRecord( out, records, buffer,
                                bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            writeOffset += validSize ;
         }
         ++idx ;
      }
   }

   ossGetCurrentTime( endTime ) ;
   {
      UINT64 begin = beginTime.time * 1000000 + beginTime.microtm ;
      UINT64 end   = endTime.time * 1000000 + endTime.microtm ;
      tail._timeCount += ( end - begin ) / 1000 ;
   }

   // tail
   if ( totalRecord == 0 )
   {
      finish = TRUE ;
      tail._exitCode = 0 ; // no records
   }
   else
   {
      double precent = ( totalRecord / (double)count ) * 100 ;
      if ( precent <= 1 )
      {
         finish = TRUE ;
         tail._exitCode = 1 ; // lt 1%
      }
   }

   if ( !finish )
   {
      count = totalRecord ;
      tail._exitCode = 2 ; // assume loop it over
   }

   // write header to file
   rc = writeCiTail( out, &tail, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // remember the tail size
   header->_tailSize = validSize ;
   // update file header to file
   rc = writeCiHeader( out, header, buffer, bufferSize, validSize, TRUE ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

done:
   if ( inOpened )
   {
      ossClose( in ) ;
   }

   if ( outOpened )
   {
      ossClose( out ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

const CHAR *_ciNode::stateDesc[ _ciNode::STATE_COUNT ] =
{
   "Normal",
   "Failed to connect to node",
   "Collection does not exist",
   "Failed to get the collection",
   "Failed to get cursor"
} ;

_sdbCi::_sdbCi()
: _repair( FALSE ) ,
  _repairRetryTimes( 0 ),
  _brakeTime(0 ),
  _brakeStep( 1000 )
{
   ossMemset( _coordAddr, 0, CI_ADDRESS_SIZE + 1 ) ;
   ossMemset( _auth, 0, CI_AUTH_SIZE + 1 ) ;
   ossMemset( _list, 0, CI_ARG_MAX_SIZE + 1 ) ;
   ossMemset( _listFile, 0, OSS_MAX_PATHSIZE + 1 ) ;
#ifdef _DEBUG
   ossMemset(_encodeFile, 0, OSS_MAX_PATHSIZE + 1 ) ;
   ossMemset(_decodeFile, 0, OSS_MAX_PATHSIZE + 1 ) ;
#endif
}

_sdbCi::~_sdbCi()
{
}

void _sdbCi::displayArgs( const po::options_description &desc )
{
   std::cout << desc << std::endl ;
}

INT32 _sdbCi::init( INT32 argc, CHAR **argv,
                    po::variables_map &vm )
{
   INT32 rc = SDB_OK ;
   po::options_description all( "Command options" ) ;
   po::options_description display( "Command options" ) ;

#ifdef _DEBUG
   INSPECT_ADD_OPTIONS_BEGIN( all )
         INSPECT_OPTIONS
         INSPECT_HIDDEN_OPTIONS
         INSPECT_HIDDEN_OPTIONS_DEBUG
         ( PMD_OPTION_HELPFULL, "help all configs" )
   INSPECT_ADD_OPTIONS_END
#else
   INSPECT_ADD_OPTIONS_BEGIN( all )
      INSPECT_OPTIONS
      INSPECT_HIDDEN_OPTIONS
      ( PMD_OPTION_HELPFULL, "help all configs" )
   INSPECT_ADD_OPTIONS_END
#endif

   INSPECT_ADD_OPTIONS_BEGIN( display )
      INSPECT_OPTIONS
   INSPECT_ADD_OPTIONS_END

   rc = utilReadCommandLine( argc, argv, all, vm ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Invalid parameters" << std::endl ;
      displayArgs( display ) ;
      goto error ;
   }

   if ( vm.empty() || vm.count( CONSISTENCY_INSPECT_HELP ) )
   {
      std::cout << display << std::endl ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }
   else if ( vm.count( CONSISTENCY_INSPECT_HELPFULL ) )
   {
      std::cout << all << std::endl ;

   }

   if ( vm.count( CONSISTENCY_INSPECT_VER ) )
   {
      ossPrintVersion( "sdbinspect version" ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }

   PD_LOG( PDEVENT, "Start sdbinspect" ) ;

   rc = _pmdCfgRecord::init( NULL, &vm ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Invalid parameters" << std::endl ;
      displayArgs( display ) ;
      goto error ;
   }

#ifdef _DEBUG
   if ( vm.count( CONSISTENCY_INSPECT_ENCODE ) )
   {
      const CHAR *outFileName = vm.count( CONSISTENCY_INSPECT_OUTPUT ) ?
                                _header._outfile : INSPECT_DEFAULT_BFILE_NAME ;
      rc = gFileGuard.encrypt( _encodeFile, outFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Encrypt file[%s] failed: %d", _encodeFile, rc ) ;
         ossPrintf( "Encrypt file[%s] failed. Error(%d): %s"OSS_NEWLINE,
                    _encodeFile, rc, getErrDesp( rc ) ) ;
         goto error ;
      }
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if ( vm.count( CONSISTENCY_INSPECT_DECODE ) )
   {
      const CHAR *outFileName = vm.count( CONSISTENCY_INSPECT_OUTPUT ) ?
                                _header._outfile : INSPECT_DEFAULT_TFILE_NAME ;
      rc = gFileGuard.decrypt(_decodeFile, outFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Decrypt file[%s] failed: %d", _decodeFile, rc ) ;
         ossPrintf( "Decrypt file[%s] failed. Error(%d): %s"OSS_NEWLINE,
                    _decodeFile, rc, getErrDesp( rc ) ) ;
         goto error ;
      }
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }
#endif

   {
      BOOLEAN initParser = FALSE ;
      // Priority: -g/-c/-s > --list > --listfile
      if ( ossStrlen( _header._groupName ) > 0 ||
           ossStrlen( _header._csName ) > 0 ||
           ossStrlen( _header._clName ) > 0 )
      {
         ossMemset( _list, 0, sizeof( _list ) ) ;
         ossMemset( _listFile, 0, sizeof( _listFile ) ) ;
      }
      else if ( ossStrlen( _list ) > 0 )
      {
         ossMemset( _listFile, 0, sizeof( _listFile ) ) ;
         initParser = TRUE ;
      }
      else if ( ossStrlen( _listFile ) > 0 )
      {
         initParser = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "Inspect target is not specified" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( initParser )
      {
         gTargetParser = new inspectTargetParser() ;
         PD_CHECK( NULL != gTargetParser, SDB_OOM, error, PDERROR,
                   "Allocate memory for inspectTargetParser failed: %d", rc ) ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _sdbCi::handle( const po::variables_map &vm )
{
   INT32 rc = SDB_OK ;
   BOOLEAN byGroup = TRUE ;
   BOOLEAN useOutput = FALSE ;
   CHAR outReport[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   CHAR *tailBuffer = NULL ;
   INT64 tailBufferSize = 0 ;
   OSSFILE startupFile ;
   BOOLEAN startupFileOpened = FALSE ;
   BOOLEAN startupFileLocked = FALSE ;

   if ( 0 != ossStrncmp( CI_ACTION_INSPECT, _header._action, CI_ACTION_SIZE ) &&
        0 != ossStrncmp( CI_ACTION_REPORT, _header._action, CI_ACTION_SIZE ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "Unknown action : " << _header._action << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else if ( 0 == ossStrncmp( CI_ACTION_REPORT,
                              _header._action, CI_ACTION_SIZE ) &&
             !vm.count( CONSISTENCY_INSPECT_FILE ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "a existed file need to be specified "
                   "when ACTION is \"report\"" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if (    vm.count( CONSISTENCY_INSPECT_CL )
       && !vm.count( CONSISTENCY_INSPECT_CS ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "collection space name should be specified when collection "
                   "name is specified" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 0 != ossStrncmp( CI_VIEW_CL, _header._view, CI_VIEWOPTION_SIZE ) &&
        0 != ossStrncmp( CI_VIEW_GROUP, _header._view, CI_VIEWOPTION_SIZE ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "Unknown action : " << _header._action << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   byGroup = ( 0 != ossStrncmp( CI_VIEW_CL, _header._view,
                                CI_VIEWOPTION_SIZE ) ) ? TRUE : FALSE ;
   useOutput = ( 0 != ossStrncmp( "", _header._outfile, OSS_MAX_PATHSIZE ) ) ;

   if ( 0 == ossStrncmp( CI_ACTION_REPORT,
                         _header._action, CI_ACTION_SIZE ) &&
        vm.count( CONSISTENCY_INSPECT_FILE ) )
   {
      // report file
      if ( !useOutput )
      {
         ossMemcpy( outReport, _header._filepath, OSS_MAX_PATHSIZE ) ;
         ossStrncat( outReport, CI_FILE_REPORT, ossStrlen( CI_FILE_REPORT ) ) ;
      }
      else
      {
         ossMemcpy( outReport, _header._outfile, OSS_MAX_PATHSIZE ) ;
      }
      rc = byGroup ? report ( _header._filepath, outReport,
                              tailBuffer, tailBufferSize )
                   : report2( _header._filepath, outReport,
                              tailBuffer, tailBufferSize );
      //rc = report2( _header._filepath ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      std::cout << _header._action << " done" << std::endl ;
      std::cout << tailBuffer << std::endl ;

      goto done ;
   }

   rc = splitAuth();
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   if ( vm.count( CONSISTENCY_INSPECT_FILE ) &&
        0 == ossStrncmp( CI_ACTION_INSPECT, _header._action, CI_ACTION_SIZE ) )
   {
      std::cout << "file is specified, initialize all options according to file"
                << std::endl ;
      rc = initialize( &_header ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize from existing file failed: %d",
                   rc ) ;
   }
   else
   {
      rc = splitAddr() ;
      PD_RC_CHECK( rc, PDERROR, "Parse coordinator address failed: %d", rc ) ;
      if ( gTargetParser )
      {
         if ( '\0' != _list[0] )
         {
            rc = gTargetParser->parseByData( _list, ossStrlen( _list ) ) ;
         }
         else
         {
            rc = gTargetParser->parseByConfFile( _listFile, _header._coordAddr,
                                                 _header._serviceName,
                                                 g_username, g_password ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Parse target from list file failed: %d",
                      rc ) ;
      }
   }

   if ( 0 != ossStrncmp( CI_VIEW_GROUP, _header._view, CI_VIEWOPTION_SIZE ) &&
        0 != ossStrncmp( CI_VIEW_CL, _header._view, CI_VIEWOPTION_SIZE ) )
   {
      std::cout << "Invalid parameters:" << std::endl
                << "Unknown action : " << _header._action << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // in one dir, sdbinspect can be started only once
   rc = ossOpen( CI_START_TMP_FILE, OSS_CREATE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, startupFile ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open startup-file: " CI_START_TMP_FILE
                << ", rc = " << rc << std::endl ;
      goto error ;
   }

   startupFileOpened = TRUE ;
   rc = ossLockFile( &startupFile, OSS_LOCK_EX ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to lock startup-file while starting"
                << ", rc = " << rc << std::endl
                << "       There's another sdbinspect running in the same dir"
                << endl ;
      goto error ;
   }
   startupFileLocked = TRUE ;

   rc = inspect() ;
   if ( CI_INSPECT_CL_NOT_FOUND == rc )
   {
      rc = SDB_OK ;
      goto done ;
   }

   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   // report file
   if ( !useOutput )
   {
      ossMemcpy( outReport, CI_FILE_NAME, OSS_MAX_PATHSIZE ) ;
   }
   else
   {
      ossMemcpy( outReport, _header._outfile, OSS_MAX_PATHSIZE ) ;
   }
   ossStrncat( outReport, CI_FILE_REPORT, ossStrlen( CI_FILE_REPORT ) ) ;
   // Read the binary file and print the repord in a text file.
   rc = byGroup ? report ( _header._outfile, outReport,
                           tailBuffer, tailBufferSize )
                : report2( _header._outfile, outReport,
                           tailBuffer, tailBufferSize ) ;

   CHECK_VALUE( (SDB_OK != rc ), error ) ;
   std::cout << _header._action << " done" << std::endl ;
   std::cout << tailBuffer << std::endl ;

done:
   if ( startupFileLocked )
   {
      // if we have locked, we have all priority of this file.
      // and we should delete it
      ossLockFile( &startupFile, OSS_LOCK_UN ) ;
      ossClose( startupFile ) ;
      ossDelete( CI_START_TMP_FILE ) ;
   }
   else if ( startupFileOpened )
   {
      // if we have opened, but lock failed.
      // we do not have priority of this file. just close the file.
      ossClose( startupFile ) ;
   }

   if ( NULL != tailBuffer )
   {
      SDB_OSS_FREE( tailBuffer ) ;
      tailBuffer = NULL ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _sdbCi::inspect()
{
   INT32 rc           = SDB_OK ;
   INT32 curLoop      = 0 ;
   UINT64 totalRecord = 0 ;
   BOOLEAN finish     = FALSE ;
   sdbclient::sdb *coord = NULL ;
   CHAR inFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   CHAR tmpFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 }  ;
   INT32 loopNum = 0 ;

   coord = new sdbclient::sdb() ;
   if( NULL == coord )
   {
      std::cout << "Error: failed to allocate sdbclient::sdb" << std::endl ;
      rc = SDB_OOM ;
      goto error ;
   }

   rc = coord->connect( _header._coordAddr, _header._serviceName,
                        g_username, g_password ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to connect to " << _header._coordAddr
                << ":" << _header._serviceName ;
      if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
      {
         std::cout << "user: " << g_username
                   << "   password: " << g_password;
      }
      std::cout << std::endl ;
      goto error ;
   }

   if ( 0 == ossStrncmp( _header._filepath, "", OSS_MAX_PATHSIZE ) )
   {
      // No -f/--file argument is given, start from the beginning.
      do
      {
         curLoop += 1 ;
         PD_LOG( PDDEBUG, "Begin to inspect for round[%d]...", curLoop ) ;

         makeTmpFileName( _header._outfile, curLoop,
                          tmpFile, OSS_MAX_PATHSIZE ) ;

         rc = inspectWithoutFile( this, coord, &_header,
                                  tmpFile, totalRecord ) ;
      }while ( CI_INSPECT_ERROR == rc ) ;

      if ( CI_INSPECT_CL_NOT_FOUND == rc )
      {
         goto done ;
      }

      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      if ( 0 == totalRecord )
      {
         finish = TRUE ;
      }

      if ( _header._loop > 1 || _repair )
      {
         // use out file as input file for next loop
         ossMemcpy( inFile, tmpFile, OSS_MAX_PATHSIZE ) ;
      }
   }
   else
   {
      // Argument -f/--file is given, using existing intermediate file to
      // generate the report.
      ossMemcpy( inFile, _header._filepath, OSS_MAX_PATHSIZE ) ;
   }

   for (INT32 idx = curLoop; idx < _header._loop && !finish ; ++idx)
   {
      makeTmpFileName( _header._outfile, idx + 1, tmpFile, OSS_MAX_PATHSIZE ) ;
      PD_LOG( PDDEBUG, "Begin to inspect for round[%d] with temporary output "
                       "file[%s]", idx + 1, tmpFile ) ;
      rc = inspectWithFile( &_header, inFile, tmpFile, totalRecord, finish ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // use out file as input file for next loop
      ossMemset( inFile, 0, OSS_MAX_PATHSIZE ) ;
      ossMemcpy( inFile, tmpFile, OSS_MAX_PATHSIZE ) ;
      curLoop = idx ;
   }

   if ( !finish && _repair )
   {
      for ( INT32 idx = curLoop; idx < curLoop + _repairRetryTimes && !finish ;
            ++idx )
      {
         makeTmpFileName( _header._outfile, idx + 2, tmpFile,
                          OSS_MAX_PATHSIZE ) ;
         PD_LOG( PDDEBUG, "Begin post-inspect operation for round[%d] with "
                          "temporary output file[%s]",
                 idx + 1 - curLoop, tmpFile ) ;
         rc = inspectWithFile( &_header, inFile, tmpFile, totalRecord, finish,
                               TRUE ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         // After try to repair, inspect once again.
         rc = inspectWithFile( &_header, inFile, tmpFile, totalRecord, finish ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         // use out file as input file for next loop
         ossMemset( inFile, 0, OSS_MAX_PATHSIZE ) ;
         ossMemcpy( inFile, tmpFile, OSS_MAX_PATHSIZE ) ;

         // Remove the eldest temp file.
         CHAR eldestTmpFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 }  ;
         makeTmpFileName( _header._outfile, idx + 2 - _header._loop,
                          eldestTmpFile, OSS_MAX_PATHSIZE ) ;
         PD_LOG( PDDEBUG, "Remove the eldest temporary file: %s",
                 eldestTmpFile ) ;
         ossDelete( eldestTmpFile ) ;
      }
   }

   // Keep the last intermediate file as the output file(in binary format) and
   // delete all the other temporary files.
   if ( 0 != ossStrncmp( "", _header._outfile, OSS_MAX_PATHSIZE ) )
   {
      rc = ossRenamePath( tmpFile, _header._outfile ) ;
   }
   else
   {
      rc = ossRenamePath( tmpFile, CI_FILE_NAME ) ;
   }
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to rename temp file to \""
                << _header._outfile << "\"" << std::endl ;
      goto error ;
   }

   // delete temp file
   for ( INT32 idx = 0 ; idx < loopNum ; ++idx )
   {
      makeTmpFileName( _header._outfile, idx + 1, tmpFile, OSS_MAX_PATHSIZE ) ;

      ossDelete( tmpFile ) ;
   }

done:
   if ( NULL != coord )
   {
      delete coord ;
      coord = NULL ;
   }
   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 _sdbCi::report ( const CHAR *inFile, const CHAR *reportFile,
                       CHAR *&tailBuffer, INT64 &tailBufferSize )
{
   INT32 rc           = SDB_OK ;
   BOOLEAN inOpened   = FALSE ;
   BOOLEAN outOpened  = FALSE ;
   CHAR *buffer       = NULL ;
   INT64 bufferSize   = 0 ;
   INT64 validSize    = 0 ;
   INT64 fileSize     = 0 ;
   INT64 offset       = 0 ;
   INT64 tailOffset   = 0 ;

   ciHeader header ;
   ciLinkList< ciOffset > Offset ;
   ciLinkList< ciNode > ciNodes ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciTail tail ;
   OSSFILE in ;
   OSSFILE out ;
   // open in file
   rc = ossOpen( inFile, OSS_RO, OSS_RU | OSS_RG, in ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << inFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   inOpened = TRUE ;

   rc = ossGetFileSize( &in, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to get file size" << std::endl ;
      goto error ;
   }

   if ( SDB_OK != rc )
   {
      std::cout << "Error: filesize is lt " << CI_HEADER_SIZE << std::endl ;
      goto error ;
   }

   rc = ossOpen( reportFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, out ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file, rc = " << rc << std::endl ;
      goto error ;
   }
   outOpened = TRUE ;

   // dump header
   rc = readCiHeader( in, &header ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = dumpCiHeader( &header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // read tail
   tailOffset = fileSize - header._tailSize ;
   rc = readCiTail( in, tailOffset, &tail ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   //skip 65536 bytes
   offset = CI_HEADER_SIZE ;
   while ( offset < tailOffset )
   {
      // dump group
      rc = readCiGroupHeader( in, offset, &groupHeader ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = dumpCiGroupHeader( &groupHeader, buffer, bufferSize, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = writeToFile( out, buffer, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      // read global nodes info
      ciNodes.clear() ;
      rc = readCiNode( in, offset, groupHeader, ciNodes ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = dumpCiNode( ciNodes, buffer, bufferSize, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      rc = writeToFile( out, buffer, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;

      UINT32 idx = 0 ;
      while ( idx < groupHeader._clCount )
      {
         ciClHeader clHeader ;
         ciLinkList< ciRecord > records ;
         ciLinkList< ciNode > nodesForCL ;

         rc = readCiClHeader( in, offset, &clHeader ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciClHeader" << std::endl ;
            goto error ;
         }
         rc = dumpCiClHeader( &clHeader, buffer, bufferSize, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         rc = writeToFile( out, buffer, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         rc = readCiNode( in, offset, groupHeader, nodesForCL ) ;
         if ( SDB_OK != rc )
         {
            std::cout << "Error: failed to get ciNode" << std::endl ;
            goto error ;
         }
         rc = dumpCiNodeSimple( nodesForCL, buffer, bufferSize, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         rc = writeToFile( out, buffer, validSize ) ;
         CHECK_VALUE( ( SDB_OK != rc ), error ) ;

         if ( clHeader._recordCount > 0 )
         {
            records.clear() ;

            rc = readCiRecord( in, offset, ciNodes,
                               clHeader, records, TRUE ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = dumpCiRecord( ciNodes, records,
                               buffer, bufferSize, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
            rc = writeToFile( out, buffer, validSize ) ;
            CHECK_VALUE( ( SDB_OK != rc ), error ) ;
         }

         ++idx ;
      }
   }

   rc = dumpCiTail( tail, tailBuffer, tailBufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, tailBuffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tailBufferSize = validSize ;

done:
   if ( inOpened )
   {
      ossClose( in ) ;
   }

   if ( outOpened )
   {
      ossClose( out ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}



INT32 _sdbCi::report2( const CHAR *inFile, const CHAR *reportFile,
                       CHAR *&tailBuffer, INT64 &tailBufferSize )
{
   INT32 rc              = SDB_OK ;
   BOOLEAN inOpened      = FALSE ;
   BOOLEAN outOpened     = FALSE ;
   ciOffset *groupOffset = NULL ;
   CHAR *buffer          = NULL ;
   INT64 bufferSize      = 0 ;
   INT64 validSize       = 0 ;
   INT64 fileSize        = 0 ;
   INT64 tailOffset      = 0 ;

   ciHeader header ;
   ciLinkList< ciNode > ciNodes ;
   ciLinkList< ciOffset > clOffsets ;
   ciGroupHeader groupHeader ;
   ciClHeader clHeader ;
   ciTail tail ;
   OSSFILE in ;
   OSSFILE out ;
   // open in file
   rc = ossOpen( inFile, OSS_RO, OSS_RU | OSS_WU | OSS_RG, in ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file: " << inFile
                << ", rc = " << rc << std::endl ;
      goto error ;
   }
   inOpened = TRUE ;

   rc = ossGetFileSize( &in, &fileSize ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to get file size" << std::endl ;
      goto error ;
   }

   if ( SDB_OK != rc )
   {
      std::cout << "Error: filesize is lt " << CI_HEADER_SIZE << std::endl ;
      goto error ;
   }

   rc = ossOpen( reportFile, OSS_REPLACE | OSS_READWRITE,
                 OSS_RU | OSS_WU | OSS_RG, out ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Error: failed to open file, rc = " << rc << std::endl ;
      goto error ;
   }
   outOpened = TRUE ;

   // dump header
   rc = readCiHeader( in, &header ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = dumpCiHeader( &header, buffer, bufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, buffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   // read tail
   tailOffset = fileSize - header._tailSize ;
   rc = readCiTail( in, tailOffset, &tail ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;

   tail._groupOffset.resetCurrentNode() ;
   groupOffset = tail._groupOffset.getHead() ;
   while ( NULL != groupOffset )
   {
      rc = dumpOneCl( in, out, groupOffset, clOffsets, NULL,
                      buffer, bufferSize, validSize ) ;
      CHECK_VALUE( ( SDB_OK != rc ), error ) ;
      groupOffset = tail._groupOffset.next() ;
   }

   rc = dumpCiTail( tail, tailBuffer, tailBufferSize, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   rc = writeToFile( out, tailBuffer, validSize ) ;
   CHECK_VALUE( ( SDB_OK != rc ), error ) ;
   tailBufferSize = validSize ;

done:
   if ( inOpened )
   {
      ossClose( in ) ;
   }

   if ( outOpened )
   {
      ossClose( out ) ;
   }

   if ( NULL != buffer )
   {
      SDB_OSS_FREE( buffer ) ;
      buffer = NULL ;
   }

   return rc ;
error:
   OUTPUT_FUNCTION( "Error occurs in ", __FUNCTION__, rc ) ;
   goto done ;
}

INT32 _sdbCi::doDataExchange( engine::pmdCfgExchange *pEx )
{
   resetResult() ;

   rdxString( pEx, CONSISTENCY_INSPECT_COORD, _coordAddr,
                   CI_ADDRESS_SIZE , FALSE, FALSE, CI_COORD_DEFVAL, FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_AUTH, _auth,
                   CI_AUTH_SIZE , FALSE, FALSE, "\"\":\"\"", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_ACTION, _header._action,
                   CI_ACTION_SIZE , FALSE, FALSE, CI_ACTION_INSPECT, FALSE ) ;

   rdxInt( pEx, CONSISTENCY_INSPECT_LOOP, _header._loop, FALSE, TRUE, 5 ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_GROUP, _header._groupName,
                   CI_GROUPNAME_SIZE, FALSE, FALSE, "", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_CS, _header._csName,
                   CI_CS_NAME_SIZE, FALSE, FALSE, "", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_CL, _header._clName,
                   CI_CL_NAME_SIZE, FALSE, FALSE, "", FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_FILE, _header._filepath,
                   OSS_MAX_PATHSIZE, FALSE, FALSE, "" ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_OUTPUT, _header._outfile,
                   OSS_MAX_PATHSIZE, FALSE, FALSE, CI_FILE_NAME ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_VIEW, _header._view,
                   CI_VIEWOPTION_SIZE, FALSE, FALSE, CI_VIEW_GROUP, FALSE ) ;

   rdxBooleanS( pEx, CONSISTENCY_INSPECT_FAST, gFastMode,
                FALSE, FALSE, FALSE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_LIST, _list,
              CI_ARG_MAX_SIZE, FALSE, FALSE, "" ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_LISTFILE, _listFile,
                   OSS_MAX_PATHSIZE, FALSE, FALSE, "" ) ;

   rdxInt( pEx, CONSISTENCY_INSPECT_BRAKETIME, _brakeTime, FALSE, FALSE, 0 ) ;

   rdxInt( pEx, CONSISTENCY_INSPECT_BRAKESTEP, _brakeStep, FALSE, FALSE,
           CI_BRAKE_DEFAULT_STEP ) ;

   rdxBooleanS( pEx, CONSISTENCY_INSPECT_REPAIR, _repair,
                FALSE, FALSE, FALSE, TRUE ) ;

   rdxInt( pEx, CONSISTENCY_INSPECT_RETRY, _repairRetryTimes, FALSE, FALSE,
           CI_REPAIR_RETRY_TIMES, TRUE ) ;

#ifdef _DEBUG
   rdxString( pEx, CONSISTENCY_INSPECT_ENCODE, _encodeFile,
              OSS_MAX_PATHSIZE, FALSE, FALSE, "", TRUE ) ;

   rdxString( pEx, CONSISTENCY_INSPECT_DECODE, _decodeFile,
              OSS_MAX_PATHSIZE, FALSE, FALSE, "", TRUE ) ;
#endif

   gOptionMgr = this ;

   return getResult() ;
}

INT32 _sdbCi::postLoaded( PMD_CFG_STEP step )
{
   return SDB_OK ;
}

INT32 _sdbCi::preSaving()
{
   return SDB_OK ;
}

// Parse coordinate address.
INT32 _sdbCi::splitAddr()
{
   INT32 rc        = SDB_OK ;
   INT32 length    = ossStrlen( _coordAddr ) ;
   CHAR *begin     = _coordAddr ;
   CHAR *end       = begin + length ;
   const CHAR *pch = NULL ;

   if ( begin == end )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " Hostname and servicename of coord must be specified"
                << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pch = ossStrrchr( _coordAddr, ':' ) ;
   if ( begin == pch )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " Hostname of coord must be specified" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == pch || end == pch + 1 )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " Service Name must be specified after the hostname, "
                   " split by \":\"" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // initialize hostname and servicename in _header
   ossMemcpy( _header._coordAddr, _coordAddr, pch - begin ) ;
   ossMemcpy( _header._serviceName, pch + 1, end - pch ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _sdbCi::splitAuth()
{
   INT32 rc        = SDB_OK ;
   INT32 length    = ossStrlen( _auth ) ;
   CHAR *begin     = _auth ;
   CHAR *end       = begin + length ;
   const CHAR *pch = NULL ;

   if ( begin == end )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " username and password of sequoiadb is NULL"
                << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pch = ossStrrchr( _auth, ':' ) ;
   if ( NULL == pch || end == pch + 1 )
   {
      std::cout << "Invalid parameters" << std::endl ;
      std::cout << " hostname and password should be split by \":\"" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // initialize hostname and servicename in _header
   ossMemcpy( g_username, _auth, pch - begin ) ;
   ossMemcpy( g_password, pch + 1, end - pch ) ;

done:
   return rc ;
error:
   goto done ;
}

//////////////////////////////////////////////////////////////////////////
///< main function
INT32 main(INT32 argc, CHAR** argv)
{
   INT32 rc  = SDB_OK ;
   sdbCi *ci = NULL ;
   po::variables_map vm ;

   sdbEnablePD( INSPECT_LOG_NAME ) ;
   setPDLevel( PDDEBUG ) ;

   ci = SDB_OSS_NEW sdbCi() ;
   PD_CHECK( NULL != ci, SDB_OOM, error, PDERROR,
             "Allocate memory for inspector failed: %d", rc ) ;

   rc = ci->init( argc, argv, vm ) ;
   if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
   {
      rc = SDB_OK ;
      goto done ;
   }
   else if ( rc )
   {
      goto error ;
   }

   rc = ci->handle( vm ) ;
   PD_RC_CHECK( rc, PDERROR, "Operation failed: %d", rc ) ;

done:
   if ( ci )
   {
      SDB_OSS_DEL ci ;
   }
   return rc ;
error:
   ossPrintf( "Something went wrong. Please check the log file[%s] at current "
              "directory"OSS_NEWLINE, INSPECT_LOG_NAME ) ;
   goto done ;
}
