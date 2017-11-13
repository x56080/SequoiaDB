/****************************************************************
@decription:   deploy for performance test
               excute cmd: bin/sdb -f 'conf/deploy_conf_TPCC.js,install_deploy/deploy_tpcc.js' 
                           -e 'var hostList=["ci-test-pm1","ci-test-pm2","ci-test-pm3"];'
@input:        hostList: e.g.['host1','host2','host3'], G1D3 and G3D3 need 3 hosts
@author:       Ting YU 2017-02-06   
****************************************************************/
if( typeof(hostList) === "undefined" ) 
{ 
   throw "invalid para: hostList, can not be null"; 
}
if( hostList.constructor !== Array ) 
{ 
   throw "invalid para: hostList, should be array"; 
}

var hostNum = hostList.length;
var tmpCoordHost = hostList[0];

main();

function main()
{
   if( mode === "standalone" )
   {
      deployStandalone();
   }
   else
   {
      deployCluster();
   }
}

function deployCluster()
{
   println("------deploy mode: H" + hostNum + "G" + datagroupNum + "D" + replSize );
   var db = createTmpCoord();
   createCata( db );
   createCoord( db );
   createData( db );
   clean( db );
   println("------succed to deploy");
}

function deployStandalone()
{
   println("------deploy mode: STANDALONE");
   
   for( var i in hostList )
   {
      var host = hostList[i]; 
      println( "-----begin to create node in " + host );
      
      var oma = new Oma( host, cmPort );
      
      var serivce = 11810;
      var dbPath = diskList[0] + "/database/standalone/" + serivce;
      var config = nodeConf;
      oma.createData( serivce, dbPath, config );
                      
      oma.startNode( serivce );     
   }
   
   println("------succed to deploy");
}

function createTmpCoord()
{
   println( "-----begin to create and link temp coord" );

   var oma = new Oma( tmpCoordHost, cmPort );
   
   var dbBasePath = diskList[0];   
   oma.createCoord( tmpCoordPort, dbBasePath + "/database/coord/" + tmpCoordPort );
   oma.startNode( tmpCoordPort );
   
   var db = new Sdb( tmpCoordHost, tmpCoordPort );
   
   return db;
}

function createCata( db )
{
   println("-----begin to create cata group");
   var cataBasePort = 11800;
   
   //create first catalog node
   var host = hostList[0];
   var service = cataBasePort;
   var dbPath = diskList[0] + "/database/cata/" + service;
   var config = updateDeployConfig( cataConf, service );
   var rg = db.createCataRG( host, service, dbPath, config );
   
   //wait for cata group to select primary node
   for(var i = 0; i < 600; i++ )   
   {  
      try
      {
         sleep(100); 
         var rg = db.getRG("SYSCatalogGroup"); 
         break;       
      } 
      catch(e)
      {
         if( e !== -71 ) throw e;         
      }   
   }
   
   //create other catalog nodes
   var i = 0;
   while( i < cataNum )
   {  
      if( i === 0 )  
      {
         i++;
         continue;      //first cata node has been already created
      }
         
      var host = hostList[ i % hostNum ];
      var service = cataBasePort + parseInt( i / hostNum ) * 20;
      var dbPath = diskList[0] + "/database/cata/" + service;
      var config = updateDeployConfig( cataConf, service );
      rg.createNode( host, service, dbPath, config );
      
      i++;
   }
   
   //start other nodes
   var i = 0;
   while( i < cataNum )
   {  
      if( i === 0 )  
      {
         i++;
         continue;      //first cata node has been already started
      }
         
      var host = hostList[ i % hostNum ];
      var service = cataBasePort + parseInt( i / hostNum ) * 20;
      rg.getNode( host, service ).start();

      i++;
   }
   checkeCataPrimary( db, "SYSCatalogGroup" );
}

function createCoord( db )
{
   println("-----begin to create coord group");
   var coordBasePort = 11810;
   var dbBasePath = diskList[0];
   
   var rg = db.createCoordRG();
   
   for( var i in hostList )
   {
      for( var j = 0; j < coordnumPerhost; j++ )
      {
         var host = hostList[i];
         var service = coordBasePort + j * 20;
         var dbPath = dbBasePath + "/database/coord/" + service;
         var config = updateDeployConfig( coordConf, service );
         rg.createNode( host, service, dbPath, config );
      }
      
   }
   
   rg.start();  
}

function createData( db )
{
   var dataBasePort = 20000;
   
   for( var n = 0; n < datagroupNum; n++ )
   {
      //create group
      var datargName = "group" + ( n + 1 );
      println( "-----begin to create data group: " + datargName );
      var rg = db.createRG( datargName );
      
      //create node
      var dataRgBasePort = dataBasePort + ( n + 1 ) * 100;
      if( n === 0 ) 
      {
         var randomHostList = hostList;
      }
      else
      {
         var randomHostList = randomArray( randomHostList );
      }
      
      var i = 0;
      while( i < replSize )
      {   
         var host = randomHostList[ i % hostNum ];
         var service = dataRgBasePort + parseInt( i / hostNum ) * 10;
         if( diskList.length === 1 )
         {
            var dbPath = diskList[0] + "/database/data/" + service;
         }
         else
         {
            var dbPath = diskList[ n + 1 ] + "/database/data/" + service;
         }
         var config = updateDeployConfig( dataConf, service );
         rg.createNode( host, service, dbPath, config );

         i++;
      }
   
      //start node
      rg.start();
      checkeDataPrimary( db, datargName );
   }
}

function updateDeployConfig( conf, service ) 
{
   var config = JSON.stringify(conf).replace( "[svcname]", service );
   return JSON.parse(config);
}

function checkeCataPrimary( db, rgname )
{
   var hasPrimary = false;                                 
   for(var i = 0; i < 5*600; i++ )  //wait for cata group to select primary node 
   {  
      try
      {
         sleep(100); 
         var cataRG = db.getRG("SYSCatalogGroup"); 
         hasPrimary = true;
         break;       
      } 
      catch(e)
      {
         if( e !== -71 ) 
         {
            println("excute: db.getRG('SYSCatalogGroup')");
            throw e;
         }            
      }   
   }
   if( hasPrimary === false )
   {
      throw "fail to select primary node after 5 minute";
   }    
}

function checkeDataPrimary( db, rgname )
{
   var hasPrimary = false;
   for(var i = 0; i < 5*600; i++ )  //wait for data group to select primary node 
   {  
      try
      {
         sleep(100); 
         db.getRG(rgname).getMaster(); 
         hasPrimary = true;
         break;       
      } 
      catch(e)
      {
         if( e !== -71 ) 
         {
            println("excute: db.getRG(" + rgname + ").getMaster()");
            throw e;  
         }          
      }   
   }
   if( hasPrimary === false )
   {
      throw "fail to select primary node after 5 minute";
   }
}

function randomArray( arr ) // [1, 2, 3]--> [2, 3, 1]
{
   var firEle = arr.shift();
   arr.push( firEle );
   return arr;
}

function clean( db )
{
   println( "-----begin to remove temp coord" );
   var oma = new Oma( tmpCoordHost, cmPort );
   oma.removeCoord( tmpCoordPort );
}
