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

   Source File Name = cmdUsrOmaUtil.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/12/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

namespace engine
{
   #define SPT_OMA_REL_PATH            SDBCM_CONF_DIR_NAME OSS_FILE_SEP
   #define SPT_OMA_REL_PATH_FILE       SPT_OMA_REL_PATH SDBCM_CFG_FILE_NAME

   /*
      define config
   */
   #define MAP_CONFIG_DESC( desc ) \
      desc.add_options() \
      ( SDBCM_RESTART_COUNT, po::value<INT32>(), "" ) \
      ( SDBCM_RESTART_INTERVAL, po::value<INT32>(), "" ) \
      ( SDBCM_AUTO_START, po::value<string>(), "" ) \
      ( SDBCM_DIALOG_LEVEL, po::value<INT32>(), "" ) \
      ( SDBCM_ENABLE_WATCH, po::value<string>(), "" ) \
      ( "*", po::value<string>(), "" )

}