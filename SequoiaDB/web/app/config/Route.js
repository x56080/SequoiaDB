(function(){
   window.SdbSacManagerConf.nowRoute = [
      { path: '/Transfer',
        options: {
           templateUrl: './app/template/Public/Transfer.html',
           resolve: resolveFun( [ './app/controller/Transfer.js' ] )
        }
      },
      { path: '/Index/Index/Index',
        options: {
           templateUrl: './app/template/Index/Index/Index.html',
           resolve: resolveFun( [ './app/controller/Index/Index/Index.js' ] )
        }
      },
      /*
      { path: '/Data/SDB-Overview/Index',
        options: {
           templateUrl: './app/template/Data/Sdb/Overview/Index.html',
           resolve: resolveFun( [ './app/other/function/Data/Sdb/Overview/Index.js', './app/controller/Data/Sdb/Overview/Index.js' ] )
        }
      },
      { path: '/Data/SDB-Operate/Index',
        options: {
           templateUrl: './app/template/Data/Sdb/Operate/Index.html',
           resolve: resolveFun( [ './app/other/function/Data/Sdb/Operate/Index.js', './app/controller/Data/Sdb/Operate/Index.js' ] )
        }
      },
      */
      { path: '/Data/SDB-Operate/Record',
        options: {
           templateUrl: './app/template/Data/Sdb/Operate/Record.html',
           resolve: resolveFun( [ './app/controller/Data/Sdb/Operate/Record.js' ] )
        }
      },
      { path: '/Data/SDB-Operate/Lobs',
        options: {
           templateUrl: './app/template/Data/Sdb/Operate/Lobs.html',
           resolve: resolveFun( [ './app/other/function/Data/Sdb/Operate/Lob.js', './app/controller/Data/Sdb/Operate/Lobs.js' ] )
        }
      },
      { path: '/Data/SDB-Database/Index',
        options: {
           templateUrl: './app/template/Data/Sdb/Database/Index.html',
           resolve: resolveFun( [ './app/other/function/Data/Sdb/Database/Index.js', './app/controller/Data/Sdb/Database/Index.js' ] )
        }
      },
      { path: '/Data/SQL-Overview/Index',
        options: {
           templateUrl: './app/template/Data/Sql/Overview/Index.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Overview/Index.js' ] )
        }
      },
      { path: '/Data/SQL-Metadata/Index',
        options: {
           templateUrl: './app/template/Data/Sql/Metadata/Index.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Metadata/Index.js' ] )
        }
      },
      { path: '/Data/SQL-Operate/Data',
        options: {
           templateUrl: './app/template/Data/Sql/Operate/Data.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Operate/Data.js' ] )
        }
      },
      { path: '/Data/SQL-Operate/Structure',
        options: {
           templateUrl: './app/template/Data/Sql/Operate/Structure.html',
           resolve: resolveFun( [ './app/controller/Data/Sql/Operate/Structure.js' ] )
        }
      },
      { path: '/Data/HDFS-web/Index',
        options: {
           templateUrl: './app/template/Data/Other/web.html',
           resolve: resolveFun( [ './app/controller/Data/Other/web.js' ] )
        }
      },
      { path: '/Data/SPARK-web/Index',
        options: {
           templateUrl: './app/template/Data/Other/web.html',
           resolve: resolveFun( [ './app/controller/Data/Other/web.js' ] )
        }
      },
      { path: '/Data/YARN-web/Index',
        options: {
           templateUrl: './app/template/Data/Other/web.html',
           resolve: resolveFun( [ './app/controller/Data/Other/web.js' ] )
        }
      },
      { path: '/Data/NotSupport',
        options: {
           templateUrl: './app/template/Data/Other/notsupport.html',
           resolve: resolveFun( [ './app/controller/Data/Other/notsupport.js' ] )
        }
      },
      { path: '/Data/Edition',
        options: {
           templateUrl: './app/template/Data/Other/edition.html',
           resolve: resolveFun( [ './app/controller/Data/Other/edition.js' ] )
        }
      },
      { path: '/Strategy/SDB/Index',
        options: {
           templateUrl: './app/template/Strategy/Sdb/Index.html',
           resolve: resolveFun( [ './app/controller/Strategy/Sdb/Index.js' ] )
        }
      },
      //监控主页
      { path: '/Monitor/Index',
        options: {
           templateUrl: './app/template/Monitor/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Index.js' ] )
         }
      },
      { path: '/Monitor/Charts',
        options: {
           templateUrl: './app/template/Monitor/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Charts.js' ] )
         }
      },
      //SDB监控页
      { path: '/Monitor/SDB-Overview/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Overview/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Overview/Index.js' ] )
         }
      },
      { path: '/Monitor/SDB-Overview/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Overview/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Overview/Charts.js' ] )
         }
      },
      { path: '/Monitor/SDB-Overview/Session',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Overview/Session.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Overview/Session.js' ] )
         }
      },
      { path: '/Monitor/SDB-Overview/Node',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Overview/Node.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Overview/Node.js' ] )
         }
      },
      { path: '/Monitor/SDB-Overview/Domain',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Overview/Domain.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Overview/Domain.js' ] )
         }
      },
      { path: '/Monitor/SDB-Overview/Resource',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Overview/Resource.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Overview/Resource.js' ] )
         }
      },
      { path: '/Monitor/SDB-Group/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Group/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Group/Index.js' ] )
         }
      },
      { path: '/Monitor/SDB-Group/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Group/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Group/Charts.js' ] )
         }
      },
      { path: '/Monitor/SDB-Node/Index',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Index.js' ] )
         }
      },
      { path: '/Monitor/SDB-Node/Session',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Session.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Session.js' ] )
         }
      },
      { path: '/Monitor/SDB-Node/Resource',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Resource.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Resource.js' ] )
         }
      },
      { path: '/Monitor/SDB-Node/Charts',
        options: {
           templateUrl: './app/template/Monitor/Sdb/Node/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Sdb/Node/Charts.js' ] )
         }
      },
      //SQL监控页
      { path: '/Monitor/SQL-Module/Index',
        options: {
           templateUrl: './app/template/Monitor/SqlModule/Preview/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/SqlModule/Preview/Index.js' ] )
         }
      },
      { path: '/Monitor/SQL-Module/Performance',
        options: {
           templateUrl: './app/template/Monitor/SqlModule/Performance/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/SqlModule/Performance/Index.js' ] )
         }
      },
      { path: '/Monitor/SQL-Module/Analyze',
        options: {
           templateUrl: './app/template/Monitor/SqlModule/Analyze/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/SqlModule/Analyze/Index.js' ] )
         }
      },
      //监控-主机列表
      { path: '/Monitor/Host-List/Index',
        options: {
           templateUrl: './app/template/Monitor/Host/List/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/List/Index.js' ] )
         }
      },
      { path: '/Monitor/Host-List/Charts',
        options: {
           templateUrl: './app/template/Monitor/Host/List/Charts.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/List/Charts.js' ] )
         }
      },
      //监控-单主机预览
      { path: '/Monitor/Host-Info/Index',
        options: {
           templateUrl: './app/template/Monitor/Host/Info/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/Info/Index.js' ] )
         }
      },
      //监控-单主机磁盘
      { path: '/Monitor/Host-Info/Disk',
        options: {
           templateUrl: './app/template/Monitor/Host/Info/Disk.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/Info/Disk.js' ] )
         }
      },
      //监控-单主机CPU
      { path: '/Monitor/Host-Info/CPU',
        options: {
           templateUrl: './app/template/Monitor/Host/Info/CPU.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/Info/CPU.js' ] )
         }
      },
      //监控-单主机内存
      { path: '/Monitor/Host-Info/Memory',
        options: {
           templateUrl: './app/template/Monitor/Host/Info/Memory.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/Info/Memory.js' ] )
         }
      },
      //监控-单主机网络
      { path: '/Monitor/Host-Info/Network',
        options: {
           templateUrl: './app/template/Monitor/Host/Info/Network.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/Info/Network.js' ] )
         }
      },
      //监控-主机告警
      { path: '/Monitor/Host/Warning',
        options: {
           templateUrl: './app/template/Monitor/Host/Warning/Index.html',
           resolve: resolveFun( [ './app/controller/Monitor/Host/Warning/Index.js' ] )
         }
      },
      //监控-社区版提示页面
      { path: '/Monitor/Preview',
        options: {
           templateUrl: './app/template/Monitor/other/Preview.html',
           resolve: resolveFun( [ './app/controller/Monitor/other/Preview.js' ] )
         }
      },
      //部署
      { path: '/Deploy/Index',
        options: {
           templateUrl: './app/template/Deploy/Index.html',
           resolve: resolveFun( [ './app/controller/Deploy/Index.js' ] )
        }
      },
      //部署主机
      { path: '/Deploy/ScanHost',
        options: {
           templateUrl: './app/template/Deploy/InstallHost/Scan.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallHost/Scan.js' ] )
        }
      },
      { path: '/Deploy/AddHost',
        options: {
           templateUrl: './app/template/Deploy/InstallHost/Add.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallHost/Add.js' ] )
        }
      },
      //部署业务
      { path: '/Deploy/SDB-Conf',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Sdb/Conf.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Sdb/Conf.js' ] )
        }
      },
      { path: '/Deploy/SDB-Mod',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Sdb/Mod.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Sdb/Mod.js' ] )
        }
      },
      { path: '/Deploy/InstallHost',
        options: {
           templateUrl: './app/template/Deploy/Task/Host.html',
           resolve: resolveFun( [ './app/controller/Deploy/Task/Host.js' ] )
        }
      },
      { path: '/Deploy/InstallModule',
        options: {
           templateUrl: './app/template/Deploy/Task/Module.html',
           resolve: resolveFun( [ './app/controller/Deploy/Task/Module.js' ] )
        }
      },
      { path: '/Deploy/ZKP-Mod',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Zookeeper/Mod.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Zookeeper/Mod.js' ] )
        }
      },
      { path: '/Deploy/SSQL-Conf',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Ssql/Conf.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Ssql/Conf.js' ] )
        }
      },
      { path: '/Deploy/SSQL-Mod',
        options: {
           templateUrl: './app/template/Deploy/InstallModule/Ssql/Mod.html',
           resolve: resolveFun( [ './app/controller/Deploy/InstallModule/Ssql/Mod.js' ] )
        }
      }
   ] ;
   window.SdbSacManagerConf.defaultRoute = { redirectTo: '/Transfer' } ;
}());