1. 在部署首页可以看到主机状态都是正常的。

   ![更新主机信息](sac/deployment/update_host_1.jpg)

2. 演示的主机 ubuntu-test-002 的IP改变了，导致OM服务连接ubuntu-test-002主机失败。

   ![更新主机信息](sac/deployment/update_host_2.jpg)

3. ubuntu-test-002 的IP从192.168.1.102变成192.168.1.103。

   ![更新主机信息](sac/deployment/update_host_3.jpg)

4. 在SAC页面选中要修改的主机，演示的就是 ubuntu-test-002 主机。

   ![更新主机信息](sac/deployment/update_host_4.jpg)

5. 点击 **更新主机信息**，在窗口中修改 ubuntu-test-002 的IP地址为192.168.1.103，点击 **确定**。

   ![更新主机信息](sac/deployment/update_host_5.jpg)

6. 更新完成，主机状态恢复正常。手工关闭窗口，点击 **取消** 按钮或者窗口 **X** 按钮。

   ![更新主机信息](sac/deployment/update_host_6.jpg)

> **Note:**  
> 更新成功后，如果主机状态仍然无法连接，可能是dns缓存没有更新导致。

####更新dns缓存####

登录OM服务的主机，演示的OM服务主机是 ubuntu-test 。

```lang-javascript
$ service nscd restart
```
