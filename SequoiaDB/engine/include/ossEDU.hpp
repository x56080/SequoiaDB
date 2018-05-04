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

   Source File Name = ossEDU.hpp

   Descriptive Name = Operating System Services Engine Dispatchable Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains structure for EDU
   signal/exception handlings.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_EDU_HPP
#define OSS_EDU_HPP
#include "core.hpp"
#include "oss.hpp"
#include "ossSignal.hpp"

namespace engine
{
#if defined(_LINUX)
   void ossStackTrace( OSS_HANDPARMS, const CHAR * dumpDir ) ;
   void ossEDUCodeTrapHandler( OSS_HANDPARMS ) ;
#elif defined(_WINDOWS)
   void ossStackTrace( LPEXCEPTION_POINTERS lpEP, const CHAR * dumpDir ) ;
   SINT32 ossEDUExceptionFilter( LPEXCEPTION_POINTERS lpEP ) ;
#endif
   void ossSetTrapExceptionPath ( const CHAR *path ) ;
   const CHAR* ossGetTrapExceptionPath () ;

   //
   // class oss_edu_data
   //    edu specific data for oss component
   //
   struct _oss_edu_data
   {
   public :
   #define OSS_EDU_DATA_EYE_CATCHER ( 0xBEEFC0FFEE00 )
      UINT64 ossEDUEyeCatcher1 ;
      UINT64 ossEDUFlag ;
      SINT32 _depth ;
      SINT32 _nestedDepth ;
   #if defined (_LINUX)
      OSS_SIGFUNCPTR ossEDUNestedSignalHandler ;
      // This long jump buffer is used to handle nested signal
      // and it is used by ossEDUNestedSignalHandler
   #endif
   #if defined (_LINUX)
      sigjmp_buf ossNestedSignalHanderJmpBuf ;
   #elif defined (_WINDOWS)
      jmp_buf ossNestedSignalHanderJmpBuf ;
   #endif
      UINT64 ossEDUEyeCatcher2 ;

      void init()
      {
         ossEDUEyeCatcher1 = OSS_EDU_DATA_EYE_CATCHER ;
         ossEDUFlag = 0 ;
         _depth = 0 ;
         _nestedDepth = 0 ;
      #if defined (_LINUX)
         ossEDUNestedSignalHandler = 0 ;
      #endif
         ossEDUEyeCatcher2 = OSS_EDU_DATA_EYE_CATCHER ;
      }
   } ;
   typedef _oss_edu_data oss_edu_data ;

   oss_edu_data* ossGetThreadEDUData() ;

   #define OSS_ENTER_SIGNAL_HANDLER( p )                                     \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_depth++ ;                                                     \
      }                                                                      \
   }

   #define OSS_LEAVE_SIGNAL_HANDLER( p )                                     \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_depth-- ;                                                     \
      }                                                                      \
   }

   #define OSS_AM_I_INSIDE_SIGNAL_HANDLER( p )                               \
   ( ( NULL != (p) ) && ( (p)->_depth > 0 ) )

   #define OSS_INVOKE_NESTED_SIGNAL_HANDLER( p )                             \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth++ ;                                               \
      }                                                                      \
   }

   #define OSS_LEAVE_NESTED_SIGNAL_HANDLER( p )                              \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth-- ;                                               \
      }                                                                      \
   }

   #define OSS_AM_I_HANDLING_NESTED_SIGNAL( p )                              \
   ( ( NULL != (p) ) && ( (p)->_nestedDepth > 0 ) )

   #define OSS_SET_NESTED_HANDLER_LEVEL( p, l )                              \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth = l ;                                             \
      }                                                                      \
   }

   #define OSS_CLEAR_NESTED_HANDLER_LEVEL( p )                               \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         (p)->_nestedDepth = 0 ;                                             \
      }                                                                      \
   }

   #define OSS_GET_NESTED_HANDLER_LEVEL( p, l )                              \
   {                                                                         \
      if ( NULL != (p) )                                                     \
      {                                                                      \
         l = (p)->_nestedDepth  ;                                            \
      }                                                                      \
   }
}
#if defined (_LINUX)
#define OSS_MAX_SIGAL         (_NSIG-1)

/*
   ossSigSet define
*/
class _ossSigSet : public SDBObject
{
public:
   _ossSigSet () ;
   ~_ossSigSet () ;

   void emptySet () ;
   void fillSet () ;
   void sigDel ( INT32 sigNum ) ;
   void sigAdd ( INT32 sigNum ) ;
   BOOLEAN isMember ( INT32 sigNum ) ;

private:
   INT32       _sigArray[OSS_MAX_SIGAL+1] ;

};
typedef _ossSigSet ossSigSet ;
typedef void (*SIG_HANDLE)( INT32 sigNum ) ;

INT32 ossRegisterSignalHandle ( ossSigSet &sigSet, SIG_HANDLE handle ) ;

#endif //_LINUX


#endif
