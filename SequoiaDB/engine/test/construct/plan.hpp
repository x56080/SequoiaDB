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

#ifndef EXECUTIONPLAN_HPP_
#define EXECUTIONPLAN_HPP_

#include <string>
#include "core.hpp"

using namespace std;

const UINT32 SCALE_BASE = 100;

class executionPlan
{
   public:
      executionPlan()
      {
         _collection = "collection";
         _cs = "cs";
         _csNum = 1;
         _collectionNum = 1;
         _insert = 0;
         _update = 0;
         _delete = 0;
         _query = 0;
         _thread = 1;
         _scale = 0;
      }
      ~executionPlan(){}

   public:
      string _host ;
      UINT16 _port ;
      UINT64 _insert;
      UINT64 _update;
      UINT64 _delete;
      UINT64 _query;
      string _cs;
      UINT32 _csNum;
      string _collection;
      UINT32 _collectionNum;
      UINT32 _thread;
      UINT32 _scale;
};


#endif

