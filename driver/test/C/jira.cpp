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

   Source File Name = jira.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include "jira.hpp"
#include "client.h"
#include <pthread.h>

using namespace std ;
using namespace boost ;

void createOid()
{
   bson_oid_t oid ;
   bson obj ;
   bson_init( &obj ) ;
   bson_oid_gen( &oid ) ;
   bson_append_oid( &obj, "_id", &oid ) ;
   bson_finish( &obj ) ;
   bson_print( &obj ) ;
   cout << endl ;
   bson_destroy( &obj ) ;
}

TEST_F(jiraTestCase, jira_3123)
{
   thread_group group ;
   int i = 0 ;
   int threadNum = 100 ;
   for( ; i < threadNum; i++ )
   {
      group.create_thread(bind(createOid)) ;
   }
   group.join_all() ;
}

/*
pthread_once_t once = PTHREAD_ONCE_INIT ;
static int a = 0 ;
void once_run()
{
//   cout << "once_run in thread " << (unsigned int)pthread_self() << endl ;
   a = 1 ;
   sleep( 2 ) ;
   cout << "wait up" << endl ;
}

void* child1(void *arg)
{
   pthread_t tid = pthread_self() ;
   cout << "thread " << tid << " enter" << endl ;
   pthread_once( &once, once_run ) ;
   cout << "thread " << tid << " return, and a is: " << a << endl ;
}

void* child2(void *arg)
{
   pthread_t tid = pthread_self() ;
   cout << "thread " << tid << " enter" << endl ;
   pthread_once( &once, once_run ) ;
   cout << "thread " << tid << " return" << endl ;
}

TEST_F(jiraTestCase, DISABLE_test2)
{
   cout << "in test2" << endl;
   cout << "Begin" << endl ;
   pthread_t tid1, tid2, tid3 ;
   pthread_create( &tid1, NULL, child1, NULL ) ;
   pthread_create( &tid2, NULL, child1, NULL ) ;
   pthread_create( &tid3, NULL, child1, NULL ) ;

   sleep( 3 ) ;
   cout << "Finish" << endl ;
   
}
*/



