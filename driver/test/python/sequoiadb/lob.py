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

   Source File Name = lob.py

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
   
def createLobWithOid( cl, oid ):     
   print '---begin to creat lob, specified oid'  
   obj = cl.create_lob( oid )
   data = "1234567891011121314151617181920"
   obj.write( data, 31 )
   obj.close()
   
   print '---begin to creat lob, oid has been existed'
   try:
      obj = cl.create_lob( oid )
   except SDBBaseError, e:
      if ( -5 != e.code ):
         pysequoiadb._print( e.detail )        
         raise e      
     
def createLobWithoutOid( cl ):        
   print '---begin to creat lob, did not specified oid'
   obj = cl.create_lob()
   obj.write( "hello", 5 )
   oid = obj.get_oid()
   obj.close()
   
   return oid
   
def listLob( cl ):
   print '---begin to list lob'
   
   i = 0 ;
   rc = cl.list_lobs()
   while True:
      try:
         lob = rc.next()
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e

   if( i != 2 ):
      print 'return lob number, expect: 2, actual: %d' % ( i )
      raise  Exception( 'CHECK_ERROR' )
      
def getLob( cl, oid ):
   print '---begin to get lob: %s' % ( oid )
   
   obj = cl.get_lob( oid )
   
   # get oid  
   oidGet = obj.get_oid()
   if ( 0 != cmp( oidGet, oid ) ):
      print 'cl.get_lob( %s ), get_oid return %s' % ( oid, oidGet )
      raise  Exception( 'GET_OID_ERROR' )
      
   # read lob
   obj.seek( 11, 0 )
   data = obj.read( 20 ) 
   expData =  "11121314151617181920" 
   if( data != expData ):
      print 'exec: seek(11,0),read(20), expect: %s, atual: %s ' % ( expData, data )
      raise  Exception( 'READ_LOB_ERROR' )
      
   #get lob's size
   size = obj.get_size()
   expSize = 31
   if ( size != expSize ):
      print 'exec: get_size(), expect: %d, atual: %d ' % ( expSize, size )
      raise  Exception( 'GET_SIZE_ERROR' )
     
   #get lob's create time
   time = obj.get_create_time()
   
   obj.close()

def removeLob( cl, oid1, oid2 ):
   print '---begin to remove lob'
   
   cl.remove_lob( oid1 )
   cl.remove_lob( oid2 )
   
   # check
   i = 0 ;
   rc = cl.list_lobs()
   while True:
      try:
         lob = rc.next()
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e

   if( i != 0 ):
      print 'remove all lob, exec: list_lobs(), expect: return 0 lob, actual: %d' % ( i )
      raise  Exception( 'CHECK_ERROR' )
   
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
      
      cs_name = "pydriver_lob_cs"
      cl_name = "pydriver_lob_cl"
            
      db = client( hostname, service )
      cl = createCL( cs_name, cl_name )
      oid1 = bson.ObjectId("5448a5181c3eb9e00b000001")
      createLobWithOid( cl, oid1 )
      oid2 = createLobWithoutOid( cl )
      listLob( cl )
      getLob( cl, oid1 )
      removeLob( cl, oid1, oid2 )
      clean( cs_name )
   
   except SDBBaseError, e:
      pysequoiadb._print( e.detail )
      raise e  
            
   finally:  
      if( locals().has_key('db') ):                    
         clean( cs_name )     
         db.disconnect()
         del db 