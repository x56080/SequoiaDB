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

import time
import random
import sys

from bson.objectid import ObjectId
import pysequoiadb
from pysequoiadb import client
from pysequoiadb import const
from pysequoiadb.error import (SDBTypeError,
                               SDBBaseError,
                               SDBEndOfCursor)

if __name__ == "__main__":

   if len(sys.argv) != 4:
      print('usage: insert.py <start_batch> <batches> <record_per_batch>')
      sys.exit(1)

   start_batch = int(sys.argv[1].strip())
   batches = int(sys.argv[2].strip())
   record_per_batch = int(sys.argv[3].strip())

   if start_batch < 0 or batches <= 0 or record_per_batch <= 0:
      print("invalid parameter!")
      sys.exit(1)

   try:
      db = client("localhost", 11810)

      cs_name = "foo"
      cs = db.get_collection_space(cs_name)

      cl_name = "bar"
      cl = cs.get_collection(cl_name)

      inner_obj = {"x":"abcdefghijklmnopqrstuvwxyz#1234567890", "y":"1234567890", "z":"abcdefghijklmnopqrstuvwxyz#abcdefghijklmnopqrstuvwxyz"}

      # insert records
      print("start_batch=" + str(start_batch) + ", batches=" + str(batches) + ", record_per_batch=" + str(record_per_batch))
      print("start...")
      start_time = time.time()
      for batch in range(start_batch, start_batch + batches):
         records = []
         for idx in range(0, record_per_batch):
            obj = {"i":batch * record_per_batch + idx, "obj":inner_obj, "int":random.randint(0, record_per_batch * batches)}
            records.append(obj)
         begin_time = time.time();
         cl.bulk_insert(1, records)
         end_time = time.time();
         print("batch " + str(batch) + ": " + str(end_time - begin_time) + " seconds.")
      finish_time = time.time()
      print("total: " + str(finish_time - start_time) + " seconds.")

      # drop collection
      del cl
      # drop collection space
      del cs

      db.disconnect()
      del db

      print("Success")

   except (SDBTypeError, SDBBaseError), e:
      pysequoiadb._print(e)
