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

   Source File Name = version.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
# The autogen will replace the following 4 values.
DB_VERSION="x.x.x"
RELEASE="0"
GIT_VERSION="xxxxxxxxxx"
BUILD_TIME="0000-00-00-00.00.00"
    
def get_version():
    return DB_VERSION
    
def get_release():
    return RELEASE
    
def get_git_version():
    return GIT_VERSION
    
def get_build_time():
    return BUILD_TIME
