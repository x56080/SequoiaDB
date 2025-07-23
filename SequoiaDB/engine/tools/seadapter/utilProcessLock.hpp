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

   Source File Name = utilProcessLock.hpp

   Descriptive Name = Util Processor Lock.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/25/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_PROCESS_LOCK__
#define UTIL_PROCESS_LOCK__

#include "oss.hpp"
#include "ossIO.hpp"

namespace seadapter
{
   class _IProcessLock
   {
   public:
      _IProcessLock() {}
      virtual ~_IProcessLock() {}

      virtual INT32 init( const CHAR *processName, void *arg ) = 0 ;
      virtual INT32 lock() = 0 ;
      virtual INT32 tryLock() = 0 ;
      virtual void unlock() = 0 ;
      virtual void destroy() = 0 ;
   } ;
   typedef _IProcessLock IProcessLock ;

   class _utilProcFlockMutex : public IProcessLock
   {
   public:
      _utilProcFlockMutex() ;
      ~_utilProcFlockMutex() ;
      INT32 init( const CHAR *processName, void *arg ) ;
      INT32 lock() ;
      INT32 tryLock() ;
      void unlock() ;
      void destroy() ;
   private:
      std::string _lockFileName ;
      OSSFILE     _file ;
      BOOLEAN     _fileOpened ;
      BOOLEAN     _fileLocked ;
   } ;
   typedef _utilProcFlockMutex utilProcFlockMutex ;
}

#endif /* UTIL_PROCESS_LOCK__ */

