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

   Source File Name = ossShMem.hpp

   Descriptive Name = share memory

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/09/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#if defined (_LINUX)
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#elif defined (_WINDOWS)

#endif


#if defined (_LINUX)
// to  create  a new segment.
// If this flag is not used, then ossSHMAlloc() will find
// the segment associated with key and check to see
// if the user  has  permission to access the segment.
#define OSS_SHM_CREATE        IPC_CREAT
// used with OSS_SHM_CREATE to ensure failure if the segment already exists
#define OSS_SHM_EXCL          IPC_EXCL

typedef UINT32    ossSHMMid;
typedef key_t     ossSHMKey;

#elif defined (_WINDOWS)
typedef HANDLE    ossSHMMid;
typedef CHAR*     ossSHMKey;

// to  create  a new segment.
// If this flag is not used, then ossSHMAlloc() will find
// the segment associated with key and check to see
// if the user  has  permission to access the segment.
#define OSS_SHM_CREATE        0X01
// used with OSS_SHM_CREATE to ensure failure if the segment already exists
#define OSS_SHM_EXCL          0X02

#endif


CHAR *ossSHMAlloc( ossSHMKey shmKey, UINT32 bufSize, INT32 shmFlag,
                   ossSHMMid &shmMid );

void ossSHMFree( ossSHMMid &shmMid, CHAR **ppBuf );

CHAR *ossSHMAttach( ossSHMKey shmKey, UINT32 bufSize, ossSHMMid &shmMid );

void ossSHMDetach( ossSHMMid & shmMid, CHAR **ppBuf );

