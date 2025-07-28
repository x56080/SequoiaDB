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

   Source File Name = aggregate.py

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
import bson
from bson.son import SON
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
         service = int( value )
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

def createCL( cs_name, cl_name ):
   print '---begin to drop cs in ready'
   try:
      db.drop_collection_space( cs_name )
   except SDBBaseError, e:
      if ( -34 != e.code ):         
         raise e
                   
   print '---begin to create cs cl'
   cs = db.create_collection_space( cs_name )     
   cl = cs.create_collection( cl_name, {"ReplSize":0} )
   
   return cl
   
def insertRec( cl ):   
   print '---begin to insert records'
   data1 = {'no':'229095', 'name':'Tom',   'major':'English',  'score':100}
   data2 = {'no':'229096', 'name':'Tina',  'major':'English',  'score':60}
   data3 = {'no':'229095', 'name':'Tom',   'major':'Math',     'score':80}
   data4 = {'no':'229096', 'name':'Tina',  'major':'Math',     'score':70}
   data5 = {'no':'229096', 'name':'Tom',   'major':'Chinese',  'score':60}
   data6 = {               'name':'Tina',  'major':'Chinese',  'score':90}
   cl.insert( data1 )
   cl.insert( data2 )
   cl.insert( data3 )
   cl.insert( data4 )
   cl.insert( data5 )
   cl.insert( data6 )

def aggregate( cl ): 
   print '---begin to aggregate'
   match = SON({'$match':{'no':{'$exists':1}}})
   group = SON({'$group':{'_id':'$major','avg_score':{'$avg':'$score'},'major':{'$first':'$major'}}})
   sort  = SON({'$sort':{'avg_score':1}})
   skip  = {'$skip':1}
   limit = {'$limit':1}
   option = [ match, group, sort, skip, limit ]
   cursor = cl.aggregate( option )
   
   i = 0
   while True:
      try:
         rec = cursor.next()
         i = i + 1
         
         # check field value
         if ( rec['major'] != 'Math' or rec['avg_score'] != 75.0 ):
            print 'expect: {major:"Math", avg_score:75.0}, actual: %s' % ( rec )
            raise  Exception( 'CHECK_FIELD_VALUE_ERROR' )
            
         # check field number
         fieldNum = len(rec)
         if ( fieldNum != 2 ):
            print 'expect: {major:"Math", avg_score:75.0}, actual: %s' % ( rec )
            raise  Exception( 'CHECK_FIELD_NUMBER_ERROR' )   
      except SDBEndOfCursor:
         break
      except ( Exception ), e:
         raise e
         
   # check count      
   if ( i != 1 ):
      print 'expect: return 1 record, actual: return %d record' % ( i )
      raise  Exception( 'COUNT_ERROR' )
      
def clean( cs_name ):
   print '---begin to drop cs in finally'
   try:
      db.drop_collection_space( cs_name )
   except SDBBaseError, e:
      if ( -34 != e.code ):  
         pysequoiadb._print(e.detail)            
         raise e
      
if __name__ == "__main__":   
   try:    
      parse_option()
      
      cs_name = "pydriver_aggregate_cs"
      cl_name = "pydriver_aggregate_cl"
            
      db = client( hostname, service )
      cl = createCL( cs_name, cl_name )
      insertRec( cl )
      aggregate( cl )
   
   except SDBBaseError, e:
      pysequoiadb._print( e.detail )
      raise e  
            
   finally:  
      if( locals().has_key('db') ):                    
         clean( cs_name )     
         db.disconnect()
         del db 