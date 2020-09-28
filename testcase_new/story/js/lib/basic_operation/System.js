var tmpSystem = {
   addAHostMap: System.prototype.addAHostMap,
   addGroup: System.prototype.addGroup,
   addUser: System.prototype.addUser,
   buildTrusty: System.prototype.buildTrusty,
   delAHostMap: System.prototype.delAHostMap,
   delGroup: System.prototype.delGroup,
   delUser: System.prototype.delUser,
   getAHostMap: System.prototype.getAHostMap,
   getCpuInfo: System.prototype.getCpuInfo,
   getCurrentUser: System.prototype.getCurrentUser,
   getDiskInfo: System.prototype.getDiskInfo,
   getEWD: System.prototype.getEWD,
   getHostName: System.prototype.getHostName,
   getHostsMap: System.prototype.getHostsMap,
   getInfo: System.prototype.getInfo,
   getIpTablesInfo: System.prototype.getIpTablesInfo,
   getMemInfo: System.prototype.getMemInfo,
   getNetcardInfo: System.prototype.getNetcardInfo,
   getPID: System.prototype.getPID,
   getProcUlimitConfigs: System.prototype.getProcUlimitConfigs,
   getReleaseInfo: System.prototype.getReleaseInfo,
   getSystemConfigs: System.prototype.getSystemConfigs,
   getTID: System.prototype.getTID,
   getUserEnv: System.prototype.getUserEnv,
   help: System.prototype.help,
   isGroupExist: System.prototype.isGroupExist,
   isProcExist: System.prototype.isProcExist,
   isUserExist: System.prototype.isUserExist,
   killProcess: System.prototype.killProcess,
   listAllUsers: System.prototype.listAllUsers,
   listGroups: System.prototype.listGroups,
   listLoginUsers: System.prototype.listLoginUsers,
   listProcess: System.prototype.listProcess,
   ping: System.prototype.ping,
   removeTrusty: System.prototype.removeTrusty,
   runService: System.prototype.runService,
   setProcUlimitConfigs: System.prototype.setProcUlimitConfigs,
   setUserConfigs: System.prototype.setUserConfigs,
   snapshotCpuInfo: System.prototype.snapshotCpuInfo,
   snapshotDiskInfo: System.prototype.snapshotDiskInfo,
   snapshotMemInfo: System.prototype.snapshotMemInfo,
   snapshotNetcardInfo: System.prototype.snapshotNetcardInfo,
   sniffPort: System.prototype.sniffPort,
   type: System.prototype.type,
   _getInfo: System.prototype._getInfo
};
var funcSystem = System;
var funcaddAHostMap = System.addAHostMap;
var funcaddGroup = System.addGroup;
var funcaddUser = System.addUser;
var funcdelAHostMap = System.delAHostMap;
var funcdelGroup = System.delGroup;
var funcdelUser = System.delUser;
var funcgetAHostMap = System.getAHostMap;
var funcgetCpuInfo = System.getCpuInfo;
var funcgetCurrentUser = System.getCurrentUser;
var funcgetDiskInfo = System.getDiskInfo;
var funcgetEWD = System.getEWD;
var funcgetHostName = System.getHostName;
var funcgetHostsMap = System.getHostsMap;
var funcgetIpTablesInfo = System.getIpTablesInfo;
var funcgetMemInfo = System.getMemInfo;
var funcgetNetcardInfo = System.getNetcardInfo;
var funcgetObj = System.getObj;
var funcgetPID = System.getPID;
var funcgetProcUlimitConfigs = System.getProcUlimitConfigs;
var funcgetReleaseInfo = System.getReleaseInfo;
var funcgetSystemConfigs = System.getSystemConfigs;
var funcgetTID = System.getTID;
var funcgetUserEnv = System.getUserEnv;
var funchelp = System.help;
var funcisGroupExist = System.isGroupExist;
var funcisProcExist = System.isProcExist;
var funcisUserExist = System.isUserExist;
var funckillProcess = System.killProcess;
var funclistAllUsers = System.listAllUsers;
var funclistGroups = System.listGroups;
var funclistLoginUsers = System.listLoginUsers;
var funclistProcess = System.listProcess;
var funcping = System.ping;
var funcrunService = System.runService;
var funcsetProcUlimitConfigs = System.setProcUlimitConfigs;
var funcsetUserConfigs = System.setUserConfigs;
var funcsnapshotCpuInfo = System.snapshotCpuInfo;
var funcsnapshotDiskInfo = System.snapshotDiskInfo;
var funcsnapshotMemInfo = System.snapshotMemInfo;
var funcsnapshotNetcardInfo = System.snapshotNetcardInfo;
var funcsniffPort = System.sniffPort;
var functype = System.type;
var func_listProcess = System._listProcess;
var func_listLoginUsers = System._listLoginUsers;
var func_listAllUsers = System._listAllUsers;
var func_listGroups = System._listGroups;
var func_createSshKey = System._createSshKey;
var func_getHomePath = System._getHomePath;
System=function(){try{return funcSystem.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.addAHostMap = function(){try{ return funcaddAHostMap.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.addGroup = function(){try{ return funcaddGroup.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.addUser = function(){try{ return funcaddUser.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.delAHostMap = function(){try{ return funcdelAHostMap.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.delGroup = function(){try{ return funcdelGroup.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.delUser = function(){try{ return funcdelUser.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getAHostMap = function(){try{ return funcgetAHostMap.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getCpuInfo = function(){try{ return funcgetCpuInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getCurrentUser = function(){try{ return funcgetCurrentUser.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getDiskInfo = function(){try{ return funcgetDiskInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getEWD = function(){try{ return funcgetEWD.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getHostName = function(){try{ return funcgetHostName.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getHostsMap = function(){try{ return funcgetHostsMap.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getIpTablesInfo = function(){try{ return funcgetIpTablesInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getMemInfo = function(){try{ return funcgetMemInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getNetcardInfo = function(){try{ return funcgetNetcardInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getObj = function(){try{ return funcgetObj.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getPID = function(){try{ return funcgetPID.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getProcUlimitConfigs = function(){try{ return funcgetProcUlimitConfigs.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getReleaseInfo = function(){try{ return funcgetReleaseInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getSystemConfigs = function(){try{ return funcgetSystemConfigs.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getTID = function(){try{ return funcgetTID.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.getUserEnv = function(){try{ return funcgetUserEnv.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.isGroupExist = function(){try{ return funcisGroupExist.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.isProcExist = function(){try{ return funcisProcExist.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.isUserExist = function(){try{ return funcisUserExist.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.killProcess = function(){try{ return funckillProcess.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.listAllUsers = function(){try{ return funclistAllUsers.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.listGroups = function(){try{ return funclistGroups.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.listLoginUsers = function(){try{ return funclistLoginUsers.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.listProcess = function(){try{ return funclistProcess.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.ping = function(){try{ return funcping.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.runService = function(){try{ return funcrunService.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.setProcUlimitConfigs = function(){try{ return funcsetProcUlimitConfigs.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.setUserConfigs = function(){try{ return funcsetUserConfigs.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.snapshotCpuInfo = function(){try{ return funcsnapshotCpuInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.snapshotDiskInfo = function(){try{ return funcsnapshotDiskInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.snapshotMemInfo = function(){try{ return funcsnapshotMemInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.snapshotNetcardInfo = function(){try{ return funcsnapshotNetcardInfo.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.sniffPort = function(){try{ return funcsniffPort.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.type = function(){try{ return functype.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System._listProcess = function(){try{ return func_listProcess.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System._listLoginUsers = function(){try{ return func_listLoginUsers.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System._listAllUsers = function(){try{ return func_listAllUsers.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System._listGroups = function(){try{ return func_listGroups.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System._createSshKey = function(){try{ return func_createSshKey.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System._getHomePath = function(){try{ return func_getHomePath.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
System.prototype.addAHostMap=function(){try{return tmpSystem.addAHostMap.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.addGroup=function(){try{return tmpSystem.addGroup.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.addUser=function(){try{return tmpSystem.addUser.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.buildTrusty=function(){try{return tmpSystem.buildTrusty.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.delAHostMap=function(){try{return tmpSystem.delAHostMap.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.delGroup=function(){try{return tmpSystem.delGroup.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.delUser=function(){try{return tmpSystem.delUser.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getAHostMap=function(){try{return tmpSystem.getAHostMap.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getCpuInfo=function(){try{return tmpSystem.getCpuInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getCurrentUser=function(){try{return tmpSystem.getCurrentUser.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getDiskInfo=function(){try{return tmpSystem.getDiskInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getEWD=function(){try{return tmpSystem.getEWD.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getHostName=function(){try{return tmpSystem.getHostName.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getHostsMap=function(){try{return tmpSystem.getHostsMap.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getInfo=function(){try{return tmpSystem.getInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getIpTablesInfo=function(){try{return tmpSystem.getIpTablesInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getMemInfo=function(){try{return tmpSystem.getMemInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getNetcardInfo=function(){try{return tmpSystem.getNetcardInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getPID=function(){try{return tmpSystem.getPID.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getProcUlimitConfigs=function(){try{return tmpSystem.getProcUlimitConfigs.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getReleaseInfo=function(){try{return tmpSystem.getReleaseInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getSystemConfigs=function(){try{return tmpSystem.getSystemConfigs.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getTID=function(){try{return tmpSystem.getTID.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.getUserEnv=function(){try{return tmpSystem.getUserEnv.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.help=function(){try{return tmpSystem.help.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.isGroupExist=function(){try{return tmpSystem.isGroupExist.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.isProcExist=function(){try{return tmpSystem.isProcExist.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.isUserExist=function(){try{return tmpSystem.isUserExist.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.killProcess=function(){try{return tmpSystem.killProcess.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.listAllUsers=function(){try{return tmpSystem.listAllUsers.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.listGroups=function(){try{return tmpSystem.listGroups.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.listLoginUsers=function(){try{return tmpSystem.listLoginUsers.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.listProcess=function(){try{return tmpSystem.listProcess.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.ping=function(){try{return tmpSystem.ping.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.removeTrusty=function(){try{return tmpSystem.removeTrusty.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.runService=function(){try{return tmpSystem.runService.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.setProcUlimitConfigs=function(){try{return tmpSystem.setProcUlimitConfigs.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.setUserConfigs=function(){try{return tmpSystem.setUserConfigs.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.snapshotCpuInfo=function(){try{return tmpSystem.snapshotCpuInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.snapshotDiskInfo=function(){try{return tmpSystem.snapshotDiskInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.snapshotMemInfo=function(){try{return tmpSystem.snapshotMemInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.snapshotNetcardInfo=function(){try{return tmpSystem.snapshotNetcardInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.sniffPort=function(){try{return tmpSystem.sniffPort.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype.type=function(){try{return tmpSystem.type.apply(this,arguments);}catch(e){commThrowError(e);}};
System.prototype._getInfo=function(){try{return tmpSystem._getInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
