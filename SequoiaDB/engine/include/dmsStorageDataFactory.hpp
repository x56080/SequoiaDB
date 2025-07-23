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

   Source File Name = dmsStorageDataFactory.hpp

   Descriptive Name = dms Storage Data Object Factory.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SU cache
   management ( including plan cache, statistics cache, etc. )

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_DATAFACTORY_HPP__
#define DMSSTORAGE_DATAFACTORY_HPP__

#include "dmsStorageDataCommon.hpp"

namespace engine
{
   class _dmsStorageDataFactory : public SDBObject
   {
   public:
      _dmsStorageDataFactory() {}
      ~_dmsStorageDataFactory() {}

      dmsStorageDataCommon* createProduct( DMS_STORAGE_TYPE type,
                                           const CHAR *suFileName,
                                           dmsStorageInfo *info,
                                           _IDmsEventHolder *pEventHolder ) ;
   } ;
   typedef _dmsStorageDataFactory dmsStorageDataFactory ;

   dmsStorageDataFactory* getDMSStorageDataFactory() ;
}

#endif /* DMSSTORAGE_DATAFACTORY_HPP__ */

