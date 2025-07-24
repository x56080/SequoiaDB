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

   Source File Name = sequoiaFSMain.hpp

   Descriptive Name = sequoiafs main entry.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSMAIN_H__
#define __SEQUOIAFSMAIN_H__

#include "sequoiaFS.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/xattr.h>
#include <sys/syscall.h>
#include "fuse.h"
#include "fuse_lowlevel.h"
#include "utilPidFile.hpp"
#include "ossFile.hpp"
#include "ossCmdRunner.hpp"

#ifndef SEQUOIAFS_SUPPORT_FUSE_VERSION
/*SequoiaFS support libfuse version, 0x0286 means 2.8.6*/
#define SEQUOIAFS_SUPPORT_FUSE_VERSION 0x0286
#endif

#endif

