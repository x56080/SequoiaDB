/******************************************************
@decription:   configure for install_deploy/deploy_tpcc.js
               only variable defined
@author:       Ting YU 2017-02-06   
******************************************************/

var mode = "cluster";

var cmPort          = 11790;
var tmpCoordPort    = 18800;
var coordnumPerhost = 2;
var cataNum         = 3;                           //total catalog number
var datagroupNum    = 11;
var replSize        = 3;
var diskList        = [                             //disks for dbPath
                        '/data/disk01/sequoiadb',
                        '/data/disk02/sequoiadb',
                        '/data/disk03/sequoiadb',
                        '/data/disk04/sequoiadb',
                        '/data/disk05/sequoiadb',
                        '/data/disk06/sequoiadb',
                        '/data/disk07/sequoiadb',
                        '/data/disk08/sequoiadb',
                        '/data/disk09/sequoiadb',
                        '/data/disk10/sequoiadb',
                        '/data/disk11/sequoiadb',
                        '/data/disk12/sequoiadb'
                      ];
                   
var cataConf  = {};
var coordConf = {};
var dataConf  = {};