确保环境中已安装python3且有ssl模块
setuptools-46.1.3.zip、pip-20.0.2.tar.gz、virtualenv-15.0.1.tar.gz安装包在./tools目录下

---------------------
1、离线安装setuptools
unzip  setuptools-46.1.3.zip

cd setuptools-46.1.3/

python3 setup.py install
-----------------------

2、离线安装pip

tar -zxf pip-20.0.2.tar.gz

cd pip-20.0.2/

python3 setup.py install

----------------------

3、离线安装virtualenv

tar -zxf virtualenv-15.0.1.tar.gz

cd virtualenv-15.0.1/

python3 setup.py install

---------------------

4、创建虚拟环境，将虚拟环境安装在特定目录下，以后ci执行s3 python用例直接用虚拟环境即可，不需要重复创建虚拟环境
virtualenv -p python环境变量  --no-site-packages --distribute --no-setuptools --no-pip --no-wheel 虚拟机名

如下：
virtualenv -p /usr/local/bin/python3  --no-site-packages --distribute --no-setuptools --no-pip --no-wheel s3virtualenv

--------------------

5、给s3virtualenv安装setuptools包

  cd setuptools-46.1.3/
  
  ../../s3virtualenv/bin/python setup.py  install
  
--------------------

6、给virtualenv安装pip包  

  cd pip-20.0.2/
  
  ../../s3virtualenv/bin/python setup.py install

-------------------

7、给virtualenv离线安装相关s3 python驱动包,每次ci执行用例时需要执行以下命令

s3 python驱动包在./tools/s3-requirements/requirements-py3.5目录下

./s3virtualenv/bin/pip install --no-index  --find-links=./tools/s3-requirements/requirements-py3.5 -r requirements.txt

------------------

8、执行测试用例

配置s3配置文件s3tests.json，需要配置以下配置项：
 #s3主机名
  "host":, 
  #s3端口号：
  "port":
  #远程主机地址（s3主机）
 "remote_host": "localhost",
 #远程主机用户名
 "remote_user": "root",
  #远程主机用户密码
 "remote_password": "sequoiadb"

./s3virtualenv/bin/python run_all_case.py

测试报告默认在./report目录下




















