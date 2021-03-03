##环境准备##

请参考[WebSphere Application Server detailed system requirements](http://www-01.ibm.com/support/docview.wss?rs=180&uid=swg27006921)，检查是否满足WebSphere安装的软硬件要求。

##安装配置##

从IBM官网下载 WebSphere 的试用版，将下载的安装包拷贝到安装服务器上。

   >**Note:**  
   >1）以WebSphere试用版 **was.cd.70011.trial.base.opt.linux.ia32.tar.gz** 为例；  
   >2）WebSphere安装包所在路径以 /opt/web/packet 为例；  
   >3）WebSphere安装目录以 /opt/IBM/WebSphere/AppServer 为例；

1. 解压并安装

   ```lang-bash
suse113-1:~ # cd /opt/web/packet/
suse113-1:/opt/web/packet # tar -xzvf was.cd.70011.trial.base.opt.linux.ia32.tar.gz
suse113-1:/opt/web/packet # cd WAS/
suse113-1:/opt/web/packet/WAS # ./install
```
2. 弹出安装界面，点击 **Next**

   ![](webserverapp/websphere/install_1.jpg)

3. 选择 **I accept both the IBM and the non-IBM terms**，点击 **Next**

   ![](webserverapp/websphere/install_2.jpg)

4. 点击 **Next**

   ![](webserverapp/websphere/install_3.jpg)

5. 勾选所有选项，点击 **Next**

   ![](webserverapp/websphere/install_4.jpg)

6. 选择安装路径，点击 **Next**

   ![](webserverapp/websphere/install_5.jpg)

7. 点击 **Next**

   ![](webserverapp/websphere/install_6.jpg)

8. 填写用户名和密码，点击 **Next**

   ![](webserverapp/websphere/install_7.jpg)

9. 点击 **Next**

   ![](webserverapp/websphere/install_8.jpg)

10. 点击 **Next**

   ![](webserverapp/websphere/install_9.jpg)

11. 点击 **Next**，开始安装

   ![](webserverapp/websphere/install_10.jpg)

12. 点击 **Finish**

   ![](webserverapp/websphere/install_11.jpg)

13. 点击 **Installation verfiication**

  ![](webserverapp/websphere/install_12.jpg)

14. 服务自启动，并且提示安装校验完成，表示完成安装（完成后窗口可关闭）

  ![](webserverapp/websphere/install_13.jpg)
