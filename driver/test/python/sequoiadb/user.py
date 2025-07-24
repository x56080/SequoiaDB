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

   Source File Name = user.py

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
   
def isStandalone( db ):   
   print '---begin to check cluster mode'
   
   is_standalone = False
   try:
      db.list_replica_groups()
   except SDBBaseError, e:
      if ( -159 == e.code ):
         is_standalone = True
      else:
         raise e   
         
   return is_standalone 
        
def clean( username, password ):
   print '---begin to remove user in finally'      
   try:      
      db.remove_user( username, password )
   except SDBBaseError, e:
      if ( -6 != e.code and -300 != e.code ): #standalone throw -6  
         pysequoiadb._print(e.detail)          
         raise e 
         
def user( username, password ):
   print '---begin to create new user'             
   db.create_user(username,password)
   
   print '---begin to login by new user'          
   db_new = client(hostname,service,username,password)
   
   print '---begin to remove new user'             
   db.remove_user(username,password)
      
if __name__ == "__main__":   
   try:    
      parse_option()     
      username = 'pydriver_user'
      password = 'pydriver_passwd'
       
      db = client(hostname,service)
      if ( isStandalone( db ) == True ):
         exit(0)
      else:         
         user( username, password )
         
   except SDBBaseError, e:
      pysequoiadb._print(e.detail)
      raise e  
            
   finally:  
      if( locals().has_key('db') ):                    
         clean( username, password )
         db.disconnect()
         del db 