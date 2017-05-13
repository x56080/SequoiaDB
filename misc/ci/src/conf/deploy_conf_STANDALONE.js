/******************************************************
@decription:   configure for install_deploy/deploy_tpcc.js
               only variable defined
@input:        diagLevel: 0 1 2 3 4 5, default 3
@author:       Ting YU 2017-02-06   
******************************************************/

var mode = "standalone";

var cmPort          = 11790;
var tmpCoordPort    = 18800;
var coordnumPerhost = null;
var cataNum         = null;                           //total catalog number
var datagroupNum    = null;
var replSize        = null;
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

var nodeConf  = { diaglevel:diagLevel,
                  diagnum:30,
                  fap:fapValue
                };