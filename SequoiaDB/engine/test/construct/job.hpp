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
*******************************************************************************/

#ifndef JOB_HPP_
#define JOB_HPP_

#include <string>

using namespace std;

enum JOB_TYPE
{
   JOB_TYPE_QUIT = 0,
   JOB_TYPE_INSERT,
   JOB_TYPE_DELETE,
   JOB_TYPE_UPDATE,
   JOB_TYPE_QUERY
};

class job
{
   public:
      job(){}
      ~job(){}
   public:
      JOB_TYPE _type;
      string _cs;
      string _collection;
};

#endif

