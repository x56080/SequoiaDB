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

   Source File Name = py3include.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
# find python3 C API include directory
import os
import platform
import sys


if sys.version_info[0] != 3:
    raise Exception("not python3")

python_include = ""
python_lib = ""

_os = platform.system()
if _os == 'Linux':
    ver = sys.version_info[0]
    sub = sys.version_info[1]
    python_include = sys.prefix + '/include/' + ("python%d.%d" % (ver, sub))
    if not os.path.exists(python_include):
        python_include += "m"
        if not os.path.exists(python_include):
            raise Exception("can't find python3 include directory")
    python_lib = sys.prefix + '/lib/' + ("python%d.%d" % (ver, sub))
elif _os == 'Windows':
    python_include = sys.prefix + "/include/"
    python_lib = sys.prefix + '/libs/'

print(python_include)
print(python_lib)
