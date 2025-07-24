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

   Source File Name = mthCommon.cpp

   Descriptive Name = Method Common

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains common functions for mth

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/12/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "mthCommon.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"

namespace engine
{
   // the function try to append newStr to ppStr.
   // if the buffer is not large enough the function is responsible to allocate
   // a larger one. If failed to allocate larger buffer, this function must
   // maintain the validity of original pointer
   INT32 mthAppendString ( CHAR **ppStr, INT32 &bufLen,
                           INT32 strLen, const CHAR *newStr,
                           INT32 newStrLen, INT32 *pMergedLen )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( ppStr && newStr, "str or newStr can't be NULL" ) ;
      // if user doesn't know the string length, pass 0
      if ( !*ppStr )
      {
         strLen = 0 ;
      }
      else if ( strLen <= 0 )
      {
         strLen = ossStrlen ( *ppStr ) ;
      }
      // if user doesn't know the new string len, pass 0
      if ( newStrLen <= 0 )
      {
         newStrLen = ossStrlen ( newStr ) ;
      }
      // make sure the string len and new string len is less than buffer
      if ( strLen + newStrLen >= bufLen )
      {
         // we need to allocate more memory if exceed buffer
         CHAR *pOldStr = *ppStr ;
         INT32 newSize = ossRoundUpToMultipleX ( strLen + newStrLen,
                                                 SDB_PAGE_SIZE ) ;
         if ( newSize < 0 )
         {
            PD_LOG ( PDERROR, "new buffer overflow, size: %d", newSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         *ppStr = (CHAR*)SDB_OSS_REALLOC ( *ppStr, sizeof(CHAR)*(newSize) ) ;
         if ( !*ppStr )
         {
            PD_LOG ( PDERROR, "Failed to allocate %d bytes buffer", newSize ) ;
            rc = SDB_OOM ;
            *ppStr = pOldStr ;
            goto error ;
         }
         bufLen = newSize ;
      }
      // now new buffer is allocated or we already have enough memory, let's do
      // copy
      if ( *ppStr && newStr )
      {
         ossMemcpy ( &(*ppStr)[strLen], newStr, newStrLen ) ;
         (*ppStr)[strLen+newStrLen] = '\0' ;

         if ( pMergedLen )
         {
            *pMergedLen = strLen + newStrLen ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDOUBLEBUFFERSIZE, "mthDoubleBufferSize" )
   INT32 mthDoubleBufferSize ( CHAR **ppStr, INT32 &bufLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHDOUBLEBUFFERSIZE ) ;
      SDB_ASSERT ( ppStr, "ppStr can't be NULL" ) ;
      CHAR *pOldStr = *ppStr ;
      INT32 newSize = ossRoundUpToMultipleX ( 2*bufLen,
                                              SDB_PAGE_SIZE ) ;
      if ( newSize < 0 )
      {
         PD_LOG ( PDERROR, "new buffer overflow" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( 0 == newSize )
      {
         newSize = SDB_PAGE_SIZE ;
      }
      *ppStr = (CHAR*)SDB_OSS_REALLOC ( *ppStr, sizeof(CHAR)*(newSize) ) ;
      if ( !*ppStr )
      {
         PD_LOG ( PDERROR, "Failed to allocate %d bytes buffer", newSize ) ;
         rc = SDB_OOM ;
         *ppStr = pOldStr ;
         goto error ;
      }
      bufLen = newSize ;

   done :
      PD_TRACE_EXITRC ( SDB__MTHDOUBLEBUFFERSIZE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 mthCheckFieldName( const CHAR *pField, INT32 &dollarNum )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTmp = pField ;
      const CHAR *pDot = NULL ;
      INT32 number = 0 ;
      dollarNum = 0 ;

      while ( pTmp && *pTmp )
      {
         pDot = ossStrchr( pTmp, '.' ) ;
         if ( '$' == *pTmp )
         {
            if ( pDot )
            {
               *(CHAR*)pDot = 0 ;
            }
            rc = ossStrToInt( pTmp + 1, &number ) ;
            // Restore
            if ( pDot )
            {
               *(CHAR*)pDot = '.' ;
            }
            if ( rc )
            {
               goto error ;
            }
            ++dollarNum ;
         }
         pTmp = pDot ? pDot + 1 : NULL ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN mthCheckUnknowDollar( const CHAR *pField,
                                 std::vector<INT64> *dollarList )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTmp = pField ;
      const CHAR *pDot = NULL ;
      INT32 number = 0 ;
      BOOLEAN hasUnknowDollar = FALSE ;

      while ( pTmp && *pTmp )
      {
         pDot = ossStrchr( pTmp, '.' ) ;

         if ( '$' == *pTmp )
         {
            if ( pDot )
            {
               *(CHAR*)pDot = 0 ;
            }
            rc = ossStrToInt( pTmp + 1, &number ) ;
            // Restore
            if ( pDot )
            {
               *(CHAR*)pDot = '.' ;
            }
            if ( rc )
            {
               goto error ;
            }

            if ( dollarList )
            {
               std::vector<INT64>::iterator it = dollarList->begin() ;
               for ( ; it != dollarList->end() ; ++it )
               {
                  if ( number == (((*it)>>32)&0xFFFFFFFF) )
                  {
                     break ;
                  }
               }
               if ( it == dollarList->end() )
               {
                  goto error ;
               }
            }
         }
         pTmp = pDot ? pDot + 1 : NULL ;
      }

   done:
      return hasUnknowDollar ? FALSE : TRUE ;
   error:
      hasUnknowDollar = TRUE ;
      goto done ;
   }

   INT32 mthConvertSubElemToNumeric( const CHAR *desc,
                                     INT32 &n )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != desc && '$' == *desc, "must be a $" ) ;
      const CHAR *p = desc ;
      const UINT32 maxLen = 10 ;
      CHAR number[maxLen + 1] = { 0 } ;
      UINT32 numberLen = 0 ;

      if ( *p && '$' == *p && p[1] && '[' == p[1] )
      {
         p += 2 ;
         while ( *p )
         {
            if ( '0' <= *p && *p <= '9' )
            {
               if ( numberLen == maxLen )
               {
                  PD_LOG( PDERROR, "number is too long" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               number[numberLen++] = *p++ ;
            }
            else if ( ']' == *p )
            {
               break ;
            }
            else
            {
               PD_LOG( PDDEBUG, "argument should be a numeric" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         if ( 0 == numberLen || ']' != *p )
         {
            PD_LOG( PDDEBUG, "invalid action in selector:%s", desc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         number[numberLen] = '\0' ;
         n = ossAtoi( number ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "invalid action:%s", desc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
     return rc ;
   error:
     goto done ;
   }
}

