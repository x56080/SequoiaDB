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

   Source File Name = connection_CRUD.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

"""
#modify: Ting YU 2015-12-11
import pysequoiadb
from pysequoiadb import client
from pysequoiadb import collectionspace
from pysequoiadb import collection
from pysequoiadb import cursor
from pysequoiadb import const
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor)
import sys, getopt
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
   
def clean( cs_name ):
   print '---begin to drop cs in finally'
   try:
      db.drop_collection_space(cs_name)
   except SDBBaseError, e:
      if ( -34 != e.code ):  
         pysequoiadb._print(e.detail)            
         raise e

def connectHost( hostname, service ):
   print '---begin to connect to host'
   hosts = [{'host':hostname,'service':service}]
   db.connect_to_hosts(hosts,user="",password="",policy="random")
   db.connect(hostname,service,user="",password="")
   
def insert( cl ):
   print '---begin to insert'
   
   data = {'age':1,'name':'tom'}
   cl.insert(data)
   
   insert_sql = 'insert into '+cs_name+'.'+cl_name+'(age,name) values(24,\'kate\')'
   db.exec_update(insert_sql)
   
def update( cl ):
   print '---begin to update'
   
   update_sql = 'update '+cs_name+'.'+cl_name+' set name = \'tom_new\' where age = 1'
   db.exec_update(update_sql)
   
   rule = {'$set':{'name':'kate_new'}}
   cond = {'age':{'$et':24}}
   cl.update(rule,condition=cond)

def query( cl ):
   print '---begin to query'
   
   # check count
   cnt = cl.get_count()
   if( 2 != cnt ):
      print 'excute: cl.get_count(), expect: 2, actual: %d' % ( cnt )
      raise  Exception( 'COUNT_ERROR' )   
   
   # check record {name:"kate_new", age:24}
   cond = {'age':{'$et':24}}
   cursor = cl.query( condition=cond )
   while True:
      try:
         record = cursor.next()
         if ( record['name'] != 'kate_new' ):
            print 'excute: cl.query(%s), expect: {name:"kate_new", age:24}, actual: %s' % ( cond, record )
            raise  Exception( 'RECORD_ERROR' )
      except SDBEndOfCursor:
         break
      except ( Exception ), e:
         raise e
         
   # check record {name:"tom_new", age:1}
   select_sql = 'select name from '+cs_name+'.'+cl_name+' where age=1'
   cursor = db.exec_sql(select_sql)    
    
   while True:
      try:
         record = cursor.next()
         if ( record['name'] != 'tom_new' ):
            print 'excute: cl.exec_sql(%s), expect: {name:"tom_new", age:1}, actual: %s' % ( select_sql, record )
            raise  Exception( 'RECORD_ERROR' )
      except SDBEndOfCursor:
         break
      except ( Exception ), e:
         raise e

def delete( cl ):
   print '---begin to delete'
   
   delete_sql = 'delete from '+cs_name+'.'+cl_name +' where age=1'
   db.exec_update(delete_sql)
   
   cond = {'age':{'$et':24}}
   cl.delete(condition=cond)

   cnt = cl.get_count()
   if( 0 != cnt ):
      print 'excute: cl.get_count(), expect: 0, actual: %d' % ( cnt )
      raise  Exception( 'COUNT_ERROR' )   
                              
if __name__ == "__main__":
   try:    
      parse_option() 
      cs_name = "pydriver_crud_cs"
      cl_name = "pydriver_crud_cl"
            
      db = client( hostname, service ) 
      connectHost( hostname, service)     
      cl = createCL( cs_name, cl_name)
      
      insert( cl )
      update( cl )
      query( cl )
      delete( cl )
         
   except SDBBaseError, e:
      pysequoiadb._print(e.detail)
      raise e  
            
   finally:  
      if( locals().has_key('db') ):                    
         clean( cs_name )     
         db.disconnect()
         del db  