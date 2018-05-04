/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilParseData.cpp

   Descriptive Name =

   When/how to use: parse Data util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/29/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#include "utilParseData.hpp"

void _bucket::wait_to_get_exclusive_lock()
{
   boost::unique_lock<boost::mutex> lock ( _mutex ) ;
   while ( 0 != lockCounter )
   {
      condVar.wait ( lock ) ;
   }
}

void _bucket::inc()
{
   boost::lock_guard<boost::mutex> lock ( _mutex ) ;
   ++lockCounter ;
}

void _bucket::dec()
{
   boost::lock_guard<boost::mutex> lock ( _mutex ) ;
   --lockCounter ;
   condVar.notify_one() ;
}

_bucket::_bucket() : lockCounter(0)
{
}



_utilDataParser::_utilDataParser() : _bufferSize(0),
                                     _line(1),
                                     _column(1),
                                     _blockNum(0),
                                     _blockSize(0),
                                     _accessModel(UTIL_GET_IO),
                                     _buffer(NULL),
                                     _pAccessData(NULL)
{
   _delChar[0]   = '"' ;
   _delField[0]  = ',' ;
   _delRecord[0] = '\n' ;
   _delChar[1]   = 0 ;
   _delField[1]  = 0 ;
   _delRecord[1] = 0 ;

   _charSpace[0] = '\t' ;
   _charSpace[1] = '\r' ;
   _charSpace[2] = '\n' ;
   _charSpace[3] = 32 ; //space
}

CHAR *_utilDataParser::_trimLeft ( CHAR *pCursor, UINT32 &size )
{
   while ( pCursor &&
           ( (_charSpace[0] != 0 && _charSpace[0] == *pCursor) ||
             (_charSpace[1] != 0 && _charSpace[1] == *pCursor) ||
             (_charSpace[2] != 0 && _charSpace[2] == *pCursor) ||
             (_charSpace[3] != 0 && _charSpace[3] == *pCursor) ) )
   {
      --size ;
      ++pCursor ;
      if ( 0 == size )
      {
         pCursor = NULL ;
         break ;
      }
   }
   return pCursor ;
}

CHAR *_utilDataParser::_trimRight ( CHAR *pCursor, UINT32 &size )
{
   while ( pCursor &&
           ( (_charSpace[0] != 0 && _charSpace[0] == *pCursor) ||
             (_charSpace[1] != 0 && _charSpace[1] == *pCursor) ||
             (_charSpace[2] != 0 && _charSpace[2] == *pCursor) ||
             (_charSpace[3] != 0 && _charSpace[3] == *pCursor) ) )
   {
      --size ;
      --pCursor ;
      if ( 0 == size )
      {
         pCursor = NULL ;
         break ;
      }
   }
   return pCursor ;
}

BOOLEAN _utilDataParser::parse_number ( const CHAR *buffer, UINT32 size )
{
   BOOLEAN  rc             = FALSE ;

   if ( *buffer != '+' && *buffer != '-' &&
        ( *buffer < '0' || *buffer >'9' ) )
   {
      rc = FALSE ;
      goto done ;
   }

   if ( 0 == size )
   {
      rc = FALSE ;
      goto done ;
   }
   /* Could use sscanf for this? */
   /* Has sign? */
   if ( '-' == *buffer || '+' == *buffer )
   {
      --size ;
      ++buffer ;
   }

   if ( 0 == size )
   {
      rc = FALSE ;
      goto done ;
   }

   while ( size > 0 && '0' == *buffer )
   {
      /* is zero */
      ++buffer ;
      --size ;
   }

   if ( 0 == size )
   {
      rc = TRUE ;
      goto done ;
   }

   if ( *buffer >= '1' && *buffer <= '9' )
   {
      do
      {
         --size ;
         ++buffer ;
      }
      while ( size > 0 && *buffer >= '0' && *buffer <= '9' ) ;
      if ( 0 == size )
      {
         rc = TRUE ;
         goto done ;
      }
   }
   if ( *buffer == '.' && buffer[1] >= '0' && buffer[1] <= '9' )
   {
      --size ;
      ++buffer ;
      while ( size > 0 && *buffer >= '0' && *buffer <= '9' )
      {
         --size ;
         ++buffer ;
      }
      if ( 0 == size )
      {
         rc = TRUE ;
         goto done ;
      }
   }
   if ( *buffer == 'e' || *buffer == 'E' )
   {
      --size ;
      ++buffer ;
      if ( 0 == size )
      {
         rc = FALSE ;
         goto done ;
      }

      if ( '+' == *buffer )
      {
         --size ;
         ++buffer ;
         if ( 0 == size )
         {
            rc = FALSE ;
            goto done ;
         }
      }

      else if ( '-' == *buffer )
      {
         --size ;
         ++buffer;
         if ( 0 == size )
         {
            rc = FALSE ;
            goto done ;
         }
      }
      while ( size > 0 && *buffer >= '0' && *buffer <= '9' )
      {
         --size ;
         ++buffer ;
      }
      if ( 0 == size )
      {
         rc = TRUE ;
         goto done ;
      }
   }
   rc = FALSE ;
done:
   return rc ;
}

void _utilDataParser::mallocBufer ( UINT32 size )
{
   SAFE_OSS_FREE ( _buffer ) ;
   _buffer = (CHAR *)SDB_OSS_MALLOC ( size ) ;
}

void _utilDataParser::setDel ( CHAR delChar, CHAR delField, CHAR delRecord )
{
   _delChar[0]   = delChar ;
   _delField[0]  = delField ;
   _delRecord[0] = delRecord ;
}

_utilDataParser::~_utilDataParser()
{
   SAFE_OSS_FREE ( _buffer ) ;
   SAFE_OSS_DELETE( _pAccessData ) ;
}
