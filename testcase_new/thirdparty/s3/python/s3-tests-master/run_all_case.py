import os
import os.path
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
global sftp
global propertiesFileName
global replaceFileName

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


def get_sftp(hostname=None, port=22, username='root', password=None):
    client = paramiko.Transport(hostname, port)
    client.connect(username=username, password=password)
    return client


def scp(local_file=None, remote_dir="/tmp"):
    sftp_client = paramiko.SFTPClient.from_transport(sftp)
    sftp_client.put(local_file, remote_dir)


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
            return
    raise Exception("it has been try 3 times to satrt s3 node, but still start s3 failed, please check "
                    "/tmp/s3start.log and sequoias3.log")


def stop_s3():
    status_command = sdb_install_path + "/tools/sequoias3/sequoias3.sh status"
    status_result = ssh_exce_command(command=status_command)
    if not str(status_result).__contains__("Total: 0"):
        stop_command = sdb_install_path + "/tools/sequoias3/sequoias3.sh stop -a"
        for i in range(3):
            ssh_exce_command(command=stop_command)
            status_result = ssh_exce_command(command=status_command)
            if str(status_result).__contains__("Total: 0"):
                return
        raise Exception("it has been try 3 times to stop s3 node, but still stop s3 failed, please check sequoias3.log")


def change_s3_conf():
    command = "cp -f " + propertiesFileName + " " + replaceFileName + ";"
    command += "echo -e 'server.port=" + str(cfg.port) + "\nsdbs3.sequoiadb.url=sequoiadb://" + cfg.sdb_coord_urls \
               + "\nsdbs3.multipartupload.completereservetime=1' "
    command += " >>" + propertiesFileName
    ssh_exce_command(command=command)


def restore_s3_conf():
    command = "mv " + replaceFileName + " " + propertiesFileName
    ssh_exce_command(command=command)


def update_sdb_conf():
    config = "'{ transisolation:1,translockwait:true}'"
    path = os.path.realpath("script") + "/updateSdbConf.js"
    scp(local_file=path, remote_dir="/tmp/updateSdbConf.js")
    command = sdb_install_path + "/bin/sdb -f /tmp/updateSdbConf.js" + " -e \"var hostname=\'" + str(cfg.sdb_coord_urls).split(":")[0] + \
              "\';var port=" + str(cfg.sdb_coord_urls).split(":")[1] + ";var config=" + config + "\""

    print(command)
    result = ssh_exce_command(command=command)
    print("result  : " + str(result))


def delete_sdb_conf():
    config = "'{ transisolation:1,translockwait:true}'"
    path = os.path.realpath("script") + "/deleteSdbConf.js"
    scp(local_file=path, remote_dir="/tmp/deleteSdbConf.js")
    command = sdb_install_path + "/bin/sdb -f /tmp/deleteSdbConf.js" + " -e \"var hostname=\'" + str(cfg.sdb_coord_urls).split(":")[0] + \
              "\';var port=" + str(cfg.sdb_coord_urls).split(":")[1] + ";var config=" + config + "\""
    print(command)
    result = ssh_exce_command(command=command)
    print("result  : " + str(result))


if __name__ == "__main__":
    cfg = s3conf.S3Config()
    s = get_ssh(hostname=cfg.remote_host, username=cfg.remote_user, password=cfg.remote_password)
    sftp = get_sftp(hostname=cfg.remote_host, username=cfg.remote_user, password=cfg.remote_password)
    command = "cat /etc/default/sequoiadb | grep 'INSTALL_DIR' | awk -F '=' '{printf(\"%s\",$2)}'"
    result = ssh_exce_command(command=command)
    sdb_install_path = result[0]
    propertiesFileName = sdb_install_path + "/tools/sequoias3/config/application.properties"
    replaceFileName = sdb_install_path + "/tools/sequoias3/config/ori_application.properties"
    try:
        update_sdb_conf()
        change_s3_conf()
        start_s3()
        runner = xmlrunner.XMLTestRunner(output=report_path)
        test_result = runner.run(all_case())
    finally:
        delete_sdb_conf()
        stop_s3()
        restore_s3_conf()
        s.close()
        sftp.close()
