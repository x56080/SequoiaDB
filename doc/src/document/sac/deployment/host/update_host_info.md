1. 在 **部署 - 主机** 可以看到主机状态都是正常的。

   ![更新主机信息](sac/deployment/host/update_host_1.png)

2. 演示的主机 sdbserver3 的 IP 改变了，导致访问 sdbserver3 主机失败。

   > **Note；**  
   > 鼠标停留在红色标记的状态上，会显示错误信息。

   ![更新主机信息](sac/deployment/host/update_host_2.png)

3. sdbserver3 的 IP 从192.168.1.103 变成 192.168.1.113。

   ![更新主机信息](sac/deployment/host/update_host_3.png)

4. 选择需要更新 IP 的主机，点击 **主机操作 - 更新主机信息**。

   ![更新主机信息](sac/deployment/host/update_host_4.png)

5. 在窗口修改 sdbserver3 的 IP 地址为 192.168.3.113，点击 **确定**。

   ![更新主机信息](sac/deployment/host/update_host_5.png)

6. 更新完成，关闭窗口。

   ![更新主机信息](sac/deployment/host/update_host_6.png)

7. sdbserver3 主机恢复正常。

   ![更新主机信息](sac/deployment/host/update_host_7.png)

> **Note:**  
> sdbserver3 修改 IP 仅用作当前演示，在其他文档中 sdbserver3 的 IP 仍然是 192.168.1.103。  
> 更新成功后，如果主机状态仍然无法连接，可能是dns缓存没有更新导致。  
> 如果主机用户 sdbadmin 的密码修改了，会更新失败。

####更新 DNS 缓存####

登录 SAC 页面的主机，演示的主机是 sdbserver1。

```lang-javascript
$ service nscd restart
```
