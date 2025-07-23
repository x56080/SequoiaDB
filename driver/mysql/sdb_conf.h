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
#ifndef SDB_CONF__H
#define SDB_CONF__H

#include <mysql/psi/mysql_file.h>
#include "sdb_def.h"

extern PSI_memory_key sdb_key_memory_conf_coord_addrs ;

class sdb_conf
{
public:

   ~sdb_conf() ;

   static sdb_conf *get_instance() ;

   int parse_conn_addrs( const char *conn_addrs ) ;

   char **get_coord_addrs() ;

   int get_coord_num() ;

   const char * get_global_domain_name()
   {
      return "mysql_storage_engine_domain" ;
   }

   void set_use_partition( my_bool val ) ;

   my_bool get_use_partition() ;

private:

   sdb_conf() ;

   sdb_conf(const sdb_conf & rh){}

   sdb_conf & operator = (const sdb_conf & rh) { return *this ;}

   void clear_coord_addrs( int num ) ;

private:
   char                          *pAddrs[SDB_COORD_NUM_MAX] ;
   int                           coord_num ;
   pthread_rwlock_t              addrs_mutex ;
   my_bool                       use_partition ;
} ;

#define SDB_CONF_INST            sdb_conf::get_instance()

#endif
