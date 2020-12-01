#! /usr/bin/python
# -*- coding: utf-8 -*-
# @Author Nie Zhibiao

# Copyright (c) 2018, SequoiaDB and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

import os
import sys
import re
import time
import base64
import random
import signal
import socket
import datetime
import operator
import optparse
import ConfigParser
import logging.config
import pysequoiadb
from getpass import getpass
from pysequoiadb import client
from collections import OrderedDict
from pysequoiadb.collection import INSERT_FLG_CONTONDUP
from pysequoiadb.errcode import *
from pysequoiadb.error import (SDBTypeError,
                               SDBBaseError)

reload(sys)
sys.setdefaultencoding('utf8')

DESCRIPTION = '''%prog is a sdbaudit exporter server.'''

USAGE = "%prog -t <sdb|mysql|mariadb> --auditpath[=]AUDITPATH " \
        "--clname[=]CS.CL [OPTION]..."

PROCESS_WAIT_TIME = 3

KW_SSL = 'ssl'
KW_USER = 'user'
KW_ADDR = 'addr'
KW_ROLE = 'role'
KW_DELETE = 'delete'
KW_CL_NAME = 'clname'
#KW_CIPHER = 'cipher'
#KW_TOKEN = 'token'
#KW_CIPHERFILE = 'cipherfile'
KW_PASSWD = 'password'
KW_MONITOR = 'options'
KW_NODE_NAME = 'nodename'
KW_INST_NAME = 'instname'
KW_INSERT_NUM = 'insertnum'
KW_AUDIT_PATH = 'auditpath'
KW_PASSWD_TYPE = 'password_type'

FIELD_ID = 'ID'
FIELD_PID = 'PID'
FIELD_TID = 'TID'
FIELD_ROLE = 'Role'
FIELD_TYPE = 'Type'
FIELD_FROM = 'From'
FIELD_ACTION = 'Action'
FIELD_RESULT = 'Result'
FIELD_CLIENT = 'Client'
FIELD_MESSAGE = 'Message'
FIELD_SVC_NAME = 'SvcName'
FIELD_USER_NAME = 'UserName'
FIELD_HOST_NAME = 'HostName'
FIELD_TIMESTAMP = 'Timestamp'
FIELD_OBJECT_TYPE = 'ObjectType'
FIELD_OBJECT_NAME = 'ObjectName'
FIELD_CONNECT_ID = 'ConnectionID'
FIELD_OPERATION_ID = 'OperationID'

LOG_TYPE_MYSQL = 'mysql'
LOG_TYPE_SDB = 'sdb'
LOG_TYPE_MARIADB = 'mariadb'
PID_FILE_NAME='sdbaudit.pid'
CONFIG_FILE_NAME = "sdbaudit.conf"
LOG_CONF_FILE_NAME='sdbaudit_log.conf'
STATUS_FILE_NAME = 'sdbaudit.status'

MY_HOME = os.path.abspath(os.path.dirname(__file__))
MY_CONF_PATH = os.path.join(MY_HOME, 'conf')


def pid_exist(pid):
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True

def string_to_bool(str):
    if str.lower() == 'true':
        return True
    else:
        return False
    
class CryptoUtil:
    def __init__(self):
        pass

    @classmethod
    def encrypt(cls, source_str):
        random_choice = ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "1234567890!@#$%^&*()")
        to_encrypt_arr = []
        shift_str = ""
        for char in source_str:
            shift_str = shift_str + chr(ord(char) + 3)
        shift_index = 0
        for index in range(0, len(shift_str) * 3):
            if index % 3 != 0:
                rand_char = random.choice(random_choice)
                to_encrypt_arr.append(rand_char)
            else:
                to_encrypt_arr.append(shift_str[shift_index])
                shift_index = shift_index + 1
        to_encrypt_str = ''.join(to_encrypt_arr)
        encrypt_str = base64.b64encode(to_encrypt_str)
        return encrypt_str

    @classmethod
    def decrypt(cls, encrypt_str):
        decrypt_str = base64.b64decode(encrypt_str)
        shift_str = []
        for index in range(len(decrypt_str)):
            if index % 3 == 0:
                shift_str.append(decrypt_str[index])
        source_arr = []
        for char in shift_str:
            source_arr.append(chr(ord(char) - 3))
        source_str = "".join(source_arr)
        return source_str

class Logger:
    def __init__(self):
        self.logger = None

    def init(self, log_config_file):
        # Get the log file path from the log configuration file, and create
        # the directory if it dose not exist.
        config_parser = ConfigParser.ConfigParser()
        files = config_parser.read(log_config_file)
        if len(files) != 1:
            print("[ERROR] Read log configuration file '{}' failed".format(log_config_file))
            return 1
        logging.config.fileConfig(log_config_file)
        self.logger = logging.getLogger("logExporter")
        return 0

    def get_logger(self):
        return self.logger

class OptionArgs():
    def __init__(self, options):
        self.__options = options

    def get_log_type(self):
        return self.__options.log_type.lower()

    def get_audit_path(self):
        return self.__options.audit_path

    def get_is_del_file(self):
        return string_to_bool(self.__options.delete)

    def get_host(self):
        return self.__options.addr.split(":")[0]

    def get_port(self):
        return self.__options.addr.split(":")[1]

    def get_user_name(self):
        return self.__options.user_name

    def get_passwd(self):
        return self.__options.passwd

    #def get_cipher(self):
    #    return string_to_bool(self.__options.cipher)

    #def get_token(self):
    #    return self.__options.token

    #def get_cipher_file(self):
    #    return self.__options.cipher_file

    def get_use_ssl(self):
        return string_to_bool(self.__options.use_ssl)

    def get_cl_full_name(self):
        return self.__options.cl_name
    
    def get_cs_name(self):
        return self.__options.cl_name.split(".")[0]

    def get_cl_name(self):
        return self.__options.cl_name.split(".")[1]

    def get_insert_num(self):
        return self.__options.insert_num

    def get_role(self):
        return self.__options.role

    def get_node_name(self):
        return self.__options.node_name

    def get_audit_log_name(self):
        if self.get_log_type() == LOG_TYPE_SDB:
            return "sdbaudit.log"
        else:
            return "server_audit.log"

class OptionsMgr:
    def __init__(self, program):
        self.__parser = optparse.OptionParser(
        description = DESCRIPTION,
        usage = USAGE,
        prog = program
        )
        self.__options = {
            KW_MONITOR: {
                KW_USER : '',
                KW_PASSWD : '',
                KW_PASSWD_TYPE : 0,
                KW_ADDR : 'localhost:11810',
                #KW_CIPHER : 'false',
                #KW_TOKEN : '',
                #KW_CIPHERFILE : './passwd',
                KW_AUDIT_PATH : '',
                KW_SSL : 'false',
                KW_DELETE : 'true',
                KW_INSERT_NUM : 1000,
                KW_CL_NAME : 'SDBSYS_GLOBAL_AUDIT_LOG_CS.AUDIT_CL',
                KW_ROLE : '',
                KW_NODE_NAME : '',
                KW_INST_NAME : ''
            }
        }
        self.__global_parser = None
        self.args = None

#    def __show_version(self):
#        file = os.path.join(MY_HOME, "version.info")
#        try:
#            with open(file, 'r') as f:
#                print(f.read())
#            sys.exit(0)
#        except IOError: 
#            print("[ERROR] Failed to show version. Make ensure the " \
#                  "version.info exists")
#            sys.exit(1)

    def __add_option(self):
        #Add -t option
        self.__parser.add_option("-t", "--type", action='store', type="string",
                                 dest='log_type', help="audit log type. " \
                                 "Option: sdb, mysql, mariadb")
        #Add -c option
        self.__parser.add_option("-c", "--conf", action='store', type="string",
                                 dest='conf_path', help="config file path")
        #Add --path option
        self.__parser.add_option("--auditpath", action='store', type="string",
                                 dest='audit_path', help="audit log file " \
                                 "paths, which must be a directory")
        #Add --delete option
        self.__parser.add_option("--delete", action='store', type='string',
                                 dest="delete", help="whether to remove the " \
                                 "audit log file that has finished " \
                                 "exporting, only the full file is deleted, " \
                                 "(arg: [true|false]")
        #Add --addr option
        self.__parser.add_option("--addr", action='store', type='string',
                                 dest="addr", help="host " \
                                 "addressed(hostname:svcname), default: " \
                                 "'localhost:11810'")
        #Add --user option
        self.__parser.add_option("-u", "--user", action='store', type='string',
                                 dest="user_name", help="username")
        #Add --password option
        self.__parser.add_option("-w", "--password", action='store',
                                 type='string', dest="passwd", help="password")
        #Add --password_type option
        self.__parser.add_option("--w_type", action='store', type='int',
                                 dest="passwd_type", help="password type. " \
                                 "'0' for plaintext, '1' for ciphertext")
        #Add --cipher option
        #self.__parser.add_option("--cipher", action='store', type='string',
        #                         dest="cipher", help="input password using a " \
        #                        "cipher file, (arg: [true|false]")
        #add --token option
        #self.__parser.add_option("--token", action='store', type='string',
        #                         dest="token", help="password encryption token")
        #Add --cipherfile option
        #self.__parser.add_option("--cipherfile", action='store', type='string',
        #                         dest="cipher_file", help="input password " \
        #                         "using a cipher file")
        #Add --ssl option
        self.__parser.add_option("--ssl", action='store', type='string',
                                 dest="use_ssl", help="set SSL connection " \
                                 "(arg: [true|false]")
        #Add --clname option
        self.__parser.add_option("--clname", action='store', type='string',
                                 dest="cl_name", help="collection full name")
        #Add --insertnum option
        self.__parser.add_option("--insertnum", action='store', type='int',
                                 dest="insert_num", help="maximum number of " \
                                 "records per bulk insert")
        #Add --role option
        self.__parser.add_option("--role", action='store', type='string',
                                 dest="role", help="node role, sdb:[coord, " \
                                 "catalog, data, om, standalone], mysql, mariadb")
        #Add --nodename option
        self.__parser.add_option("--nodename", action='store', type='string',
                                 dest="node_name", help="node name, " \
                                 "<hostname>:<svcname>")
#        #Add --version option
#        self.__parser.add_option("-v", "--version", action='store_true',
#                                 dest="version", help="show version")

    def __load_global_configs(self):
        file = os.path.join(MY_CONF_PATH, CONFIG_FILE_NAME)
        if not os.path.exists(file):
            print("[ERROR] Configuration file {} dose not " \
                  "exists".format(CONFIG_FILE_NAME))
            return 1
        self.__global_parser = ConfigParser.ConfigParser()
        self.__global_parser.read(file)
        for section, options in self.__options.iteritems():
            for option, value in options.iteritems():
                if not self.__global_parser.has_option(section, option):
                    if not self.__global_parser.has_section(section):
                        self.__global_parser.add_section(section)
                    self.__global_parser.set(section, option, str(value))
        return 0

    def parse_option(self):
        self.__add_option()
        options,args = self.__parser.parse_args()
#        if options.version:
#          self.__show_version()
        if not options.log_type:
            print("[ERROR] Log type can not be null. Please use -t to specify.")
            return 1

        log_type = options.log_type.lower()
        if log_type != LOG_TYPE_SDB and log_type != LOG_TYPE_MYSQL and log_type != LOG_TYPE_MARIADB:
            print("[ERROR] Invalid log type. Please use 'sdb','mysql' or " \
                  "'mariadb' instead.")
            return 1
        if options.conf_path:
            if not os.path.isfile(options.conf_path):
                print("[ERROR] -c '{}' is not a file".format(options.conf_path))
                return 1
            rc = self.__load_global_configs()
            if 0 != rc:
                return rc
            local_parser = ConfigParser.ConfigParser()
            local_parser.read(options.conf_path)
            for section in self.__global_parser.sections():
                for option in self.__global_parser.options(section):
                    if not local_parser.has_option(section, option):
                        if not local_parser.has_section(section):
                            local_parser.add_section(section)
                        local_parser.set(section, option, 
                                        self.__global_parser.get(section, option))
            if not options.audit_path:
                options.audit_path = local_parser.get(KW_MONITOR, KW_AUDIT_PATH)
            if not options.delete:
                options.delete = local_parser.get(KW_MONITOR, KW_DELETE)
            if not options.addr:
                options.addr = local_parser.get(KW_MONITOR, KW_ADDR)
            if not options.user_name:
                options.user_name = local_parser.get(KW_MONITOR, KW_USER)
            if not options.passwd:
                options.passwd = local_parser.get(KW_MONITOR, KW_PASSWD) 
            if not options.passwd_type:
                options.passwd_type = int(local_parser.get(KW_MONITOR,
                                                           KW_PASSWD_TYPE)) 
#            if not options.cipher:
#                options.cipher = local_parser.get(KW_MONITOR, KW_CIPHER)
#            if not options.token:
#                options.token = local_parser.get(KW_MONITOR, KW_TOKEN)
#            if not options.cipher_file:
#                options.cipher_file = local_parser.get(KW_MONITOR,
#                                                       KW_CIPHERFILE)
            if not options.use_ssl:
                options.use_ssl = local_parser.get(KW_MONITOR, KW_SSL)
            if not options.cl_name:
                options.cl_name = local_parser.get(KW_MONITOR, KW_CL_NAME)
            if not options.insert_num:
                options.insert_num = int(local_parser.get(KW_MONITOR, KW_INSERT_NUM))
            if not options.role:
                options.role = local_parser.get(KW_MONITOR, KW_ROLE)
            if not options.node_name:
                options.node_name = local_parser.get(KW_MONITOR, KW_NODE_NAME)

        if len(options.audit_path) == 0:
            print("[ERROR] Auditpath can not be null. Please use --path to " \
                  "specify.")
            return 1
        elif not os.path.isdir(options.audit_path):
            print("[ERROR] Auditpath must be a directory")
            return 1
        if options.passwd_type == 1:
            options.passwd = CryptoUtil.decrypt(options.passwd)
        elif options.passwd_type == 0:
            passwd = CryptoUtil.encrypt(options.passwd)
            new_local_parser = ConfigParser.ConfigParser()
            new_local_parser.read(options.conf_path)
            new_local_parser.set(KW_MONITOR, KW_PASSWD, passwd)
            new_local_parser.set(KW_MONITOR, KW_PASSWD_TYPE, '1')
            new_local_parser.write(open(options.conf_path, 'w'))

        self.args = OptionArgs(options)
        return 0

class StatMgr:
    """ Status manager is responsible for loading, reading and updating the
        status file of the reporter work
    """
    def __init__(self, stat_file):
        self.parser = None
        self.stat_file = stat_file
        self.file_inode = 0
        self.last_parsed_row = 0

    def __init_stat_file(self):
        self.file_inode = 0
        self.set_last_parsed_row(0)
        self.update_stat()

    def load_stat(self):
        self.parser = ConfigParser.ConfigParser()
        try:
            if not os.path.exists(self.stat_file):
                # No status file at all. Treat as fresh start.
                logger.warn('Status file {} dose not exist. Init it with '
                            'default values'.format(self.stat_file))
                self.__init_stat_file()

            self.parser.read(self.stat_file)
            stat_sec_name = 'status'
            self.file_inode = int(self.parser.get(stat_sec_name, 'file_inode'))
            self.last_parsed_row = int(self.parser.get(stat_sec_name,
                                                      'last_parsed_row'))
        except Exception as error:
            logger.error('Load status failed: ' + str(error))
            return 1

    def get_file_inode(self):
        return self.file_inode

    def set_file_inode(self, file_inode):
        self.file_inode = file_inode

    def get_last_parsed_row(self):
        return self.last_parsed_row

    def set_last_parsed_row(self, row):
        self.last_parsed_row = row

    def update_stat(self):
        stat_sec_name = 'status'
        if not self.parser.has_section(stat_sec_name):
            self.parser.add_section(stat_sec_name)

        self.parser.set(stat_sec_name, 'file_inode', self.file_inode)
        self.parser.set(stat_sec_name, "last_parsed_row", self.last_parsed_row)
        self.parser.write(open(self.stat_file, 'w'))

class SdbConnect:
    
    def __init__(self, args):
        self.args = args
        self.__host = self.args.get_host()
        self.__port = self.args.get_port()
        self.__user = self.args.get_user_name()
        self.__passwd = self.args.get_passwd()
        self.__cs_name = self.args.get_cs_name()
        self.__cl_name = self.args.get_cl_name()
        self.__use_ssl = self.args.get_use_ssl()
        self.__has_connect = False
        self.__connection = None
        self.__cl = None

    def ensure_cl(self):
        index_name = 'UniqueID'
        index = OrderedDict([('ID', 1), ('NodeName', 1)])
        self.__connection = client(self.__host, self.__port, self.__user,
                                   self.__passwd, self.__use_ssl)
        self.__has_connect = True
        while True:
            try:
                cs = self.__connection.get_collection_space(self.__cs_name)
                self.__cl = cs.get_collection(self.__cl_name)
                if not self.__cl.is_index_exist(index_name):
                    self.__cl.create_index(index, index_name, True)
                break
            except (SDBTypeError, SDBBaseError) as e:
                if SDB_DMS_CS_NOTEXIST == get_errcode(e.code):
                    self.__connection.create_collection_space(self.__cs_name)
                elif SDB_DMS_NOTEXIST == get_errcode(e.code):
                    self.__cl = cs.create_collection(self.__cl_name)
                    self.__cl.create_index(index, index_name, True)
                    break

    def write_row(self, records):
        self.__cl.bulk_insert(INSERT_FLG_CONTONDUP, records)
        
    def __del__(self):
        if self.__has_connect:
            self.__connection.disconnect()
            self.__has_connect = False

class LogExporter:
    """ parsing audit log and reporter into sdb collection """

    def __init__(self, args, stat_mgr, log_type, connect):
        self.stat_mgr = stat_mgr
        self.connect = connect
        self.__log_type = log_type
        self.__audit_path = args.get_audit_path()
        self.__audit_log_name = args.get_audit_log_name()
        self.__role = args.get_role()
        self.__node_name = args.get_node_name()
        self.__insert_num = args.get_insert_num()
        self.__cl_full_name = args.get_cl_full_name()
        self.__delete_file = args.get_is_del_file()
        self.__records = []
        self.__num_of_records = 0
        self.__buf = ""

    def __is_valid_suffix(self, suffix):
        if self.__log_type == LOG_TYPE_SDB:
            try:
                time.strptime(suffix, "%Y-%m-%d-%H:%M:%S")
                return True
            except ValueError:
                return False
        else:
            return suffix.isdigit()

    def quit(self, signum, frame):
        work_path = os.getcwd()
        pid_file = os.path.join(work_path, PID_FILE_NAME)
        if os.path.exists(pid_file):
            os.remove(pid_file)
        self.connect.write_row(self.__records)
        self.stat_mgr.update_stat()
        sys.exit()

    def __get_audit_file_list(self, reverse_order):
        audit_list = []
        audit_path = self.__audit_path
        audit_file_name = self.__audit_log_name

        file_list = os.listdir(audit_path)
        if 0 == len(file_list):
            return audit_list

        file_list = sorted(file_list, reverse=reverse_order)

        for f in file_list:
            if f.startswith(audit_file_name):
                if len(f) > len(audit_file_name):
                    suffix = os.path.splitext(f)[-1]
                    suffix_num = suffix[1:]
                    if not self.__is_valid_suffix(suffix_num):
                        continue
                    else:
                        audit_list.append(f)
                else:
                    audit_list.append(f)

        if self.__log_type == LOG_TYPE_SDB:
            # new_audit_log ----> old_audit_log
            if reverse_order:
                try:
                    audit_list.remove(audit_file_name)
                    audit_list.insert(0, audit_file_name)
                except ValueError:
                    pass
            # old_audit_log ----> new_audit_log
            else:
                try:
                    audit_list.remove(audit_file_name)
                    audit_list.append(audit_file_name)
                except ValueError:
                    pass
        return audit_list

    def __get_eldest_audit_file(self):
        audit_path = self.__audit_path
        audit_file_name = self.__audit_log_name
        audit_file = os.path.join(audit_path, audit_file_name)
        if self.__log_type == LOG_TYPE_SDB:
            reverse_order = False  
        else:
            reverse_order = True

        while True:
            audit_file_list = self.__get_audit_file_list(reverse_order)
            if len(audit_file_list) == 0:
                logging.info("No audit file in the audit path " \
                             "{}".format(audit_path))
                time.sleep(PROCESS_WAIT_TIME)
                continue
            audit_inode_before = os.stat(audit_file).st_ino
            for f in audit_file_list:
                file_path = os.path.join(audit_path, f)
                cur_inode = os.stat(file_path).st_ino
                try:
                    fd = open(file_path, 'r')
                    audit_inode_after = os.stat(audit_file).st_ino
                    if audit_inode_after != audit_inode_before:
                        fd.close()
                        break
                    else:
                        return cur_inode, fd
                except IOError:
                    break

    def __get_next_file(self):
        if 0 == self.stat_mgr.get_file_inode():
            return self.__get_eldest_audit_file()
        audit_path = self.__audit_path
        base_file = os.path.join(audit_path, self.__audit_log_name)
        if self.__log_type == LOG_TYPE_SDB:
            reverse_order = True  
        else:
            reverse_order = False

        while True:
            found_file = False
            pre_file_inode = 0
            base_inode_before = os.stat(base_file).st_ino
            audit_file_list = self.__get_audit_file_list(reverse_order)
            if len(audit_file_list) == 0:
                logging.info("No audit file in the audit " \
                             "path {}".format(audit_path))
                time.sleep(PROCESS_WAIT_TIME)
                continue
            for file in audit_file_list:
                file_path = os.path.join(audit_path, file)
                cur_file_stat = os.stat(file_path)
                cur_file_inode = cur_file_stat.st_ino
                if 0 == cur_file_stat.st_size:
                    if 0 == pre_file_inode:
                        return cur_file_inode, None
                    else:
                        logging.error('Audit file {} with inode {} is '
                                      'empty'.format(file, cur_file_inode))
                        return 0, None

                if cur_file_inode == self.stat_mgr.get_file_inode():
                    # Found the file with the expected inode id. Check if
                    # all records in the file have been processed.
                    found_file = True
                    base_inode_after = os.stat(base_file).st_ino
                    try:
                        fd = open(file_path, 'r')
                        if base_inode_after != base_inode_before:
                            # File list changed. Need to check again.
                            fd.close()
                            break
                    except IOError:
                        # If we can not open the file, the target file is
                        #really gone, and incremental synchronization is
                        #not possible any longer.
                        logging.error(
                            'Audit file {} not found'.format(file))
                        return 0, None
                    index = -1
                    for index, line in enumerate(fd):
                         pass
                    fd.seek(0)
                    line_num = index + 1
                    last_parsed_row = self.stat_mgr.get_last_parsed_row()
                    if line_num > last_parsed_row:
                        return cur_file_inode, fd
                    elif line_num == last_parsed_row:
                        # All records in the file have been processed.
                        if 0 == pre_file_inode:
                            # It's the base audit file server_audit.log.
                            fd.close()
                            return cur_file_inode, None
                        else:
                            # All Records in the last file have been
                            # processed, and it's not the base audit file.
                            # So go to the previous one.
                            self.stat_mgr.set_file_inode(pre_file_inode)
                            self.stat_mgr.set_last_parsed_row(0)
                            self.stat_mgr.update_stat()
                            if self.__delete_file and os.path.exists(file_path):
                                os.remove(file_path)
                            break
                    else:
                        logging.error('Line number {} in file {} with '
                                      'inode {} is less than the value {} '
                                      'in the stat file'.format(
                            line_num, file, cur_file_inode, last_parsed_row
                        ))
                        return 0, None
                else:
                    pre_file_inode = cur_file_inode
            if not found_file:
                # If we can not find the file with the target inode, and the
                # audit file list is not changed, the target file is really
                # gone, and incremental synchronization is not possible any
                # longer. If the file list is changed, let's try to find
                # again.
                base_inode_after = os.stat(base_file).st_ino
                if base_inode_after == base_inode_before:
                    logging.error(
                        'Audit file with inode {} not found'.format(
                            self.stat_mgr.get_file_inode()))
                    return 0, None

    def __export_sdb_log(self):
        if 1 == len(self.__buf):
            self.stat_mgr.update_stat()
            return
        try:
            self.__buf = self.__buf.encode('UTF-8')
        except:
            self.stat_mgr.update_stat()
            return  

        try:
            buf = ""
            checked = False
            dict = {}
            for ch in self.__buf:
                if ch != ':':
                    buf += ch
                    continue
                if buf.endswith(FIELD_FROM):
                    value = buf.split(FIELD_FROM)[0].strip()
                    dict[FIELD_USER_NAME] = value
                    buf = ''
                    continue
                elif buf.endswith(FIELD_OBJECT_TYPE):
                    value = int(re.split('[()+]', buf.split()[0])[1])
                    dict[FIELD_RESULT] = int(value)
                    buf = ''
                    continue
                elif buf.endswith(FIELD_RESULT):
                    value = buf.split(FIELD_RESULT)[0].strip()
                    dict[FIELD_ACTION] = value
                    buf = ''
                    continue
                
                value = buf.split()[0].strip()
                if buf.endswith(FIELD_TYPE):
                    checked = True
                    dict[FIELD_TIMESTAMP] = value
                    dt = datetime.datetime.strptime(value,
                                                    "%Y-%m-%d-%H.%M.%S.%f")
                    myTime = int(time.mktime(dt.timetuple())) << 32 | \
                                                       dt.microsecond
                    dict[FIELD_ID] = myTime
                elif buf.endswith(FIELD_PID):
                    checked = True
                    dict[FIELD_TYPE] = value
                elif buf.endswith(FIELD_TID):
                    checked = True
                    dict[FIELD_PID] = int(value)
                elif buf.endswith(FIELD_USER_NAME):
                    checked = True
                    dict[FIELD_TID] = int(value)
                elif buf.endswith(FIELD_ACTION):
                    checked = True
                    dict[FIELD_CLIENT] = value
                elif buf.endswith(FIELD_OBJECT_NAME):
                    checked = True
                    dict[FIELD_OBJECT_TYPE] = value
                elif buf.endswith(FIELD_MESSAGE):
                    if value == self.__cl_full_name:
                        self.stat_mgr.update_stat()
                        return
                    checked = True
                    dict[FIELD_OBJECT_NAME] = value
                if checked:
                    buf = ''
                    checked = False
                    continue
                buf += ch
            dict[FIELD_MESSAGE] = buf.strip()
            dict[FIELD_CONNECT_ID] = 0
            dict[FIELD_OPERATION_ID] = 0
            dict[FIELD_ROLE] = self.__role
            dict[FIELD_HOST_NAME] = self.__node_name.split(":")[0].strip()
            dict[FIELD_SVC_NAME] = int(self.__node_name.split(":")[1].strip())

            self.__records.append(dict)
            self.__num_of_records += 1
            if self.__num_of_records >= self.__insert_num:
                self.connect.write_row(self.__records)
                self.stat_mgr.update_stat()
                self.__records = []
                self.__num_of_records = 0
        except Exception,ValueError:
            logger.error('parse log failed: {}'.format(self.__buf))
            self.stat_mgr.update_stat()

    def __sql_get_operation_type(self, sql):
        operation = sql.strip("[ ']").split()[0].lower()
        if operation.startswith("alter") \
                or operation.startswith("create") \
                or operation.startswith("drop") \
                or operation.startswith("declare") \
                or operation.startswith("flush") \
                or operation.startswith("truncate") \
                or operation.startswith("rename"):
            return "DDL"
        elif operation.startswith("call") \
                or operation.startswith("delete") \
                or operation.startswith("do") \
                or operation.startswith("handler") \
                or operation.startswith("insert") \
                or operation.startswith("load") \
                or operation.startswith("replace") \
                or operation.startswith("select") \
                or operation.startswith("update"):
            return "DML"
        elif sql.startswith("select"):
            return "DQL"
        elif operation.startswith("grant") \
                or operation.startswith("revoke"):
            return "DCL"
        else:
            return ""

    def __export_sql_log(self, line, row_number):
        dict = {}
        if 1 == len(line):
            self.stat_mgr.update_stat()
            return
        try:
            line = line.encode('UTF-8')
        except:
            self.stat_mgr.update_stat()
            return  
        try:
            list = line.split(",")
            my_time = time.mktime(time.strptime(list[0], "%Y%m%d %H:%M:%S"))
            dict[FIELD_ID] = (int(my_time)) << 32 | row_number
            dict[FIELD_ROLE] = self.__role
            dict[FIELD_HOST_NAME] = self.__node_name.split(":")[0].strip()
            dict[FIELD_SVC_NAME] = int(self.__node_name.split(":")[1].strip())
            localtime = time.localtime(my_time)
            dict[FIELD_TIMESTAMP] = time.strftime("%Y-%m-%d-%H.%M.%S.000000",
                                              localtime)
            dict[FIELD_USER_NAME] = list[2]
            dict[FIELD_CLIENT] = socket.gethostbyname(list[3])
            dict[FIELD_PID] = 0
            dict[FIELD_TID] = 0
            dict[FIELD_CONNECT_ID] = int(list[4])
            dict[FIELD_OPERATION_ID] = int(list[5])
            dict[FIELD_MESSAGE] = re.findall(r"'(.*)'", line)[0]
            dict[FIELD_TYPE] = self.__sql_get_operation_type(dict['Message'])
            dict[FIELD_ACTION] = list[6]
            dict[FIELD_OBJECT_TYPE] = "DATABASE"
            dict[FIELD_OBJECT_NAME] = list[7]
            dict[FIELD_RESULT] = int(list[-1].strip())

            self.__records.append(dict)
            self.__num_of_records += 1
            if self.__num_of_records >= self.__insert_num:
                self.connect.write_row(self.__records)
                self.stat_mgr.update_stat()
                self.__records = []
                self.__num_of_records = 0
        except ValueError,Exception:
            logger.error('parse log failed: {}'.format(line))
            self.stat_mgr.update_stat()

    def __export_audit_log_file(self, file_inode, f):
        row_number = 0
        self.stat_mgr.set_file_inode(file_inode)
        lines = f.readlines()
        for origline in lines:
            row_number += 1
            # start from last parse row
            if int(self.stat_mgr.get_last_parsed_row()) >= row_number:
                continue
            self.stat_mgr.set_last_parsed_row(row_number)
            if self.__log_type == LOG_TYPE_SDB:
                self.__buf += origline
                if len(origline) == 1 and origline == "\n":
                    self.__export_sdb_log()
                    self.__buf = ""
            else:
                self.__export_sql_log(origline, row_number)
        
        if len(self.__records):
            self.connect.write_row(self.__records)
            self.__records = []
            self.__num_of_records = 0
            self.stat_mgr.update_stat()

    def run(self):
        fd = None
        try:
            while True:
                # Find and open the next file which should be processed.
                file_inode, fd = self.__get_next_file()
                if 0 == file_inode:
                    logging.error('Get next audit file failed')
                    sys.exit(1)
                if fd is None:
                    # The file is found, but all operations have been processed.
                    # Let's sleep for a while.
                    time.sleep(PROCESS_WAIT_TIME)
                    continue
                self.__export_audit_log_file(file_inode, fd)
                if fd is not None and not fd.closed:
                    fd.close()
        finally:
            if fd is not None and not fd.closed:
                fd.close()

def run_task(args, work_path):
    """ Start exporter worker """

    global logger

    #run logging 
    log_config_file = os.path.join(MY_CONF_PATH, LOG_CONF_FILE_NAME)
    log_instance = Logger()
    rc = log_instance.init(log_config_file)
    if 0 != rc:
        print("[ERROR] Initialize logging failed: {}".format(rc))
        sys.exit(1)
    logger = log_instance.get_logger()
    logger.info("Start sdbaudit reporter tool...")

    #ensure cl exists
    connect = SdbConnect(args)
    connect.ensure_cl()

    try:
        #init status file
        stat_file = os.path.join(work_path, STATUS_FILE_NAME)
        stat_mgr = StatMgr(stat_file)
        try:
            stat_mgr.load_stat()
        except Exception, err:
            logger.error('Load status failed: {}'.format(err))
            raise
            
        #init log reporter
        log_exporter = LogExporter(args, stat_mgr, args.get_log_type(), connect)

        # Signal handler, make it write records and write status file
        # when ctrl + c is pressed.
        signal.signal(signal.SIGINT, log_exporter.quit)
        signal.signal(signal.SIGTERM, log_exporter.quit)

        #run log reporter
        log_exporter.run()
    except Exception as error:
        logger.error('Run task failed:' + str(error))
        raise

def main():
    work_path = os.getcwd()
    pid_file = os.path.join(work_path, PID_FILE_NAME)
    if os.path.exists(pid_file):
        with open(pid_file, "r") as f:
            pid = str(f.readline())
        if os.path.exists("/proc/{pid}".format(pid=pid)):
            with open("/proc/{pid}/cmdline".format(pid=pid), "r") as process:
                process_info = process.readline()
            if process_info.find(sys.argv[0]) != -1:
                print("Only one sdbaudit exporter process is allowed to run " \
                      "at the same time. Exit...")
                return 1
    with open(pid_file, "w") as f:
        pid = str(os.getpid())
        f.write(pid)

    program = os.path.basename(__file__.replace(".py", ""))
    optMgr = OptionsMgr(program)
    rc = optMgr.parse_option()
    if 0 != rc:
        return rc

    run_task(optMgr.args, work_path)



logger = None

if __name__ == '__main__':
    rc = main()
    sys.exit(rc)
