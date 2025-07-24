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

   Source File Name = backup.py

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

def isStandalone( db ):   
   print '---begin to get cluster mode'   
   is_standalone = False
   try:
      db.list_replica_groups()
   except SDBBaseError, e:
      if ( -159 == e.code ):
         is_standalone = True
      else:
         raise e   
         
   return is_standalone 
   
def getOneRG():
   print '---begin to get 1 data group name'
   rc = db.get_list( 7, condition={'Role': 0} ) 
   info = rc.next()
   dataRG = info['GroupName'] 
         
   return dataRG
   
def ready( backupName ):
   print '---begin to remove backup in ready'
   try:
      db.remove_backup( {'Name': backupName} )
   except SDBBaseError, e:
      if ( -241 != e.code ):         
         raise e
   
def backup( dataRGName, backupName ):   
   print '---begin to backup data group: %s' % (dataRGName)   
   option = {'GroupName': dataRGName, 'Name': backupName, 'OverWrite': True}
   db.backup_offline( option )

   print '---begin to list backup'
   option = {'Name': backupName}
   cursor = db.list_backup( option )
   i = 0        
   while True:
      try:
         record = cursor.next()
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e
   if( i != 1 ):
      print 'exeute: list_backup( %d )' % ( option )
      print 'return record number, expect: 1, actual: %d' % ( i )
      raise  Exception( 'COUNT_ERROR' )     
   
   print '---begin to remove backup: %s' % (backupName)
   db.remove_backup( {'Name': backupName} )
   
   option = {'Name': backupName}
   cursor = db.list_backup( option )
   i = 0        
   while True:
      try:
         record = cursor.next()
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e
   if( i != 0 ):
      print 'exeute: remove_backup, list_backup( %s )' % ( option )
      print 'return record number, expect: 0, actual: %d' % ( i )
      raise  Exception( 'CHECK_ERROR' )      
      
def clean( backupName ):
   print '---begin to remove backup in clean'
   try:
      db.remove_backup( {'Name': backupName} )
   except SDBBaseError, e:
      if ( -241 != e.code ):
         pysequoiadb._print(e.detail)           
         raise e
      
if __name__ == "__main__":   
   try:    
      parse_option()
      
      backupName = 'pydriver_dataRG_backup'
            
      db = client( hostname, service )
      ready( backupName )
      
      # main
      if( isStandalone( db ) == True ):
         print 'Mode is standalone!'
         exit(0) 
      else:    
         dataRGName = getOneRG()
         backup( dataRGName, backupName )
      
   except SDBBaseError, e:
      pysequoiadb._print( e.detail )
      raise e  
            
   finally:  
      if( locals().has_key('db') ):                    
         clean( backupName )    
         db.disconnect()
         del db 