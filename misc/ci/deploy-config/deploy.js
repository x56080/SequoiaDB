/******************************************************
@decription:   deploy cluster in CI

@input:        osArch: amd64 | ppc
               mode: standalone | h3g1d3 | h3g3d3
               hostList: e.g.["host1","host2","host3"]
               installDir: install directory
               
@author:       Ting YU 2016-04-26   
******************************************************/
if ( typeof(osArch) === "undefined" ) { var osArch = "x86";}
if ( typeof(mode) === "undefined" ) 
{ 
   throw "invalid para: mode, can not be null"; 
}
if ( mode !== "standalone" && mode !== "h3g1d3" && mode !== "h3g3d3" ) 
{ 
   throw "invalid para: mode, should be standalone | h3g1d3 | h3g3d3"; 
}
if ( typeof(hostList) === "undefined" ) 
{ 
   throw "invalid para: hostList, can not be null"; 
}
if ( hostList.constructor !== Array ) 
{ 
   throw "invalid para: hostList, should be array"; 
}
if ( hostList.length !== 3 ) 
{ 
   throw "invalid para: hostList, should has 3 element"; 
}
if ( typeof(installDir) === "undefined" ) 
{ 
   throw "invalid para: installDir, can not be null"; 
}
var fapValue = "";
if ( osArch === "amd64" ) { fapValue = "fapmongo";}

var cmPort         = 11790;
var tmpCoordPort   = 18800;
var cataPort       = 11800;
var coordPort      = 11810;
var dataPort1      = 21100;
var dataPort2      = 22100;
var dataPort3      = 23100;
var standalonePort = 11810;
var databaseDir    = installDir + "/database";

switch( mode )
{
   case "standalone":
      deployStandalone();
      break;
           
   case "h3g1d3":
      deployClster( mode );
      break;
      
   case "h3g3d3":
      deployClster( mode );
      break;      
   
}

function deployStandalone()
{
   println("------deploy mode: standalone");
   
   for( var i in hostList )
   {
      var hostname = hostList[i]; 
      println( "-----begin to create node in " + hostname );
      
      //create node
      var oma = new Oma( hostname, cmPort );
      oma.createData( standalonePort, 
                      databaseDir+"/standalone/"+standalonePort,
                      {diaglevel:5, fap:fapValue} );
      oma.startNode( standalonePort );     
   }
   
   println("------succed to deploy");
}

function deployClster( mode )
{
   println("------deploy mode: " + mode );   
   switch( mode )
   {              
      case "h3g1d3":
         var datargNum = 1;
         break;         
      case "h3g3d3":
         var datargNum = 3;
         break;            
   }   
   var controlHost = hostList[0];
   
   //1 create tmp coord
   println( "-----begin to create temp coord" );
   var oma = new Oma( controlHost, cmPort );
   oma.createCoord( tmpCoordPort, databaseDir+"/coord/"+tmpCoordPort );
   oma.startNode( tmpCoordPort );
   
   println("-----begin to link temp coord");
   var db = new Sdb( controlHost, tmpCoordPort );
   
   //2 create cata group
   println("-----begin to create cata group");
   var config = { diaglevel:5,
                  sharingbreak:30000,
                  diagnum:30,
                  optimeout:60000,
                  fap:fapValue
                };
   db.createCataRG( controlHost, cataPort, 
                    databaseDir+"/cata/"+cataPort,
                    config  ); 
                                     
   for(var i = 0; i < 600; i++ )  //wait for cata group to select primary node 
   {  
      try
      {
         sleep(100); 
         var cataRG = db.getRG("SYSCatalogGroup"); 
         break;       
      } 
      catch(e)
      {
         if( e !== -71 ) throw e;         
      }   
   }                                              
   
   var node1 = cataRG.createNode( hostList[1], cataPort, 
                                  databaseDir+"/cata/"+cataPort,
                                  config );
   var node2 = cataRG.createNode( hostList[2], cataPort, 
                                  databaseDir+"/cata/"+cataPort,
                                  config );
   node1.start();
   node2.start();
   
   //3 create coord group
   println("-----begin to create coord group");
   var coordRG = db.createCoordRG();
   for( var i in hostList )
   {
      var config = {  diaglevel:5,                      
                      diagnum:30,
                      optimeout:60000,
                      fap:fapValue 
                   };
      coordRG.createNode( hostList[i], coordPort, 
                          databaseDir+"/coord/"+coordPort,
                          config );
   }
   coordRG.start();  
   
   //4 create data groups
   for( var n = 1; n <= datargNum; n++ )
   {
      var datargName = "group" + n;
      var dataPort = eval( "dataPort" + n );
      println( "-----begin to create data group: " + datargName );
      
      var dataRG = db.createRG( datargName );
      for( var i in hostList )
      {
         var config = { diaglevel:5,
                        sharingbreak:30000,
                        diagnum:30,
                        optimeout:60000,
                        fap:fapValue
                      };
         dataRG.createNode( hostList[i], dataPort, 
                            databaseDir+"/data/"+dataPort,
                            config );
      }
      dataRG.start();
   } 
   
   println("-----begin to remove temp coord");
   oma.removeCoord(18800); 
   
   println("------succed to deploy");
}
