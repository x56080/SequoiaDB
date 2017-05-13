/******************************************************
@decription:   configure for install_deploy/deploy_tpcc.js
               only variable defined
@author:       CSQ 2017-03-22   
******************************************************/

var mode = "cluster";

var cmPort          = 11790;
var tmpCoordPort    = 18800;
var coordnumPerhost = 1;
var cataNum         = 3;                           //total catalog number
var datagroupNum    = 3;
var replSize        = 3;
var diskList        = [                             //disks for dbPath
                        '/data/sequoiadb'
                      ];
                     
if( typeof( diagLevel ) === "undefined" ) 
{
   var diagLevel = 3;
}

var cataConf  = { diaglevel:diagLevel,
                  sharingbreak:30000,
                  diagnum:30,
                  diagpath:'/opt/sequoiadb/database/cata/[svcname]/diaglog',
                  transactionon:true
                };
var coordConf = { diaglevel:diagLevel,
                  diagnum:30,
                  diagpath:'/opt/sequoiadb/database/coord/[svcname]/diaglog'
                };
var dataConf  = { diaglevel:diagLevel,
                  sharingbreak:30000,
                  diagnum:30,
                  diagpath:'/opt/sequoiadb/database/data/[svcname]/diaglog'
                };
                