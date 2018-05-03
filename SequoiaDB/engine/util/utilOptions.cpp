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

   Source File Name = utilOptions.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilOptions.hpp"
#include "utilParam.hpp"
#include "pd.hpp"
#include <iostream>

namespace engine
{
   utilOptions::utilOptions()
   {
      _parsed = FALSE;
   }

   utilOptions::~utilOptions()
   {
      utilOptionGroups::iterator it = _optGroups.begin();
      while (it != _optGroups.end())
      {
         utilOptionGroup* group = *it;
         SDB_OSS_DEL(group);
         it = _optGroups.erase(it);
      }
   }

   utilOptionGroup* utilOptions::_getOptGroup(const string& groupName)
   {
      utilOptionGroup* group = NULL;
      
      for (UINT32 i = 0; i < _optGroups.size(); i++)
      {
         utilOptionGroup* _group = _optGroups.at(i);
         if (groupName == _group->name)
         {
            group = _group;
            break;
         }
      }

      return group;
   }

   utilOptAdd utilOptions::addOptions(const string& groupName, BOOLEAN hidden)
   {
      utilOptionGroup* group = NULL;

      group = _getOptGroup(groupName);
      if (NULL == group)
      {
         group = SDB_OSS_NEW utilOptionGroup(groupName, hidden);
         if (NULL == group)
         {
            throw "Out of memory";
         }
         _optGroups.push_back(group);
      }

      return group->desc.add_options();
   }

   INT32 utilOptions::parse(INT32 argc, CHAR* argv[])
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(!_parsed, "already parsed");

      for (UINT32 i = 0; i < _optGroups.size(); i++)
      {
         utilOptionGroup* group = _optGroups.at(i);
         _allDesc.add(group->desc);
      }

      rc = utilReadCommandLine( argc, argv, _allDesc, _allOpt, FALSE );
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to parse arguments, rc=%d", rc);
         goto error;
      }

      _parsed = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   void utilOptions::print(BOOLEAN printHidden)
   {
      for (UINT32 i = 0; i < _optGroups.size(); i++)
      {
         utilOptionGroup* group = _optGroups.at(i);

         if (group->hidden && !printHidden)
         {
            continue;
         }

         if (!group->name.empty())
         {
            std::cout << group->name << ":" << std::endl;
         }
         std::cout << group->desc << std::endl;
      }
   }

   BOOLEAN utilOptions::has(const string& option)
   {
      SDB_ASSERT(_parsed, "must be parsed");
      SDB_ASSERT(!option.empty(), "empty option");

      return (_allOpt.count(option) > 0);
   }
}
