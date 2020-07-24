//
// Created by yangshangde on 7/23/20.
//

#ifndef SEQUOIADB_COORDSDBDATASOURCEDEF_HPP
#define SEQUOIADB_COORDSDBDATASOURCEDEF_HPP


#define DATASOURCE_NAME_SDB            "SequoiaDB"

namespace engine
{
   typedef INT32 (*DATASOURCE_LOAD_ENTRY)() ;
   typedef utilMap< const CHAR*, DATASOURCE_LOAD_ENTRY >
         SDB_DATASOURCE_ENTRY_MAP ;

   extern SDB_DATASOURCE_ENTRY_MAP



}

#endif //SEQUOIADB_COORDSDBDATASOURCEDEF_HPP
