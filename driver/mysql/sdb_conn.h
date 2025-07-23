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

   
*******************************************************************************/
#ifndef SDB_CONN__H
#define SDB_CONN__H

#include <map>
#include <mysql/psi/mysql_thread.h>
#include <my_global.h>
#include <atomic_class.h>
#include "include/client.hpp"


class sdb_conn_auto_ptr ;
class sdb_cl_auto_ptr ;
class sdb_conn
{
public:

   sdb_conn( my_thread_id _tid ) ;

   ~sdb_conn() ;

   int connect( ) ;

   sdbclient::sdb & get_sdb() ;

   my_thread_id get_tid() ;

   int begin_transaction() ;

   int commit_transaction() ;

   int rollback_transaction() ;

   bool is_transaction() ;

   int get_cl( char *cs_name, char *cl_name,
               sdb_cl_auto_ptr &cl_ptr,
               bool create = FALSE,
               const bson::BSONObj &options = sdbclient::_sdbStaticObject ) ;

   int create_cl( char *cs_name, char *cl_name,
                  sdb_cl_auto_ptr &cl_ptr,
                  const bson::BSONObj &options = sdbclient::_sdbStaticObject ) ;

   void clear_cl( char *cs_name, char *cl_name ) ;

   void clear_all_cl() ;

   bool is_idle() ;

   int create_global_domain( const char *domain_name ) ;

   int create_global_domain_cs( const char *domain_name, char *cs_name ) ;

private:
   sdbclient::sdb                                  connection ;
   bool                                            transactionon ;
   my_thread_id                                    tid ;
   pthread_rwlock_t                                rw_mutex ;
   std::multimap<std::string, sdb_cl_auto_ptr>     cl_list ;
} ;

#endif
