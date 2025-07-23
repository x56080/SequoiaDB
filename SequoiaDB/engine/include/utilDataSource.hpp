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

   Source File Name = utilDataSource.hpp

   Descriptive Name = Data source utilities

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains catalog command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/13/2021  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_DATASOURCE_HPP__
#define UTIL_DATASOURCE_HPP__
#include "IDataSource.hpp"

namespace engine
{
   /**
    * Get data source propagation mode description by type.
    */
   const CHAR *sdbDSTransModeDesc( SDB_DS_TRANS_PROPAGATE_MODE mode ) ;

   /**
    * Get data source propagation mode accorrding to the description.
    */
   SDB_DS_TRANS_PROPAGATE_MODE sdbDSTransModeFromDesc( const CHAR *descStr ) ;
}

#endif /* UTIL_DATASOURCE_HPP__ */
