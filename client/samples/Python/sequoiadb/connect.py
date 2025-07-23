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

   Source File Name = connect.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#! /usr/bin/python

import pysequoiadb
from pysequoiadb import client
from pysequoiadb.error import (SDBTypeError,
                               SDBBaseError,
                               SDBEndOfCursor)

if "__main__" == __name__:

   # connect to local db, using default args value.
   # host= 'localhost', port= 11810, user= '', password= ''
   try:
      db_default = client()
      print( db_default )
      del db_default
   except (SDBTypeError, SDBBaseError) as e:
      print(e)

   # connect to db, using default args value.
   # host= '192.168.20.111', port= 11810, user= '', password= ''
   # 192.168.20.111 is not a valid 
   try:
      db_to_1 = client('192.168.20.48', 11810, '', '')
      del db_to_1
   except (SDBTypeError, SDBBaseError) as e:
      print(e)

   # connect to db, using default args value.
   # host= 'localhost', port= 11810, user= '', password= ''
   try:
      db = client("ubuntu-dev9", 11810)

      host = "192.168.20.48"
      service = '11860'

      # try to connect another db server by service
      db.connect(host, service)
      db.disconnect()
      # try to connect other db server
      hosts = [ {'host':'192.168.20.48', 'service':11810},
                {'host':'192.168.20.111', 'service':50000},
                {'host':'localhost', 'service':11810} ]
      db.connect_to_hosts(hosts, user="", password="")

      # close connection to db server
      db.disconnect()

      # release clinet
      del db

   except (SDBTypeError, SDBBaseError) as e:
      print(e)
