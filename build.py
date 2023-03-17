############################################
#decription:
#   compile source code, include:
#   engineer, driver, connector
############################################

import os,sys
import getopt
import commands
import platform
import shutil

WINDOWS = 'windows'
LINUX = 'linux'

def help_info():
   print ('usage: python build.py')
   print ('')
   print ('')
   print ('   --dd                     debug build no optimization')
   print ('   --release                release build')
   print ('   --enterprise             edition type: enterprise, default: community')
   print ('   -h, --help               show this help info')
   print ('')
   print ('')
   print ('Examples:')
   print ('   python build.py --dd')
   print ('')
   print ('')

def run_in_dir( cmd, dir ):
   os.chdir( dir )
   rs = os.system( cmd )
   os.chdir( cur_dir )
   return rs

def err_exit( rs, err_msg ):
   if rs != 0:
      print( err_msg )
      sys.exit(1)

def guess_os():
   system_type = platform.system()
   if system_type == 'Windows' or system_type == 'Microsoft':
      return WINDOWS
   elif system_type == 'Linux':
      return LINUX
   else:
      err_exit( 1, 'The platform is not supported!' )

def get_file_name( src_dir, ends , key_word , flag=True):
   for root_path, dir_list, file_list in os.walk( src_dir ):
      for f_name in file_list:
         if not f_name.endswith(ends):
            continue
         if flag:
            if key_word in f_name:
               return f_name
         else:
            if key_word not in f_name:
               return f_name
   return ''

################### begin #################
scrpt_path = sys.path[0]
os_type = guess_os()
cur_dir = os.getcwd()
code_path = scrpt_path
build_type = ''
edition_type = ''

short_args = 'h'
long_args = ['help', 'dd', 'release', 'enterprise']
try:
   opts, args = getopt.getopt(sys.argv[1:], short_args, long_args )
except getopt.GetoptError:
   help_info()
   sys.exit(1)

for opt, arg in opts:
   if opt in ('-h', '--help'):
      help_info()
      sys.exit(0)
   elif opt == '--dd':
      build_type = opt
   elif opt == '--release':
      build_type = ''
   elif opt == '--enterprise':
      edition_type = opt   
   else:
      help_info()
      sys.exit(1)

build_cmd_pre = 'scons' + ' ' + build_type

#compile engine
build_all_cmd = build_cmd_pre  + ' ' + edition_type + ' ' + '--all -j 4'
rs = run_in_dir( build_all_cmd, code_path )
err_exit( rs, 'Failed to compile engine!')

#compile java driver
java_code_path = os.path.join( code_path, 'driver', 'java')
build_java_cmd = 'mvn clean package -Dmaven.test.skip=true'
rs = run_in_dir( build_java_cmd, java_code_path )
err_exit( rs, 'Failed to compile Java driver!' )

#compile fdw
if os_type == LINUX:
   rs = os.system( "pg_config --version" )
   if rs == 0:
      fdw_code_path = os.path.join( code_path, 'driver', 'postgresql' )
      rs = run_in_dir( 'make clean', fdw_code_path )
      err_exit( rs, 'Failed to compile fdw in make clean')
      rs = run_in_dir( 'make local', fdw_code_path )
      err_exit( rs, 'Failed to compile fdw in make local')
      rs = run_in_dir( 'make all',   fdw_code_path )
      err_exit( rs, 'Failed to compile fdw in make all')
   else:
      print("Warning: unable to find postgresql, so skip fdw compilation!")

#compile php
php_code_path = os.path.join( code_path, 'driver', 'php' )
build_php_cmd_common = build_cmd_pre + ' --phpversion='
php_ver_file_path = ''
if os_type == LINUX:
   php_ver_file_path = os.path.join( php_code_path, 'php_ver_linux.list' )
   php_ver_file_obj = open( php_ver_file_path )
   try:
      while 1:
         line = php_ver_file_obj.readline()
         if not line:
            break
         build_php_cmd = build_php_cmd_common + line
         rs = run_in_dir( build_php_cmd, php_code_path )
         err_exit( rs, 'Failed to compile PHP driver!' )
   finally:
      php_ver_file_obj.close()

#compile python
build_python_cmd = build_cmd_pre
python_code_path = os.path.join( code_path, 'driver', 'python' )
rs = run_in_dir( build_python_cmd, python_code_path )
err_exit( rs, 'Failed to compile python driver!')

#compile python3
rs = os.system('python3 --version')
if rs == 0:
   build_python_cmd = build_cmd_pre + ' --py3'
   python_code_path = os.path.join( code_path, 'driver', 'python' )
   rs = run_in_dir( build_python_cmd, python_code_path )
   err_exit( rs, 'Failed to compile python3 driver!')
else:
   print("Warning: python3 command not found, so skip python3 compilation!")

#compile C# driver
if os_type == WINDOWS:
   c_sharp_code_path = os.path.join( code_path, 'driver', 'C#.Net' )
   build_c_sharp_cmd = build_cmd_pre
   rs = run_in_dir( build_c_sharp_cmd, c_sharp_code_path )
   err_exit( rs, 'Failed to compile C# driver!' )

#hive, hadoop connector need sequoaidb.jar, so get it
jar_name = 'sequoiadb.jar'
jar_dir = os.path.join( java_code_path, 'target' )
file_name = get_file_name( jar_dir, '.jar', 'javadoc', False )
if file_name.strip() == '':
   err_exit( 1, 'Not found jar file in ' + jar_dir )

src_file = os.path.join( jar_dir, file_name)
target_file = os.path.join( java_code_path, jar_name)
try:
   shutil.copy( src_file, target_file )
except:
   err_exit( 1, 'Failed to get ' + target_file )

#compile hive
if os_type == LINUX:
   hive_code_path = os.path.join( code_path, 'driver', 'hadoop', 'hive' )
   rs = run_in_dir( 'ant', hive_code_path )
   err_exit( rs, 'Failed to compile hive!')

#compile hadoop connector 
hdcn_code_path = os.path.join( code_path, 'driver', 'hadoop', 'hadoop-connector' )
rs = run_in_dir( 'ant -Dhadoop.version=1.2', hdcn_code_path )
err_exit( rs, 'Failed to compile hadoop connector 1.2!')
rs = run_in_dir( 'ant -Dhadoop.version=2.2', hdcn_code_path )
err_exit( rs, 'Failed to compile hadoop connector 2.2!')

#compile spark 
#TODO: zichuan has new compile 

print( 'Compile source code completed!' )
