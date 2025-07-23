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

   Source File Name = collectionspace.py

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

if __name__ == "__main__":

   try:
      # connect to local db, using default args value.
      # host='localhost', port= 11810, user= '', password= ''
      db = client("192.168.20.48", 11810)
      # create collection space
      # try to get a collection space named by cs_name specified
      cs_name = 'subject'
      cs = db.create_collection_space( cs_name )

      # 1.get collection space
      # try to get a collection space named by cs_name specified
      cs = db.get_collection_space( cs_name )

      # 2.get collection space
      # try to get a collection space named by 'sports' use __getitem__
      cs = db[cs_name]
      print("get collection space:[%s] success" % cs_name)

      # 3.get collection space
      # try to get a collection space named by 'sports' use __getattri__
      cs = db.subject
      print("get collection space:[%s] success." % 'subject')

      # release
      cs_name = cs.get_collection_space_name()
      db.drop_collection_space(cs_name)
      del cs

      del db

   except (SDBTypeError, SDBBaseError) as e:
      print(e)
