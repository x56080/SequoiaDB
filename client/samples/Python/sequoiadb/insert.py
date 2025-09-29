"""

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

   Source File Name = insert.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

"""
#! /usr/bin/python

from pysequoiadb import *

if __name__ == "__main__":

   host = "localhost"
   port = 11810
   cs_name = 'sample'
   cl_name = 'employee'

   db = client(host, port)
   try:
      cs = db.create_collection_space(cs_name)
      cl = cs.create_collection(cl_name)

      record1 = {"name": "Tom", "age": 24}
      record2 = {"name": "Jack", "age": 22}
      docs = [record1, record2]

      # case 1: insert
      result1 = cl.insert(record1)
      print(result1)

      # case 2: insert_with_flag
      result2 = cl.insert_with_flag(record2)
      print(result2)

      # case 3: bulk_insert
      result3 = cl.bulk_insert(collection.INSERT_FLG_REPLACEONDUP, docs)
      print(result3)

      # clear
      cs.drop_collection(cl_name)
      db.drop_collection_space(cs_name)
   except SDBBaseError as e:
      print(e)
   finally:
      db.disconnect()
