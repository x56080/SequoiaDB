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

#ifndef CASERUNNER_HPP_
#define CASERUNNER_HPP_

#include <vector>
#include <boost/thread.hpp>

#include "core.hpp"
#include "plan.hpp"
#include "job.hpp"
#include "statistics.hpp"
#include "ossLatch.hpp"

using namespace std;

namespace sdbclient
{
   class sdb;
}

class caseRunner
{
   public:
      caseRunner();
      ~caseRunner();

   public:
      static void active(caseRunner *runner);

   public:
      INT32 run(executionPlan &plan);

      void join();

   private:
      inline UINT64 _range()
      {
         return _plan._insert + _plan._update +
                _plan._delete + _plan._query;
      }

      inline JOB_TYPE _type(UINT64 rand)
      {
         if (rand < _plan._insert)
         {
            --_plan._insert;
            return JOB_TYPE_INSERT;
         }
         else if (rand < (_plan._insert + _plan._delete))
         {
            --_plan._delete;
            return JOB_TYPE_DELETE;
         }
         else if (rand <
                  (_plan._insert + _plan._delete + _plan._update))
         {
            --_plan._update;
            return JOB_TYPE_UPDATE;
         }
         else if (rand <
                  (_plan._insert + _plan._delete +
                   _plan._update + _plan._query))
         {
            --_plan._query;
            return JOB_TYPE_QUERY;
         }
         else
         {
            return JOB_TYPE_QUIT;
         }
      }

   private:
      INT32 _init(executionPlan &plan);

      void _crun();

      void _getJob(job &j);

      INT32 _insert(const job &j, sdbclient::sdb &conn);

      INT32 _drop(const job &j, sdbclient::sdb &conn);

      INT32 _update(const job &j, sdbclient::sdb &conn);

      INT32 _query(const job &j, sdbclient::sdb &conn);

   private:
      vector<boost::thread *> _consumers;
      executionPlan _plan;
      _ossSpinXLatch _mtx;
      statistics _statistics;
};

#endif

