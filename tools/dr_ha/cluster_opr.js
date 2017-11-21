/* ******************************************************************************
@Description: 集群操作脚本
@Modify list:
@   2016-01-19 Jianhui Xu  Init
@   2017-07-20 Jianhui Xu  引入参数动态生效，集群只读机制；引擎版本必须 2.8.2 及以上。
@   2017-11-15 Jiaming Wu  将串行的集群重启操作优化为并发操作。
****************************************************************************** */

/* SequoiaDB 安装目录定义，必须以 '/' 结尾 */
if ( typeof(SEQPATH) != "string" || SEQPATH.length == 0 ) { SEQPATH = "/opt/sequoiadb/" ; }
/* 机器登入用户名定义 */
if ( typeof(USERNAME) != "string" ) { USERNAME = "sdbadmin" ; }
/* 机器登入密码定义 */
if ( typeof(PASSWD) != "string" ) { PASSWD = "sdbadmin" ; }
/* 数据库登入用户名定义 */
if ( typeof(SDBUSERNAME) != "string" ) { SDBUSERNAME = "" ; }
/* 数据库登入密码定义 */
if ( typeof(SDBPASSWD) != "string" ) { SDBPASSWD = "" ; }
/* 子网1机器定义，必须为字符串数组 */
if ( typeof(SUB1HOSTS) == "undefined" ) { SUB1HOSTS = [ "vmsvr2-suse-x64-1" ] ; }
/* 子网2机器定义，必须为字符串数组 */
if ( typeof(SUB2HOSTS) == "undefined" ) { SUB2HOSTS = [ "vmsvr2-cent-x64" ] ; }
/* 协调节点定义，如果协调节点已经在 Catalog的编目组信息中，则此处填写一个可用Coord即可 */
if ( typeof(COORDADDR) == "undefined" ) { COORDADDR = [ "vmsvr2-suse-x64-1:50000" ] }
/* 当前子网取值, 1表示子网1，2表示子网2，其它取值非法 */
if ( typeof(CURSUB) == "undefined" ) { CURSUB = 1 ; }
/* 当前操作，取值 "init", "split", "merge" */
if ( typeof(CUROPR) == "undefined" ) { CUROPR = "split" ; }
/* 是否激活该子网集群，取值 true/false */
if ( typeof(ACTIVE) == "undefined" ) { ACTIVE = true ; }

/* 内部定义, 请勿修改 */
if ( SEQPATH.charAt( SEQPATH.length - 1 ) != '/' ) { SEQPATH += '/' ; }
var SDBSTART = SEQPATH + "bin/sdbstart" ;
var SDBSTOP  = SEQPATH + "bin/sdbstop" ;
var SDBLIST  = SEQPATH + "bin/sdblist" ;
var SDBSHELL = SEQPATH + "bin/sdb" ;
var CONFLOCAL= SEQPATH + "conf/local" ;
var INITFILE = SEQPATH + "datacenter_init.info" ;
var CURHOSTS = [] ;
var CURCATAS = [] ;   // only catalog
var CURDATAS = [] ;   // only data
var CURCOORDS= [] ;   // only coord
var CATAADDRLINE = "" ;
var READSIZE = 655360 ;

/* *****************************************************************************
@discription: 从地址中分解出Hostname和SvcName
@nodeAddr : address( string ), ex: '192.168.10.106:30000'
@author: Jianhui Xu
@return: string array
         ex: [ 'hostname', 'svcname' ]
***************************************************************************** */
function splitHostAndSvcFromAddr( nodeAddr ) {
   var infoArray = new Array() ;
   var pos = nodeAddr.indexOf( ":" ) ;
   if ( -1 != pos ) {
      infoArray[0] = nodeAddr.substring( 0, pos ) ;
      infoArray[1] = nodeAddr.substring( pos + 1 ) ;
   } else {
      infoArray[0] = nodeAddr ;
      infoArray[1] = "11810" ;
   }
   //println( "HostName: " + infoArray[0] + ", SvcName: " + infoArray[1] ) ;
   return infoArray ;
}

/* *****************************************************************************
@discription: 从Group Object Array中提取出编目，协调和数据组的节点信息
@objarray : group obj array, ex: [ { group info obj}, { group info obj} ]
@keepHosts : 过滤的 hostname 数组, ex: [ "a", "b" ]，当为空时表示不过滤
@author: Jianhui Xu
@return: array[][], array[0] for catalog nodes array
                    array[1] for data nodes array
                    array[2] for coord nodes array
        ex: [
                [ "192.168.20.106:30000"],
                [ "192.168.20.106:20000", "192.168.20.106:40000" ],
                [ "192.168.20.106:50000" ]
            ]
   error with exception
***************************************************************************** */
function parseGroupNodes( objarray, keepHosts ) {
   var retarray = new Array() ;
   retarray.push( new Array() ) ;  //0 catalog
   retarray.push( new Array() ) ;  //1 data
   retarray.push( new Array() ) ;  //2 coord
   var index = 0 ;

   for ( var i = 0 ; i < objarray.length ; ++i ) {
      var tmpObj = objarray[i] ;
      if ( 1 == tmpObj["GroupID"] ) {
         index = 0 ;
      } else if ( 2 == tmpObj["GroupID"] ) {
         index = 2 ;
      } else {
         index = 1 ;
      }
      var tmpGroupArray = tmpObj["Group"] ;
      for ( var j = 0 ; j < tmpGroupArray.length ; ++j ) {
         var tmpNodeObj = tmpGroupArray[j] ;
         var nodename = tmpNodeObj["HostName"] ;
         /* Filter jduge */
         if ( keepHosts.length != 0 && -1 == keepHosts.indexOf( nodename ) ) {
            /* the host will be filtered */
            continue ;
         }
         /* Get the svcname */
         for ( var k = 0 ; k < tmpNodeObj.Service.length ; ++k ) {
            var tmpSvcObj = tmpNodeObj.Service[k] ;
            if ( tmpSvcObj["Type"] == 0 ) {
               nodename = nodename + ":" + tmpSvcObj["Name"] ;
               retarray[index].push( nodename ) ;
               break ;
            }
         } // end for service
      } // end for node
   } // end for group

   return retarray ;
}

/* *****************************************************************************
@discription: 合并数据并踢重
@left : array
@right: array
@return: array
***************************************************************************** */
function mergeArrayWithoutRepeat( left, right ) {
   var newArray = new Array() ;
   /* merge left */
   for ( var i = 0 ; i < left.length ; ++i ) {
      if ( -1 == newArray.indexOf( left[i] ) ) {
         newArray.push( left[i] ) ;
      }
   }
   /* merge right */
   for ( var j = 0 ; j < right.length ; ++j ) {
      if ( -1 == newArray.indexOf( right[j] ) ) {
         newArray.push( right[j] ) ;
      }
   }
   return newArray ;
}

/* *****************************************************************************
@discription: 根据keepHosts生成新的nodes array，如果keepHosts为空，则全部有效
@nodesarray : array, ex:[ "192.168.20.106:20000" ]
@keepHosts: array, ex:[ "192.168.20.106" ]
@return: array
***************************************************************************** */
function makeNodesArrayWithKeepHosts( nodesarray, keepHosts ) {
   var newArray = new Array() ;
   /* merge right */
   for ( var i = 0 ; i < nodesarray.length ; ++i ) {
      var nodesinfo = splitHostAndSvcFromAddr( nodesarray[i] ) ;
      if ( keepHosts.length != 0 && -1 == keepHosts.indexOf( nodesinfo[0] ) ) {
         continue ;
      }
      newArray.push( nodesarray[i] ) ;
   }
   return newArray ;
}

/* *****************************************************************************
@discription: 根据keepHosts生成新的addr line，如果keepHosts为空，则不改变
@addrLine : string, ex:"r730-90:11823,r730-91:11823,r730-92:11823"
@keepHosts: array, ex:[ "192.168.20.106" ]
@return: string
***************************************************************************** */
function makeAddrLineWithKeepHosts( addrLine, keepHosts ) {
   if ( keepHosts.length == 0 ) {
      return addrLine ;
   }
   var addrArray = addrLine.split( "," ) ;
   var nodeInfo ;
   var newArray = new Array() ;
   for ( var i = 0 ; i < addrArray.length ; ++i ) {
      nodeInfo = splitHostAndSvcFromAddr( addrArray[i] ) ;
      if ( -1 == keepHosts.indexOf( nodeInfo[0] ) ) {
         continue ;
      }
      newArray.push( addrArray[i] ) ;
   }
   return newArray.join( "," ) ;
}

/* *****************************************************************************
@discription: 获取本机IP地址列表
@exceptLo: 是否排除回环地址，即：127.0.0.1
@author: Jianhui Xu
@return: string array
     ex: [ "127.0.0.1", "192.168.20.106" ]
***************************************************************************** */
function getHostIPs( exceptLo ) {
   if ( typeof( exceptLo ) == "undefined" ) {
      exceptLo = true ;
   }
   var retArray = new Array() ;
   var tmpInfo = System.getNetcardInfo() ;
   var obj = eval( '(' + tmpInfo + ')' ) ;
   var cardArray = obj["Netcards"] ;
   for ( var i = 0 ; i < cardArray.length ; ++i ) {
      if ( exceptLo && cardArray[i].Ip == "127.0.0.1" ) {
         continue ;
      }
      retArray.push( cardArray[i].Ip ) ;
   }
   return retArray ;
}

/* *****************************************************************************
@discription: 校验参数正确性
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function checkArgs() {
   if ( CURSUB != 1 && CURSUB != 2 ) {
      println( "CURSUB must be 1 or 2" ) ;
      return false ;
   }
   if ( CUROPR != "init" && CUROPR != "split" && CUROPR != "merge" ) {
      println( "CUROPR must be 'init', 'split' or 'merge' " ) ;
      return false ;
   }

   if ( COORDADDR.length == 0 ) {
      println( "COORDADDR is empty, you should config it first" ) ;
      return false ;
   }
   /* Value args */
   if ( CURSUB == 1 ) {
      CURHOSTS = SUB1HOSTS ;
   } else {
      CURHOSTS = SUB2HOSTS ;
   }
   if ( CURHOSTS.length == 0 ) {
      println( "CURHOSTS is empty, should need to set SUB1HOSTS and SUB2HOSTS" ) ;
      return false ;
   }
   /* Check current host is wether in CURHOSTS */
   var curHost = System.getHostName() ;
   if ( -1 == CURHOSTS.indexOf( curHost ) ) {
      // check ip is in the CURHOSTS
      var ipArray = getHostIPs( true ) ;
      var ipIn = false ;
      for ( var i = 0 ; i < ipArray.length ; ++i ) {
         if ( -1 != CURHOSTS.indexOf( ipArray[i] ) ) {
            ipIn = true ;
            break ;
         }
      }
      if ( false == ipIn ) {
         println( "Current host[" + curHost + "] is not in CURHOSTS: " + CURHOSTS + ". Make sure CURSUB value is right?" ) ;
      }
      return false ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 校验每台机器的环境配置
@hosts : 字符串数组（机器列表信息）
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function checkHostsEvn( hosts ) {
   var checkFiles = [ SDBSTART, SDBSTOP, SDBLIST, SDBSHELL, CONFLOCAL ] ;

   for ( i in hosts ) {
      var ssh ;
      try {
         ssh = new Ssh( hosts[i], USERNAME,  PASSWD ) ;
      } catch( e ) {
         println( "SSH to " + hosts[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }

      try {
         /* Check Files */
         for ( j in checkFiles ) {
           ssh.exec( 'ls ' + checkFiles[j] ) ;
         }
      } catch( e ) {
         println( "Check file[" + checkFiles[j] + "] in " + hosts[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         ssh.close() ;
         return false ;
      }

      /* Close the ssh connection */
      ssh.close() ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 将Catalog所有Group信息保存到filename中
@coordAddr : coord address(string)
@filename : string
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function saveGroupsInfo( coordAddr, filename ) {
   var db ;
   var file ;
   var number = 0 ;
   try {
      var addrArray = splitHostAndSvcFromAddr( coordAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + coordAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      file = new File( filename ) ;
   } catch( e ) {
      println( "Create or open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   try {
      var cursor = db.listReplicaGroups() ;
      while( cursor.next() ) {
         if ( 0 == number ) {
            file.write( "[\n" ) ;
         } else {
            file.write( ",\n" ) ;
         }
         file.write( cursor.current().toString() ) ;
         ++number ;
      }
      file.write( "\n]\n" ) ;
   } catch ( e ) {
      println( "Write info to  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      file.close() ;
      return false ;
   }

   db.close() ;
   file.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 从filename中读取保存的GroupsInfo，并转换成 [obj,obj]返回
@filename : string
@author: Jianhui Xu
@return: obj array
  error: throw exception
***************************************************************************** */
function readGroupsInfo( filename ) {
   var file ;
   var text ;
   var objarray = new Array() ;

   try {
      file = new File( filename ) ;
   } catch( e ) {
      println( "Open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }

   try {
      text = file.read( READSIZE ) ;
      var pos = text.indexOf( "%%%%" ) ;
      if ( -1 != pos ) {
         text = text.substring( 0, pos ) ;
      }
      objarray = eval( '(' + text + ')' ) ;
   } catch ( e ) {
      println( "Read info from  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      file.close() ;
      throw e ;
   }

   file.close() ;
   return objarray ;
}

/* *****************************************************************************
@discription: 根据active设置datacenter的readonly属性
              active:true  -> readonly:false
              active:false -> readonly:true
@cataAddr : catalog address(string)
@newAddrLine : string, ex : vmsvr2-suse-x64-1:30003,vmsvr2-suse-x64-1:30013
@active: true/false
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function updateDCInfoInCatalog( cataAddr, newAddrLine, active ) {
   var db ;
   var isReadOnly = false ;

   if ( !active ) { isReadOnly = true ; }
   try {
      var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      db.SYSINFO.SYSDCBASE.update( {$set:{Readonly:isReadOnly, 'DataCenter.Address':newAddrLine} }, {Type:"GLOBAL"} ) ;
   } catch ( e ) {
      println( "Update readonly to " + isReadOnly + " in " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   db.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 将Catalog所有Group信息中非keepHosts的Host信息删除
@cataAddr : catalog address(string)
@keepHosts : hostname数组
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function updateGroupsInCatalog( cataAddr, keepHosts ) {
   var db ;

   try {
      var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   var tmpGroupInfo ;
   try {
      tmpGroupInfo = db.SYSCAT.SYSNODES.find().toArray() ;
   } catch ( e ) {
      println( "Get groups info failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   /* filter group by hostname */
   for ( var i = 0 ; i < tmpGroupInfo.length ; ++i ) {
      var tmpGroupObj = eval( "(" + tmpGroupInfo[i] + ")" ) ;
      var tmpFieldGroup = tmpGroupObj["Group"] ;
      var newFieldGroup = new Array() ;
      var kickNum = 0 ;

      for ( var j = 0 ; j < tmpFieldGroup.length ; ++j ) {
         var tmpHostObj = tmpFieldGroup[ j ] ;
         /* Find in the keepHosts */
         if ( -1 != keepHosts.indexOf( tmpHostObj.HostName ) ) {
           newFieldGroup.push( tmpHostObj ) ;
         } else {
            ++kickNum ;
            println( "Kick host[" + tmpHostObj.HostName + "] from group[" + tmpGroupObj.GroupName + "]" ) ;
         }
      }

      /* Update to db */
      if ( kickNum > 0 ) {
         try {
            db.SYSCAT.SYSNODES.update( { '$set':{ 'Group' : newFieldGroup } }, { '_id': tmpGroupObj['_id'] } ) ;
            println( "Update kicked group[" + tmpGroupObj.GroupName + "] to " + cataAddr + " succeed" ) ;
         } catch ( e ) {
            println( "Update kicked group info to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            db.close() ;
            return false ;
         }
      } else {
         println( "Group[" + tmpGroupObj.GroupName + "] not change in " + cataAddr ) ;
      }
   }

   db.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 恢复Catalog所有Group信息
@cataAddr : catalog address(string)
@groupsArray : obj array
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function restoreGroupsInCatalog( cataAddr, groupsArray ) {
   var db ;

   try {
      var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
      db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
   } catch ( e ) {
      println( "Connect to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   /* filter group by hostname */
   for ( var i = 0 ; i < groupsArray.length ; ++i ) {
      var tmpGroupObj = groupsArray[i] ;
      var tmpFieldGroup = tmpGroupObj["Group"] ;

      /* Update to db */
      try {
         db.SYSCAT.SYSNODES.update( { '$set':{ 'Group' : tmpFieldGroup } }, { 'GroupName': tmpGroupObj['GroupName'] } ) ;
         println( "Restore group[" + tmpGroupObj.GroupName + "] to " + cataAddr + " succeed" ) ;
      } catch ( e ) {
         println( "Restore group info to " + cataAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         db.close() ;
         return false ;
      }
   }

   db.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 获取远端节点的配置
@address : node address(string), ex: "192.168.20.106:20000"
@author: Jianhui Xu
@return: cofig obj
  error: with exception
***************************************************************************** */
function getConfigObj( address ) {
   var addrArray = splitHostAndSvcFromAddr( address ) ;
   var ssh ;
   var obj ;
   /* Ssh to remote host */
   try {
      ssh = new Ssh( addrArray[0], USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + addrArray[0] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }
   /* Begin to get config object from remote host */
   try {
      var conffile = CONFLOCAL + "/" + addrArray[1] + "/sdb.conf" ;
      var tmpString = ssh.exec( SDBSHELL + ' -s "Oma.getOmaConfigs( ' + "'" + conffile + "'" + ' )" ' ) ;
      ssh.exec( SDBSHELL + ' -s quit ' ) ;
      ssh.close() ;
      obj = eval( '(' + tmpString + ')' ) ;
   } catch ( e ) {
      println( "Get config object from " + address + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      throw e ;
   }

   return obj ;
}

/* *****************************************************************************
@discription: 保存配置到远端节点中
@address : node address(string), ex: "192.168.20.106:20000"
@obj : config object
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function saveConfigObj( address, obj ) {
   var addrArray = splitHostAndSvcFromAddr( address ) ;
   var ssh ;
   /* Ssh to remote host */
   try {
      ssh = new Ssh( addrArray[0], USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + addrArray[0] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }
   /* Begin to save config object to remote host */
   try {
      var conffile = CONFLOCAL + "/" + addrArray[1] + "/sdb.conf" ;
      var objstring = JSON.stringify( obj ) ;
      objstring = objstring.replace( /\"/g, "\\\"" ) ;
      ssh.exec( SDBSHELL + ' -s " var obj = ' +  objstring + ' " ; ' ) ;
      ssh.exec( SDBSHELL + ' -s "Oma.setOmaConfigs( obj, ' + "'" + conffile + "'" + ' ) ; " ' ) ;
      ssh.exec( SDBSHELL + ' -s quit ' ) ;
      ssh.close() ;
   } catch ( e ) {
      println( "Save config object to " + address + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      return false ;
   }

   return true ;
}

/* *****************************************************************************
@discription: 更新节点集的配置
@nodesArray : node address array
@key :
@value :
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function updateNodesConfig( nodesArray, key, value ) {
   for ( var i = 0 ; i < nodesArray.length ; ++i ) {
      var obj ;
      try {
         obj = getConfigObj( nodesArray[i] ) ;
      } catch ( e ) {
         println( "Get config obj from " + nodesArray[i] + " failed: " + e ) ;
         return false ;
      }
      if ( typeof( obj[ key ] ) == "undefined" || obj[key] != value ) {
         obj[key] = value ;
         if ( !saveConfigObj( nodesArray[i], obj ) ) {
            println( "Save config obj to " + nodesArray[i] + " failed" ) ;
            return false ;
         }
      }
   }
   return true ;
}

/* *****************************************************************************
@discription: 动态生效节点配置
@nodesArray : node address array
@ignoreErrArray : ignore error code array
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function reloadNodesConf( nodesArray, ignoreErrArray ) {
   for ( var i = 0 ; i < nodesArray.length ; ++i ) {
      var addrArray = splitHostAndSvcFromAddr( nodesArray[i] ) ;
      try {
         var db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
         db.reloadConf( { Global : false } ) ;
         db.close() ;
      } catch ( e ) {
         /* When the error in ignoredErrArray, make sure is succeed */
         if ( -1 == ignoreErrArray.indexOf( e ) ) {
            println( "Reload config from " +  nodesArray[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
            return false ;
         }
      }
   }

   return true ;
}

/* *****************************************************************************
@discription: 让所有组重选主
@coordAddr : coord address(string)
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function reelectAllGroups( coordAddrs ) {
   var db ;
   var get = false ;
   for ( var i = 0 ; i < coordAddrs.length ; ++i ) {
      try {
         var addrArray = splitHostAndSvcFromAddr( coordAddrs[i] ) ;
         db = new Sdb( addrArray[0], addrArray[1], SDBUSERNAME, SDBPASSWD ) ;
      } catch ( e ) {
         println( "Connect to " + coordAddrs[i] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         continue ;
      }

      /* Get replica groups */
      try {
         var cursor = db.listReplicaGroups() ;
         while ( cursor.next() ) {
            var obj = eval( '(' + cursor.current().toString() + ')' ) ;
            if ( 1 != obj["GroupID"] && obj["GroupID"] < 1000 ) {
               continue ;
            }
            rg = db.getRG( obj["GroupName"] ) ;
            try {
               rg.reelect( { Seconds : 20 } ) ;
            } catch ( e ) {
               /* Ignore all error */
            }
         }
         get = true ;
      } catch ( e ) {
         println( "Reelect all groups failed: " + e +  "(" + getLastErrMsg() + ")" ) ;
      }

      db.close() ;
      break ;
   }

   return get ;
}

/* *****************************************************************************
@discription: 将catalog address line保存到文件
@coordAddr : coord address(string)
@filename : string
@author: Jianhui Xu
@return: true / false
***************************************************************************** */
function saveCatalogAddrLine( coordAddr, filename ) {
   var addrstring = "" ;

   try {
      var tmpObj = getConfigObj( coordAddr ) ;
      addrstring = tmpObj["catalogaddr"] ;
   } catch ( e ) {
      println( "Get catalog address line from " + coordAddr + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      file = new File( filename ) ;
      file.seek( 0, 'e' ) ;
   } catch( e ) {
      println( "Create or open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      db.close() ;
      return false ;
   }

   try {
      file.write( "\n%%%%" + addrstring + "%%%%\n" ) ;
   } catch ( e ) {
      println( "Write catalog address line to  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      file.close() ;
      return false ;
   }

   file.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 从filename中读取保存的catalog address line，并转换成string返回
@filename : string
@author: Jianhui Xu
@return: string
  error: throw exception
***************************************************************************** */
function readCatalogAddressLine( filename ) {
   var file ;
   var text ;

   try {
      file = new File( filename ) ;
   } catch( e ) {
      println( "Open file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      throw e ;
   }

   try {
      text = file.read( READSIZE ) ;
      var pos = text.indexOf( "%%%%" ) ;
      if ( -1 != pos ) {
         text = text.substring( pos + 4 ) ;
         var posend = text.indexOf( "%%%%" ) ;
         if ( -1 != posend ) {
            text = text.substring(0, posend ) ;
         }
      }
   } catch ( e ) {
      println( "Read info from  file[" + filename + "] failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      file.close() ;
      throw e ;
   }

   file.close() ;
   return text ;
}

/* *****************************************************************************
@discription: 将Catalog节点改为standalone模式启动
@cataAddr : catalog address( string ), ex: '192.168.10.106:30000'
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function change2Standalone( cataAddr ) {
   var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
   var ssh ;
   /* Stop the node and start by standalone */
   try {
      ssh = new Ssh( addrArray[0], USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + addrArray[0] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      ssh.exec( SDBSTOP + " -p " + addrArray[1] ) ;
      println( "Stop " + addrArray[1] + " succeed in " + addrArray[0] ) ;
      var cmdline = SDBSTART + " -p " + addrArray[1] + ' -o "--role standalone" ' ;
      // println( "CommandLine: " + cmdline ) ;
      ssh.exec( cmdline ) ;
      println( "Start " + addrArray[1] + " by standalone succeed in " + addrArray[0] ) ;
   } catch ( e ) {
      println( "Change " + cataAddr + " to standalone failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      return false ;
   }
   ssh.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 将Catalog节点改为Catalog角色模式启动
@cataAddr : catalog address( string ), ex: '192.168.10.106:30000'
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function change2Catalog( cataAddr ) {
   var addrArray = splitHostAndSvcFromAddr( cataAddr ) ;
   var ssh ;
   /* Restore the node and start */
   try {
      ssh = new Ssh( addrArray[0], USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + addrArray[0] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      ssh.exec( SDBSTOP + " -p " + addrArray[1] ) ;
      println( "Stop " + addrArray[1] + " succeed in " + addrArray[0] ) ;
      var cmdline = SDBSTART + " -p " + addrArray[1] ;
      //println( "CommandLine: " + cmdline ) ;
      ssh.exec( cmdline ) ;
      println( "Restore " + addrArray[1] + " to catalog succeed in " + addrArray[0] ) ;
   } catch ( e ) {
      println( "Restore " + cataAddr + " to catalog failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      return false ;
   }
   ssh.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 重启hostname中的所有SequoiaDB节点
@hostname : string
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function restartAllNode( hostname ) {
   var ssh ;
   /* Stop and start  */
   try {
      ssh = new Ssh( hostname, USERNAME, PASSWD ) ;
   } catch ( e ) {
      println( "Ssh to " + hostname+ " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      return false ;
   }

   try {
      ssh.exec( SDBSTOP + " -t all " ) ;
      println( "Stop all nodes succeed in " + hostname ) ;
      ssh.exec( SDBSTART + " -t all " ) ;
      println( "Start all nodes succeed in " + hostname ) ;
   } catch ( e ) {
      println( "Restart all nodes in " + hostname + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
      ssh.close() ;
      return false ;
   }
   ssh.close() ;
   return true ;
}

/* *****************************************************************************
@discription: 重启所有主机中的所有SequoiaDB节点
@hostnameArr : String Array[]
@author: Jiaming Wu
@return: true/false
***************************************************************************** */
function restartAllHostNode( hostnameArr ) {

   var hostNumber = hostnameArr.length ;
   var svcnameArr = new Array( hostNumber ) ;
   var nodeInfo = new Array( hostNumber ) ;

   /* Get svcname  */
   for ( var j = 0 ; j < hostNumber ; ++j ) {
      svcnameArr[ j ] = Oma.getAOmaSvcName( hostnameArr[ j ] ) ;
   }

   /* Restart all host */
   var restartJob = new Array( hostNumber ) ;
   for ( var j = 0 ; j < hostNumber ; ++j ) {
      try
      {
         var oma = new Oma( hostnameArr[ j ], svcnameArr[ j ] ) ;
         nodeInfo[ j ] = oma.listNodes() ;
      }
      catch( e )
      {
         println( "List " + hostnameArr[ j ] + " nodes failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }

      try {
         ssh = new Ssh( hostnameArr[ j ], USERNAME, PASSWD ) ;
      } catch ( e ) {
         println( "Ssh to " + hostnameArr[ j ] + " failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }
      try
      {
         var retStr = ssh.exec( SDBSHELL + ' -s \'var cmd = new Cmd(); cmd.start( "'
                                + SDBSTOP + ' -t all && ' + SDBSTART + ' -t all", "", 1, 0  ) ; \' ' ) ;
         var pid = retStr.split( "\n" )[ 0 ] ;
         restartJob[ j ] = { "pid" : "" + pid } ;
      }
      catch( e )
      {
         println( "Restart " + hostnameArr[ j ] + " node failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }
   }

   /* Check whether all backgroup stop job have been completed  */
   var sysArr = new Array( hostNumber ) ;
   var finishFlagArr = new Array( hostNumber ) ;
   var finishNumber = 0 ;
   var remoteArr = new Array( hostNumber ) ;

   for( var j = 0; j < hostNumber; ++j ) {
      /* Get remote obj */
      try
      {
         remoteArr[ j ] = new Remote( hostnameArr[ j ], svcnameArr[ j ] ) ;
      }
      catch( e )
      {
         println( "Get " + hostnameArr[ j ] + " remote obj failed: " + e + "(" + getLastErrMsg() + ")" ) ;
         return false ;
      }
      finishFlagArr[ j ] = false ;
      sysArr[ j ] = remoteArr[ j ].getSystem() ;
   }
   while( finishNumber < hostNumber )
   {
      for( var j = 0; j < hostNumber; ++j )
      {
         if( finishFlagArr[ j ] != true )
         {
            var listProc = sysArr[ j ].listProcess( {}, restartJob[ j ] ) ;
            if( listProc.size() == 0 )
            {
               finishFlagArr[ j ] = true ;
               finishNumber++ ;
            }
         }
      }
      sleep( 500 ) ;
   }

   // Compare starttime to determine whether the node have been restart
   for ( var j = 0 ; j < hostNumber ; ++j ) {
      var currentNodeInfo ;
      var localNodeInfo ;
      try
      {
         var oma = new Oma( hostnameArr[ j ], svcnameArr[ j ] ) ;
         currentNodeInfo = oma.listNodes() ;
         localNodeInfo = oma.listNodes( { mode: "local" } ) ;
      }
      catch( e )
      {
         println( "Failed to get cluster node info" ) ;
         return false ;
      }
      if( currentNodeInfo.size() != localNodeInfo.size() )
      {
         println( "Start " + hostnameArr[ j ] + " node failed" ) ;
         return false ;
      }

      var nodeInfoArray = [] ;
      while ( true ) {
         var bs = nodeInfo[ j ].next();
         if ( ! bs ) break ;
         nodeInfoArray.push ( bs.toObj() ) ;
      }

      while( currentNodeInfo.more() ) {
         var node = currentNodeInfo.next().toObj() ;
         var filter = new _Filter( { "svcname": node.svcname } ) ;
         var result = filter.match( nodeInfoArray ) ;
         if( result.size() != 1 )
         {
            continue ;
         }
         if( node.starttime == result.next().toObj().starttime )
         {
            println( "Stop " + hostnameArr[ j ] + " node failed" ) ;
            return false ;
         }
      }
      println( "Restart all nodes succeed in " + hostnameArr[ j ] ) ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 准备运行时的环境信息
@filename: string
@keepHosts : array
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function prepareEnv( filename, keepHosts ) {
   if ( !File.exist( filename ) ) {
      println( "File[" + filename + "] not exist, you should init with the file first" ) ;
      return false ;
   }

   var objarray ;
   try {
      objarray = readGroupsInfo( filename ) ;
   } catch ( e ) {
      println( "Read groups info from file[" + filename + "] failed: " + e ) ;
      return false ;
   }
   try {
      CATAADDRLINE = readCatalogAddressLine( filename ) ;
   } catch( e ) {
      println( "Read catalog address line failed: " + e ) ;
      return false ;
   }
   /* Parse nodes */
   var nodesarray ;
   try {
      nodesarray = parseGroupNodes( objarray, keepHosts ) ;
   } catch ( e ) {
      println( "Parse group nodes failed: " + e ) ;
      return false ;
   }
   CURCATAS = nodesarray[0] ; // catalog
   CURDATAS = nodesarray[1] ;  // data
   CURCOORDS = mergeArrayWithoutRepeat( nodesarray[2], makeNodesArrayWithKeepHosts( COORDADDR, keepHosts ) ) ;
   if ( CURCATAS.length == 0 ) {
      println( "Catalog nodes is empty" ) ;
      return false ;
   }
   return true ;
}

/* *****************************************************************************
@discription: 切分大集群，将非keepHosts的host从cataAddrs的保存组信息中踢除
@cataAddrs : string array, ex: [ '192.168.20.106:30000' ]
@keepHosts : string array, ex: [ 'test1', 'test2' ]
@active : true/false
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function splitCluster( cataAddrs, keepHosts, active ) {
   var newAddrLine = makeAddrLineWithKeepHosts( CATAADDRLINE, keepHosts ) ;
   for ( var i = 0 ; i < cataAddrs.length ; ++i  ) {
      /* 1. Change catalog to standalone */
      if ( change2Standalone( cataAddrs[ i ] ) ) {
         println( "Change " + cataAddrs[ i ] + " to standalone succeed"  ) ;
      } else {
         println( "Change " + cataAddrs[ i ] + " to standalone failed"  ) ;
         return false ;
      }
      /* 2. Update catalog groups info (kick the hosts) */
      if ( updateGroupsInCatalog( cataAddrs[i], keepHosts ) ) {
         println( "Update " + cataAddrs[ i ] + " catalog's info succeed"  ) ;
      } else {
         println( "Update " + cataAddrs[ i ] + " catalog's info failed"  ) ;
         return false ;
      }
      /* 3. Update catalog datacenter readonly prop */
      if ( updateDCInfoInCatalog( cataAddrs[i], newAddrLine, active ) ) {
         println( "Update " + cataAddrs[i] + " catalog's readonly prop succeed" ) ;
      } else {
         println( "Update " + cataAddrs[i] + " catalog's readonly prop failed" ) ;
         return false ;
      }
      /* 4. Restore to catalog */
      /* Not change, at last restart all nodes
      if ( change2Catalog( cataAddrs[ i ] ) ) {
         println( "Restore " + cataAddrs[ i ] + " to catalog succeed"  ) ;
      } else {
         println( "Restore " + cataAddrs[ i ] + " to catalog failed"  ) ;
         return false ;
      } */
   }

   /* 5. Update all node's addr--kick host */
   var allNodes = mergeArrayWithoutRepeat( CURCATAS, mergeArrayWithoutRepeat( CURDATAS, CURCOORDS ) ) ;
   if ( updateNodesConfig( allNodes, "catalogaddr", newAddrLine ) ) {
      println( "Update all nodes's catalogaddr to " + newAddrLine + " succeed" ) ;
   } else {
      println( "Update all nodes's catalogaddr to " + newAddrLine + " failed" ) ;
      return false ;
   }

   /* 6. Restart all keepHosts's nodes */
   if ( restartAllHostNode( keepHosts ) ) {
      println( "Restart all host nodes succeed"  ) ;
   } else {
      println( "Restart all host nodes failed"  ) ;
      return false ;
   }

   return true ;
}

/* *****************************************************************************
@discription: 初始化大集群，将大集群的组信息和Catalog Address line全部保存
@coordAddrs : string array, ex: [ '192.168.20.106:30000' ]
@filename : string
@active : true/false
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function initCluster( coordAddrs, filename, active ) {
   if ( File.exist( filename ) ) {
      println( "Already init. If you want to re-init, you should to remove the file: " + filename ) ;
      return false ;
   }
   var init = false ;
   for ( var i = 0 ; i < coordAddrs.length ; ++i  ) {
      try { File.remove( filename ) ; } catch( e ) {}
      if ( saveGroupsInfo( coordAddrs[i], filename ) ) {
         if ( saveCatalogAddrLine( coordAddrs[i], filename ) ) {
            init = true ;
            break ;
         }
      }
   }
   if ( false == init ) {
      println( "Init failed" ) ;
      try { File.remove( filename ) ; } catch( e ) {}
      return false ;
   }
   /* prepare env and set weigth */
   if ( !prepareEnv( filename, CURHOSTS ) ) {
      println( "Prepare env failed" ) ;
      try { File.remove( filename ) ; } catch( e ) {}
      return false ;
   }
   var weigth = 10 ;
   if ( active ) {
      weigth = 100 ;
   }
   // update catalog and datanode
   print( "Begin to update catalog and data nodes's config..." ) ;
   if ( !updateNodesConfig( mergeArrayWithoutRepeat( CURCATAS, CURDATAS ), "weight", weigth ) ) {
      println( "Update catalog and data node's config failed" ) ;
      try { File.remove( filename ) ; } catch( e ) {}
      return false ;
   } else {
      println( "Done" ) ;
   }
   /* Reload all catalog and datanode */
   print( "Begin to reload catalog and data nodes's config..." ) ;
   if ( !reloadNodesConf( mergeArrayWithoutRepeat( CURCATAS, CURDATAS ), [ -15 ] ) ) {
      println( "Reload catalog and data node's config failed" ) ;
      try { File.remove( filename ) ; } catch( e ) {}
      return false ;
   } else {
      println( "Done" ) ;
   }
   /* Reelect all groups and ignore the error */
   print( "Begin to reelect all groups..." ) ;
   if ( !reelectAllGroups( coordAddrs ) ) {
      println( "WARNING: Reelect all groups failed" ) ;
   } else {
      println( "Done" ) ;
   }

   return true ;
}

/* *****************************************************************************
@discription: 合并大集群，根据INIT保存的文件恢复Catalog所有组信息
@cataAddrs : string array, ex: [ '192.168.20.106:30000' ]
@keepHosts : string array, ex: [ 'test1', 'test2' ]
@filename : string
@active : true/false
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function mergeCluster( cataAddrs, keepHosts, filename, active ) {
   var groupsArray ;
   try {
      groupsArray = readGroupsInfo( filename ) ;
   } catch ( e ) {
      println( "Read groups info from file[" + filename + "] failed: " + e ) ;
      return false ;
   }

   for ( var i = 0 ; i < cataAddrs.length ; ++i  ) {
      /* 1. Change catalog to standalone */
      if ( change2Standalone( cataAddrs[ i ] ) ) {
         println( "Change " + cataAddrs[ i ] + " to standalone succeed"  ) ;
      } else {
         println( "Change " + cataAddrs[ i ] + " to standalone failed"  ) ;
         return false ;
      }
      /* 2. Update catalog groups info (restore the hosts) */
      if ( restoreGroupsInCatalog( cataAddrs[i], groupsArray ) ) {
         println( "Restore " + cataAddrs[ i ] + " catalog's info succeed"  ) ;
      } else {
         println( "Restore " + cataAddrs[ i ] + " catalog's info failed"  ) ;
         return false ;
      }
      /* 3. Update catalog datacenter readonly prop */
      if ( updateDCInfoInCatalog( cataAddrs[i], CATAADDRLINE, true ) ) {
         println( "Update " + cataAddrs[i] + " catalog's readonly prop succeed" ) ;
      } else {
         println( "Update " + cataAddrs[i] + " catalog's readonly prop failed" ) ;
         return false ;
      }
      /* 4. Restore to catalog */
      /* Not change, at last restart all nodes
      if ( change2Catalog( cataAddrs[ i ] ) ) {
         println( "Restore " + cataAddrs[ i ] + " to catalog succeed"  ) ;
      } else {
         println( "Restore " + cataAddrs[ i ] + " to catalog failed"  ) ;
         return false ;
      } */
   }

   /* 6. Update all all node's addr */
   var allNodes = mergeArrayWithoutRepeat( CURCATAS, mergeArrayWithoutRepeat( CURDATAS, CURCOORDS ) ) ;
   if ( updateNodesConfig( allNodes, "catalogaddr", CATAADDRLINE ) ) {
      println( "Update all nodes's catalogaddr to " + CATAADDRLINE + " succeed" ) ;
   } else {
      println( "Update all nodes's catalogaddr to " + CATAADDRLINE + " failed" ) ;
      return false ;
   }

   /* 7. Restart all keepHosts's nodes */
   if ( restartAllHostNode( keepHosts ) ) {
      println( "Restart all host nodes succeed"  ) ;
   } else {
      println( "Restart all host nodes failed"  ) ;
      return false ;
   }

   return true ;
}

/* *****************************************************************************
@discription: 入口函数
@author: Jianhui Xu
@return: true/false
***************************************************************************** */
function main() {
   println( "Begin to check args..." ) ;
   if ( checkArgs() ) {
      println( "Done" ) ;
   } else {
      println( "Failed" ) ;
      return ;
   }
   println( "Begin to check enviroment..." ) ;
   if ( checkHostsEvn( CURHOSTS ) ) {
      println( "Done" ) ;
   } else {
      println( "Failed" ) ;
      return ;
   }

   /* Doing */
   if ( "init" == CUROPR ) {
      println( "Begin to init cluster..." ) ;
      if ( initCluster( COORDADDR, INITFILE, ACTIVE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else if ( "split" == CUROPR ) {
      println( "Begin to split cluster..." ) ;
      if ( !prepareEnv( INITFILE, CURHOSTS ) ) {
         println( "Prepare env failed" ) ;
         return false ;
      }
      if ( splitCluster( CURCATAS, CURHOSTS, ACTIVE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else if ( "merge" == CUROPR ) {
      println( "Begin to merge cluster..." ) ;
      if ( !prepareEnv( INITFILE, CURHOSTS ) ) {
         println( "Prepare env failed" ) ;
         return false ;
      }
      if ( mergeCluster( CURCATAS, CURHOSTS, INITFILE, ACTIVE ) ) {
         println( "Done" ) ;
      } else {
         println( "Failed" ) ;
         return ;
      }
   } else {
      println( "Unknow command[" + CUROPR + "]" ) ;
      throw "Unknow command" ;
   }
}

main() ;
