import os
import unittest
import paramiko

import xmlrunner
from lib import s3conf

case_path = os.path.join(os.getcwd(), "")
report_path = "report"
tmp_case_path0 = os.path.split(case_path)[0]
tmp_case_path1 = os.path.split(case_path)[1]
# sequoias3暂时支持带support后缀文件下的用例，后面如果有新增新的用例，请添加
pattern_param = "test_*_support.py"
global cfg
global sdb_install_path
global s

if tmp_case_path1.endswith('.py'):
    pattern_param = tmp_case_path1
    case_path = tmp_case_path0


def all_case():
    discover = unittest.defaultTestLoader.discover(case_path, pattern=pattern_param, top_level_dir=None)
    return discover


def get_ssh(hostname=None, port=22, username='root', password=None):
    s = paramiko.SSHClient()
    s.set_missing_host_key_policy(paramiko.AutoAddPolicy)
    s.connect(hostname=hostname, port=port, username=username, password=password)
    return s


def ssh_exce_command(command=None):
    stdin, stdout, stderr = s.exec_command(command=command)
    return stdout.readlines()


def start_s3():
    stop_s3()
    start_command = "source /etc/profile;" + sdb_install_path + "/tools/sequoias3/sequoias3.sh start >> " \
                                                                "/tmp/s3start.log"
    status_command = sdb_install_path + "/tools/sequoias3/sequoias3.sh status"
    for i in range(3):
        ssh_exce_command(command=start_command)
        status_result = ssh_exce_command(command=status_command)
        if not str(status_result).__contains__("Total: 0"):
            break


def stop_s3():
    status_command = sdb_install_path + "/tools/sequoias3/sequoias3.sh status"
    status_result = ssh_exce_command(command=status_command)
    if not str(status_result).__contains__("Total: 0"):
        stop_command = sdb_install_path + "/tools/sequoias3/sequoias3.sh stop -a"
        for i in range(3):
            ssh_exce_command(command=stop_command)
            status_result = ssh_exce_command(command=status_command)
            if str(status_result).__contains__("Total: 0"):
                break


if __name__ == "__main__":
    try:
        cfg = s3conf.S3Config()
        s = get_ssh(hostname=cfg.remote_host, username=cfg.remote_user, password=cfg.remote_password)
        command = "cat /etc/default/sequoiadb | grep 'INSTALL_DIR' | awk -F '=' '{printf(\"%s\",$2)}'"
        result = ssh_exce_command(command=command)
        sdb_install_path = result[0]
        start_s3()
        runner = xmlrunner.XMLTestRunner(output=report_path)
        test_result = runner.run(all_case())
    finally:
        stop_s3()
        s.close()
