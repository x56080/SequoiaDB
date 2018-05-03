#!/usr/bin/python
import getopt
import os
import sys
from datetime import datetime

LICENSE_COMMENT_START = "/*******************************************************************************"
LICENSE_COMMENT_END = "*******************************************************************************/"
LICENSE = '''   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
'''
COPYRIGHT = "   Copyright (C) 2011-2018 SequoiaDB Ltd.\n"
BLANK_LINE = "\n"
LICENSE_AGPL = "GNU Affero General Public License"
LICENSE_APACHE = "Apache License"
LICENSE_UNKNOWN = "Unknown License"
CXX_FILE_SUFFIX = ['.c', '.cpp', ".h", ".hpp"]

# log_file = None


def init_log(log_path):
    # global log_file
    # log_file = open(log_path, 'w')
    pass


def term_log():
    # global log_file
    # if log_file is not None:
    #    log_file.close()
    pass


def log(msg):
    # global log_file
    # if log_file is not None:
    time = datetime.now().strftime("[%Y-%m-%d %H:%M:%S.%f]")
    s = "%s%s" % (time, msg)
    print(s)


def get_file_suffix(file_path):
    base = os.path.basename(file_path)
    s = os.path.splitext(base)
    if len(s) > 1:
        return s[1]
    else:
        return None


def is_comment_start(line):
    if line.startswith("/*"):
        return True
    else:
        return False


def is_comment_end(line):
    if line.endswith("*/"):
        return True
    else:
        return False


def is_copyright(line):
    if line.find("Copyright (C)") == -1:
        return False
    else:
        return True


def is_desc_start(line):
    if line.find("Source File Name") == -1:
        return False
    else:
        return True


def is_agpl(line):
    if line.find(LICENSE_AGPL) == -1:
        return False
    else:
        return True


def is_apache_license(line):
    if line.find(LICENSE_APACHE) == -1:
        return False
    else:
        return True


def is_desc_end(line):
    if line.find("Last Changed") == -1:
        return False
    else:
        return True


def find_cxx_license(file_path):
    f = open(file_path, 'r')
    in_comment = False
    is_first_line = True
    in_license_comment = False
    license = None

    for raw_line in f:
        line = raw_line.strip()
        if is_first_line:
            if is_comment_start(line):
                in_comment = True
            is_first_line = False
        if in_comment:
            if is_copyright(line):
                in_license_comment = True
                license = LICENSE_UNKNOWN
            if in_license_comment:
                if is_agpl(line):
                    license = LICENSE_AGPL
                    break
                if is_apache_license(line):
                    license = LICENSE_APACHE
                    break
            if is_comment_end(line):
                in_comment = False
                if in_license_comment:
                    in_license_comment = False
                    break
    f.close()
    return license


def replace_cxx_license(file_path):
    new_file = file_path + ".license"
    f = open(file_path, 'r')
    new_f = open(new_file, 'w')
    new_f.truncate(0)
    in_comment = False
    is_first_line = True
    in_license_comment = False
    under_desc = False

    for raw_line in f:
        line = raw_line.strip()
        if is_first_line:
            if is_comment_start(line):
                in_comment = True
            is_first_line = False
        if in_comment:
            if is_copyright(line):
                in_license_comment = True
                new_f.write(COPYRIGHT)
                new_f.write(BLANK_LINE)
                new_f.write(LICENSE)
            if in_license_comment:
                if is_desc_start(line):
                    under_desc = True
                    new_f.write(BLANK_LINE)
                if under_desc and not is_comment_end(line):
                    new_f.write(raw_line)
            else:
                new_f.write(raw_line)
            if is_comment_end(line):
                in_comment = False
                if in_license_comment:
                    in_license_comment = False
                new_f.write(raw_line)
        else:
            new_f.write(raw_line)

    new_f.flush()
    new_f.close()
    f.close()
    os.unlink(file_path)
    os.rename(new_file, file_path)


def process_file(file_path):
    suffix = get_file_suffix(file_path)
    if suffix is None:
        log("%s: ignore" % file_path)
        return
    if suffix.lower() in CXX_FILE_SUFFIX:
        license = find_cxx_license(file_path)
        if license in [LICENSE_AGPL, LICENSE_APACHE]:
            replace_cxx_license(file_path)
            log("%s: %s" % (file_path, license))
        else:
            log("%s: %s, ignore" % (file_path, license))
    else:
        log("%s: not CXX file, ignore" % file_path)


def traverse_files(path):
    if os.path.isdir(path):
        file_names = os.listdir(path)
        for file_name in file_names:
            if file_name == ".svn":
                # ignore .svn directory
                continue
            file_path = os.path.join('%s%s%s' % (path, os.path.sep, file_name))
            if os.path.isfile(file_path):
                process_file(file_path)
            elif os.path.isdir(file_path):
                traverse_files(file_path)
    elif os.path.isfile(path):
        process_file(path)
    else:
        raise Exception("Invalid file or directory: " + path)


def print_help(name):
    print('usage: %s [option]...' % name)
    print("")
    print("Options:")
    print("\t-h, --help     print help information")
    print("\t-p, --path     specify source code path, default is './'")
    #print("\t-l, --log      specify log path, default is 'sdblicense.log'")


def main(argv):
    path = "./"
    log_path = "sdblicense.log"
    try:
        opts, args = getopt.getopt(argv[1:], "hp:l:", ["help", "path=", "log="])
    except getopt.GetoptError:
        print_help(argv[0])
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print_help(argv[0])
            sys.exit(0)
        elif opt in ("-p", "--path"):
            path = arg
        # elif opt in ("-l", "--log"):
        #    log_path = arg

    init_log(log_path)
    try:
        traverse_files(path)
    finally:
        term_log()


if __name__ == '__main__':
    main(sys.argv)
