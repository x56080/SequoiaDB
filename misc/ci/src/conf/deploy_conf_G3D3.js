/******************************************************
@decription:   configure for install_deploy/deploy_tpcc.js
               only variable defined
@input:        diagLevel: 0 1 2 3 4 5, default 3
@author:       Ting YU 2017-02-06   
******************************************************/

var mode = "cluster";

var cmPort          = 11790;
var tmpCoordPort    = 18800;
var coordnumPerhost = 1;
var cataNum         = 3;                           //total catalog number
var datagroupNum    = 3;
var replSize        = 3;
var diskList        = [ '/opt/sequoiadb' ];        //disks for dbPath

if( typeof( diagLevel ) === "undefined" ) 
{
   var diagLevel = 3;
}

var osArch = new Cmd().run('arch');
if ( osArch === "x86_64\n" ) 
{ 
   var fapValue = "fapmongo";
}
else
{
   var fapValue = "";
}

var cataConf  = { diaglevel:diagLevel,
                  sharingbreak:30000,
                  diagnum:30,
                  optimeout:60000,
                  fap:fapValue,
                  transactionon:true
                };
var coordConf = { diaglevel:diagLevel,
                  diagnum:30,
                  optimeout:60000,
                  fap:fapValue 
                };
var dataConf  = { diaglevel:diagLevel,
                  sharingbreak:30000,
                  diagnum:30,
                  optimeout:60000,
                  fap:fapValue
                };