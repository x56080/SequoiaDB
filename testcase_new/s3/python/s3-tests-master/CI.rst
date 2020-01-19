
1、保证ci 环境安装了python3.5

2、需要安装python虚拟机，在 Debian/Ubuntu命令::
sudo apt-get install python-virtualenv

3、在s3 python测试目录下，执行命令::
  ./bootstrap

4、配置s3配置文件s3tests.json，需要配置以下配置项：
  #s3主机名
  "host":, 
  #s3端口号：
  "port":,
  #s3的access_key
  "access_key": "",
  #s3的secret_key
  "secret_key": "",

5、在s3 python测试目录下，执行s3 python 用例,  命令::
 ./virtualenv/bin/python run_all_case.py

6、测试报告默认在当前report目录下
