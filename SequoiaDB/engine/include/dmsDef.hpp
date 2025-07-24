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

   Source File Name = dmsDef.hpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/23/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_DEF_HPP_
#define DMS_DEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"

namespace engine
{

   /*
      DMS_STORAGE_ENGINE_TYPE define
    */
   enum DMS_STORAGE_ENGINE_TYPE
   {
      DMS_STORAGE_ENGINE_UNKNOWN = -1,
      DMS_STORAGE_ENGINE_MMAP = 0,
      DMS_STORAGE_ENGINE_VESSEL = 1,
      DMS_STORAGE_ENGINE_WIREDTIGER = 2
   } ;

   #define DMS_STORAGE_ENGINE_NAME_UNKNOWN "unknown"
   #define DMS_STORAGE_ENGINE_NAME_MMAP "mmap"
   #define DMS_STORAGE_ENGINE_NAME_VESSEL "vessel"
   #define DMS_STORAGE_ENGINE_NAME_WIREDTIGER "wiredtiger"

   OSS_INLINE const CHAR *dmsGetStorageEngineName( DMS_STORAGE_ENGINE_TYPE engine )
   {
      const CHAR *name = DMS_STORAGE_ENGINE_NAME_UNKNOWN ;
      switch ( engine )
      {
         case DMS_STORAGE_ENGINE_MMAP:
            name = DMS_STORAGE_ENGINE_NAME_MMAP ;
            break ;
         case DMS_STORAGE_ENGINE_VESSEL:
            name = DMS_STORAGE_ENGINE_NAME_VESSEL ;
            break ;
         case DMS_STORAGE_ENGINE_WIREDTIGER:
            name = DMS_STORAGE_ENGINE_NAME_WIREDTIGER ;
            break ;
         default:
            break ;
      }
      return name ;
   }

   OSS_INLINE DMS_STORAGE_ENGINE_TYPE dmsGetStorageEngine( const CHAR *name )
   {
      DMS_STORAGE_ENGINE_TYPE engine = DMS_STORAGE_ENGINE_UNKNOWN ;
      if ( 0 == ossStrcasecmp( name, DMS_STORAGE_ENGINE_NAME_MMAP ) )
      {
         engine = DMS_STORAGE_ENGINE_MMAP ;
      }
      else if ( 0 == ossStrcasecmp( name, DMS_STORAGE_ENGINE_NAME_VESSEL ) )
      {
         engine = DMS_STORAGE_ENGINE_VESSEL ;
      }
      else if ( 0 == ossStrcasecmp( name, DMS_STORAGE_ENGINE_NAME_WIREDTIGER ) )
      {
         engine = DMS_STORAGE_ENGINE_WIREDTIGER ;
      }
      return engine ;
   }

}

#endif /* DMS_DEF_HPP_ */

