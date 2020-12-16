
用户在 SequoiaDB 存储集群修改配置页面可以查看节点配置，并在线修改节点配置。

集群模式
----
1. 通过左侧导航【配置】选择 SequoiaDB 服务，点击进入配置页面，当前页面可以查看该服务的节点列表
![进入配置页面][sdb_config_1]

2. 修改节点配置有单节点修改和批量修改两种方式：

   - 批量修改 

      在表格中勾选需要修改配置的节点，选择后点击 **批量修改配置** 按钮，进入配置修改页面
       ![批量修改配置][sdb_config_2]

   - 单节点修改

      在表格中选择需要修改配置的节点，直接点击选中的列表，进入配置修改页面
       ![单节点修改配置][sdb_config_3]

3. 进入【批量节点配置】页面中可以进行修改配置、重启节点和查看详细配置操作
![配置][sdb_config_4]

   - 查看节点配置

     在该页面默认可以看到常用的配置项，点击 **查看详细配置** 后点击 **选择显示列** 按钮可以选择想要显示的配置项

   - 修改配置

       1. 点击 **修改配置** 按钮，在弹窗选择需要修改的配置项进行修改，配置项留空则表示默认值
   ![修改配置弹窗][sdb_config_5]

       2. 填写参数后点击 **确定** 按钮，如果修改的配置项中有需要重启生效的项，列表中会将需要重启生效的配置项标红显示，可以点击上方的 **重启节点** 按钮进行重启服务
   ![修改配置][sdb_config_6]

4. 修改完配置后回到【配置】页面，可以在节点列表中看到哪些节点的配置项发生了变化，有变化的节点前面会显示【变化】标识。
![修改完成][sdb_config_7]

单机模式
----
通过左侧导航【配置】选择 SequoiaDB 服务，点击进入配置页面，可以查看节点的常用配置项，以及进行修改配置 、重启节点、查看详细配置操作。
![进入配置页面][sdb_config_8]

- 查看节点详细配置

   在该页面默认可以看到常用的配置项，点击 **查看详细配置** 之后可以查看所有配置项

- 修改配置
  
   1. 点击 **修改配置** 按钮打开修改配置弹窗，选择需要修改的配置项进行修改
    ![修改配置弹窗][sdb_config_9]

   2. 填写好修改的配置项之后点击 **确定** 按钮，如果修改的配置项中有需要重启生效的项，列表中会将需要重启生效的配置项标红显示，可以点击上方的 **重启节点** 按钮进行重启服务
    ![修改完成][sdb_config_10]




[^_^]:
    本文使用的所有链接及引用
[sdb_config_1]:images/SAC/Operation/Sequoiadb_Data/sdb_config_1.png
[sdb_config_2]:images/SAC/Operation/Sequoiadb_Data/sdb_config_2.png
[sdb_config_3]:images/SAC/Operation/Sequoiadb_Data/sdb_config_3.png
[sdb_config_4]:images/SAC/Operation/Sequoiadb_Data/sdb_config_4.png
[sdb_config_5]:images/SAC/Operation/Sequoiadb_Data/sdb_config_5.png
[sdb_config_6]:images/SAC/Operation/Sequoiadb_Data/sdb_config_6.png
[sdb_config_7]:images/SAC/Operation/Sequoiadb_Data/sdb_config_7.png
[sdb_config_8]:images/SAC/Operation/Sequoiadb_Data/sdb_config_8.png
[sdb_config_9]:images/SAC/Operation/Sequoiadb_Data/sdb_config_9.png
[sdb_config_10]:images/SAC/Operation/Sequoiadb_Data/sdb_config_10.png
