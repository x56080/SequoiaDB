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
#ifndef SDB_CONDITION__H
#define SDB_CONDITION__H

#include "sdb_item.h"

enum sdb_cond_status
{
   sdb_cond_supported = 1,
   sdb_cond_partsupported,
   sdb_cond_beforesupported,
   sdb_cond_unsupported,

   sdb_cond_unknown = 65535
} ;

class sdb_cond_ctx : public Sql_alloc
{
public:

   sdb_cond_ctx() ;

   ~sdb_cond_ctx() ;

   void push( Item *cond_item ) ;

   void pop() ;

   void pop_all() ;

   void clear() ;

   sdb_item *create_sdb_item( Item_func *cond_item ) ;

   int to_bson( bson::BSONObj &obj ) ;

   void update_stat( int rc ) ;

   bool keep_on() ;

   sdb_item                   *cur_item ;
   List<sdb_item>             item_list ;
   sdb_cond_status            status ;
} ;

void sdb_parse_condtion( const Item *cond_item, sdb_cond_ctx *sdb_cond ) ;


#endif
