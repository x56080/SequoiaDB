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

   Source File Name = impUtil.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impUtil.hpp"
#include "utilCommon.hpp"
#include "pd.hpp"
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace import
{
   UINT32 RC2ShellRC(INT32 rc)
   {
      return engine::utilRC2ShellRC(rc);
   }

   static BOOLEAN _hasFile(vector<string>& files, const string& file)
   {
      for (vector<string>::iterator it = files.begin(); it != files.end(); it++)
      {
         if (fs::equivalent(*it, file))
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   INT32 parseFileList(const string& fileList, vector<string>& files)
   {
      INT32 rc = SDB_OK;

      try
      {
         boost::char_separator<char> hostSep(",");
         typedef boost::tokenizer<boost::char_separator<char> > CustomTokenizer;
         CustomTokenizer fileTok(fileList, hostSep);

         files.clear();

         for (CustomTokenizer::iterator it = fileTok.begin();
              it != fileTok.end(); it++)
         {
            string file = *it;
            file = boost::algorithm::trim_copy_if(file, boost::is_space());
            if (file.empty())
            {
               // ignore empty string or white space
               continue;
            }

            if (!fs::exists(file))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if (fs::is_directory(file))
            {
               fs::directory_iterator it(file);
               fs::directory_iterator file_end;

               for (; it != file_end; it++)
               {
                  if (fs::is_directory(it->status()))
                  {
                     // ignore sub-directory
                     continue;
                  }

                  if (!_hasFile(files, it->path().string()))
                  {
                     files.push_back(it->path().string());
                  }
               }
            }
            else
            {
               if (!_hasFile(files, file))
               {
                  files.push_back(file);
               }
            }
         }
      }
      catch(std::exception& e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "unexpected error happened: %s", e.what());
         goto error;
      }

      if (files.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}
