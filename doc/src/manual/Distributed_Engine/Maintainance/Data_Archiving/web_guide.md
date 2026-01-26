
每个调度服务都提供了可视化页面管理的功能，通过 web 界面方式完成调度任务管理等操作。

## 运行环境

### 端口

网页的访问端口由部署调度服务时指定，部署完成后访问 http://ip:port 即可进入到 WEB 界面

### 浏览器

可以使用以下浏览器：

- Chrome，支持 Chrome 23.0 以上版本，建议使用当前较新的版本

- Firefox，支持 Firefox 22.0 以上版本，建议使用当前较新的版本

- IE，支持 IE10.0 以上版本，建议使用当前较新的版本

- Microsoft Edge，建议使用较新版本

- 其他主流浏览器，建议使用较新的版本；国内浏览器通常是多内核，在不能正常使用可以尝试切换内核

### 分辨率和缩放

- 页面需要分辨率不小于 1024 x 768，为了更好使用体验，建议分辨率在 1366 x 768 或更高。

- 页面支持缩放，支持 80%、90%、100%、125% 的缩放，默认是 100%。

### 浏览器设置要求

- IE 浏览器设置了较高安全级别，需要把网站添加到可信站点。

- 部分浏览器需要关闭特殊模式，如：IE 要关闭兼容模式（兼容模式默认关闭）。

- 浏览器必须允许网站运行 Javascript （默认允许）。

## 主要操作

### 节点管理

#### 查看节点列表

点击左侧导航栏【节点管理】，将展示集群所有调度服务节点的主机名、IP、端口、状态等信息。用户可在顶部搜索栏按主机名过滤信息。

![节点列表][node-list]

#### 查看节点详情

用户在操作栏点击 **查看** 按钮后，对应调度服务节点的详细信息将以对话框形式展示。

![节点详情][node-detail]

### 站点管理

#### 查看站点列表

点击左侧导航栏【站点管理】，将展示已创建站点的站点名称、数据源用户和数据源地址等信息。用户可在顶部搜索栏按站点名过滤信息。

![站点列表][site-list]

#### 查看站点详情

用户在操作栏点击 **查看** 按钮后，对应站点的详细信息将以对话框形式展示。

![站点详情][site-detail]

### 调度任务管理

#### 查看调度任务列表

点击左侧导航栏【调度任务管理】，将展示已创建调度任务的任务名称、任务描述、任务类型和启用状态等信息。用户可在顶部搜索栏按调度任务名过滤信息。

![调度任务列表][schedule-list]

#### 创建调度任务

在调度任务列表界面，点击 创建调度任务 按钮，弹出创建调度任务对话框，支持创建迁移、数据切换和清理调度任务。

![创建调度任务][create-schedule]

#### 查看调度任务

在调度任务任务列表界面，点击指定调度任务的 查看 按钮，以对话框形式展示该调度任务的详细配置信息

![调度任务详情][schedule-detail]

#### 修改调度任务

在调度任务列表界面，点击 编辑 按钮，弹出调度任务编辑对话框，支持修改创建调度任务时指定的任务配置参数

![修改调度任务][update-schedule]

#### 查看调度任务运行记录

用户在操作栏点击 **运行记录** 按钮，列表展示该调度任务的运行记录，显示该调度任务每次触发的后台任务执行的开始时间、结束时间、执行节点、状态等信息。用户可在顶部搜索栏按调度任务ID过滤信息

![调度任务运行记录][task-list]

#### 查看任务运行记录详情

在任务运行记录界面，点击指定任务记录的 **查看详情** 按钮，以对话框形式展示任务运行的详细信息

![任务运行记录详情][task-detail]

#### 查看任务运行详情

在任务运行记录界面，点击指定任务记录的 **运行详情** 按钮，以对话框形式展示该任务运行的详细信息，迁移任务则展示迁移的记录数等信息，清理任务则展示清理的集合等信息。

![任务运行详情][task-progress]

#### 启用/禁用、删除调度任务

在调度任务列表界面，可通过点击指定调度任务的**开关按钮**或 **删除** 按钮，完成对指定调度任务的启用/禁用或删除操作。

### 系统配置管理

#### 查看系统配置

点击左侧导航栏【系统配置管理】，将展示系统上所有支持在线修改的配置项及值和描述。用户可在顶部搜索栏按配置项过滤信息。

![系统配置列表][conf-list]

#### 修改系统配置

在系统配置列表界面，点击 **修改** 按钮，弹出修改配置项对话框，支持修改配置项的值

![修改系统配置][update-conf]

[^_^]:
    本文使用到的所有链接及引用。
[node-list]:images/Distributed_Engine/Maintainance/Data_Archiving/node-list.png
[node-detail]:images/Distributed_Engine/Maintainance/Data_Archiving/node-detail.png
[site-list]:images/Distributed_Engine/Maintainance/Data_Archiving/site-list.png
[site-detail]:images/Distributed_Engine/Maintainance/Data_Archiving/site-detail.png
[schedule-list]:images/Distributed_Engine/Maintainance/Data_Archiving/schedule-list.png
[create-schedule]:images/Distributed_Engine/Maintainance/Data_Archiving/create-schedule.png
[schedule-detail]:images/Distributed_Engine/Maintainance/Data_Archiving/schedule-detail.png
[update-schedule]:images/Distributed_Engine/Maintainance/Data_Archiving/update-schedule.png
[task-list]:images/Distributed_Engine/Maintainance/Data_Archiving/task-list.png
[task-detail]:images/Distributed_Engine/Maintainance/Data_Archiving/task-detail.png
[task-progress]:images/Distributed_Engine/Maintainance/Data_Archiving/task-progress.png
[conf-list]:images/Distributed_Engine/Maintainance/Data_Archiving/conf-list.png
[update-conf]:images/Distributed_Engine/Maintainance/Data_Archiving/update-conf.png