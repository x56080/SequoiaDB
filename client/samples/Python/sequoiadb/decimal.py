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

   Source File Name = decimal.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

"""
import pysequoiadb
from pysequoiadb import client
from pysequoiadb.error import SDBEndOfCursor

import bson
from bson import Decimal


if '__main__' == __name__:

   d = Decimal("12345.678909876543212345678909877654321", 100, 30)

   dmx = Decimal(0)
   dmx.set_max()

   dmn = Decimal(0)
   dmn.set_min()

   dzero = Decimal(0)
   dzero.set_zero()

   dn = Decimal(0)

   doc = { "name":"hello", "key": d }

   docs = [
      { "name":"world", "key": dmx },
      { "name":"fuck", "key": dmn },
      { "name":"the", "key": dzero },
      { "name":"decimal", "key": dn },
   ]

   db = client("localhost", 50000)

   db.create_collection_space("foo")

   db.foo.create_collection("bar")

   db.foo.bar.insert(doc)
   print('insert a doc: %r' % doc)
   db.foo.bar.bulk_insert(0, docs)
   print('insert: %r' % docs)


   cr = db.foo.bar.query()

   try:
      while True:
         record = cr.next()
         print(record)
         dd = record['key']
         print('is max : %r'  % dd.is_max())
         print('is min : %r'  % dd.is_min())
         print('is zero : %r' % dd.is_zero())
   except SDBEndOfCursor:
      print("find all record")

   db.drop_collection_space("foo")
