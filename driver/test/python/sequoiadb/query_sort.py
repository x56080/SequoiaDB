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

   Source File Name = query_sort.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#modify: Ting YU 2015-12-11
import pysequoiadb
from pysequoiadb import client
from pysequoiadb import collectionspace
from pysequoiadb import collection
from pysequoiadb import cursor
from pysequoiadb import const
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor)
import sys,getopt
import traceback

hostname=None
service=None

# parase the args
def parse_option():

   if len( sys.argv ) < 2:
      usage()
      sys.exit(1)
   
   try:  
      opts, args = getopt.getopt( sys.argv[1:], 'hH:p:', ['help'] )
   except getopt.GetoptError, err:
      print str( err )
      usage()
      sys.exit(1)
   
   global hostname, service
   for op, value in opts:
      if op == '-H':
         hostname = value
      elif op == '-p':
         service = int(value)
      elif op in ('-h', '--help'):
         usage()
         sys.exit()
      else:
         print 'arguments error'
         usage()
         sys.exit(1)
         
def usage():
   print 'Command options:'
   print '-h,--help  help'
   print '-H   arg   hostname'
   print '-p   arg   coord_port'

def createCL( cs_name, cl_name):
   print '---begin to drop cs in ready'
   try:
      db.drop_collection_space(cs_name)
   except SDBBaseError, e:
      if ( -34 != e.code ):            
         raise e
                   
   print '---begin to create cs cl'
   cs = db.create_collection_space(cs_name)     
   cl = cs.create_collection(cl_name, {"ReplSize":0})
   
   return cl
   
def insert( cl ):   
   print '---begin to insert records'
   for i in range(1,100):
      cl.insert({'age':i,'name':'mike'+str(i)})
      
def query( cl ):   
   print '---begin to query'
   
   condition = {'age':{'$gt':96}}
   selector = {'age':'','name':''}
   order = {'age':-1}
   cursor = cl.query(condition=condition,selector=selector,order_by=order)
     
   i = 0      
   ageVal = 99
   while True:
      try:
         record = cursor.next()
         if( record['age'] != ageVal ):
            print 'exeute: cl.query, condition=%s, selector=%s, order_by=%s' % (condition, selector, order)     
            print 'expect: {age:%d, name:"mike"%d}, actual: %s' % ( ageVal, ageVal, record )
            raise  Exception( 'CHECK_ERROR' )
         ageVal = ageVal - 1 
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e
   if( i != 3 ):
      print 'exeute: cl.query, condition=%s, selector=%s, order_by=%s' % (condition, selector, order)
      print 'return record number, expect: 3, actual: %d' % ( i )
      raise  Exception( 'COUNT_ERROR' ) 
            
def clean( cs_name ):
   print '---begin to drop cs in finally'
   try:
      db.drop_collection_space(cs_name)
   except SDBBaseError, e:
      if ( -34 != e.code ): 
         pysequoiadb._print(e.detail)             
         raise e
      
if __name__ == "__main__":   
   try:    
      parse_option()
      
      cs_name = "pydriver_query_sort_cs"
      cl_name = "pydriver_query_sort_cl"
            
      db = client( hostname, service )      
      cl = createCL( cs_name, cl_name)
      
      # main
      insert( cl )
      query( cl )
      
   except SDBBaseError, e:
      pysequoiadb._print(e.detail)
      raise e  
            
   finally:  
      if( locals().has_key('db') ):                    
         clean( cs_name )     
         db.disconnect()
         del db 