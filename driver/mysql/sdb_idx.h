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
#ifndef SDB_IDX__H
#define SDB_IDX__H

#include "sql_class.h"
#include "include/client.hpp"
#include "sdb_cl_ptr.h"

int sdb_create_index( const KEY *keyInfo, sdb_cl_auto_ptr cl ) ;

int sdb_drop_index( const KEY *keyInfo, sdb_cl_auto_ptr cl ) ;

const char *sdb_get_idx_name( KEY *key_info ) ;

int sdb_get_idx_order( KEY * key_info, bson::BSONObj &order ) ;

int build_match_obj_by_start_stop_key( uint keynr,
                                       const uchar *key_ptr,
                                       key_part_map keypart_map,
                                       enum ha_rkey_function find_flag,
                                       key_range *end_range,
                                       TABLE *table,
                                       bson::BSONObj &matchObj ) ;

#endif
