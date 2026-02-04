// Deploy a 3 group 1 node cluster
// | --------- | ------------- | ------------------ |
// | Node      | HostName      | ServiceName        |
// | --------- | ------------- | ------------------ |
// | coord     | DSHOSTNAME    | DSSVCNAME          |
// | tmp coord | DSHOSTNAME    | DSSVCNAME + 10     |
// | catalog   | first host    | DSSVCNAME + 20     |
// | group1    | second host   | DSSVCNAME + 30     |
// | group2    | third host    | DSSVCNAME + 40     |
// | group3    | fourth host   | DSSVCNAME + 50     |
// | --------- | ------------- | ------------------ |
// Default:
// DSHOSTNAME = COORDHOSTNAME
// DSSVCNAME = RSRVPORTBEGIN
function deployDSCluster()
{
   if ( RSRVPORTBEGIN > DSSVCNAME ||
        DSSVCNAME > RSRVPORTEND )
   {
      // user deployed cluster
      return;
   }

   // get host list
   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var hostSet = {};
   var cursor = db1.list( SDB_LIST_GROUPS, {}, { "Group.HostName": null } );
   while ( rec = cursor.next() )
   {
      var obj = rec.toObj();
      for ( var i = 0; i < obj.Group.length; i++ )
      {
         hostSet[ obj.Group[i].HostName ] = 1;
      }
   }
   var hostArray = [];
   for ( var key in hostSet )
   {
      hostArray.push( key );
   }
   hostArray.sort();
   var hostPoller = new HostPoller( hostArray );
   db1.close();

   // deploy
   var svc = parseInt( DSSVCNAME );
   var coordHost = DSHOSTNAME;
   var coordSvc = svc;
   svc += 10;

   var tmpCoordHost = DSHOSTNAME;
   var tmpCoordSvc = svc;
   svc += 10;
   createTmpCoord( tmpCoordHost, tmpCoordSvc ) ;

   var catalogHost = hostPoller.get();
   var catalogSvc = svc;
   svc += 10;
   createCatalog( catalogHost, catalogSvc, tmpCoordHost, tmpCoordSvc ) ;

   createCoord( coordHost, coordSvc, tmpCoordHost, tmpCoordSvc ) ;

   var group1Host = hostPoller.get();
   var group1Svc = svc;
   svc += 10;
   createData( "group1", group1Host, group1Svc, tmpCoordHost, tmpCoordSvc ) ;

   var group2Host = hostPoller.get();
   var group2Svc = svc;
   svc += 10;
   createData( "group2", group2Host, group2Svc, tmpCoordHost, tmpCoordSvc ) ;

   var group3Host= hostPoller.get();
   var group3Svc = svc;
   createData( "group3", group3Host, group3Svc, tmpCoordHost, tmpCoordSvc ) ;

   // setup database
   var db3 = new Sdb( coordHost, coordSvc ) ;
   db3.getRecycleBin().disable();
   db3.close();

   removeTmpCoord( tmpCoordHost, tmpCoordSvc ) ;
}

function HostPoller( hostArray )
{
   this.hostIt = 0;
   this.hostArray = hostArray;
   this.get = function()
   {
      if (this.hostIt >= this.hostArray.length)
      {
         this.hostIt = 0;
      }
      return this.hostArray[this.hostIt++];
   }
}

function createTmpCoord( hostName, tmpCoordSvc )
{
   var oma = new Oma( hostName, CMSVCNAME );

   try
   {
      oma.createCoord( tmpCoordSvc, RSRVNODEDIR + "/coord/" + tmpCoordSvc );
   }
   catch( e )
   {
      if ( e != SDBCM_NODE_EXISTED )  // ignore error
      {
         println( "Unexpected error[" + e + "] when creating temp coord: " +
                  "localhost:" + tmpCoordSvc + "!" ) ;
         throw e ;
      }
   }

   oma.startNode( tmpCoordSvc ) ;
}

function createCatalog( catalogHost, catalogSvc, tmpCoordHost, tmpCoordSvc )
{
   var db2 = new Sdb( tmpCoordHost, tmpCoordSvc ) ;

   try
   {
      db2.createCataRG( catalogHost, catalogSvc,
                        RSRVNODEDIR + "/catalog/" + catalogSvc ) ;
   }
   catch( e )
   {
      if ( e == SDBCM_NODE_EXISTED || e == SDB_COORD_RECREATE_CATALOG ) // ignore error
      {
         addCataAddr2TmpCoord( catalogHost, catalogSvc,
                               tmpCoordHost, tmpCoordSvc ) ;
      }
      else
      {
         println( "Unexpected error[" + e + "] when creating catalog " +
                  "node: " + catalogHost + ":" + catalogSvc + "!" ) ;
         throw e ;
      }
   }
}

function addCataAddr2TmpCoord( catalogHost, catalogSvc, tmpCoordHost, tmpCoordSvc )
{
   var oma = new Oma( tmpCoordHost, CMSVCNAME ) ;

   var cataSvc = parseInt( catalogSvc ) + 3 ;
   var cataAddrSetting = catalogHost + ":" + cataSvc ;
   oma.updateNodeConfigs( tmpCoordSvc, { catalogaddr: cataAddrSetting } ) ;

   oma.stopNode( tmpCoordSvc ) ;
   oma.startNode( tmpCoordSvc ) ;
}

function createCoord( coordHost, coordSvc, tmpCoordHost, tmpCoordSvc )
{
   var db2 = new Sdb( tmpCoordHost, tmpCoordSvc ) ;

   try
   {
      db2.createCoordRG() ;
   }
   catch( e )
   {
      if ( e != SDB_CAT_GRP_EXIST ) // ignore error
      {
         println( "Unexpected error[" + e + "] when creating coord group!" ) ;
         throw e ;
      }
   }

   try
   {
      var rg = db2.getCoordRG() ;
      rg.createNode( coordHost, coordSvc, RSRVNODEDIR + "/coord/" + coordSvc ) ;
   }
   catch( e )
   {
      if ( e != SDBCM_NODE_EXISTED ) // ignore error
      {
         println( "Unexpected error[" + e + "] when creating coord node!" ) ;
         throw e ;
      }
   }

   try
   {
      rg.start() ;
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when starting coord group!" ) ;
      throw e ;
   }
}

function createData( groupName, groupHost, groupSvc, tmpCoordHost, tmpCoordSvc )
{
   var db2 = new Sdb( tmpCoordHost, tmpCoordSvc ) ;
   var rg = null ;

   try
   {
      rg = db2.createRG( groupName ) ;
   }
   catch( e )
   {
      if ( e != SDB_CAT_GRP_EXIST )
      {
         println( "Unexpected error[" + e + "] when get data group[" +
                  groupName + "]!" ) ;
         throw e ;
      }
      else
      {
         rg = db2.getRG( groupName ) ;
      }
   }

   try
   {
      rg.createNode( groupHost, groupSvc, RSRVNODEDIR + "/data/" + groupSvc ) ;
   }
   catch( e )
   {
      if ( e != SDBCM_NODE_EXISTED )
      {
         println( "Unexpected error[" + e + "] when creating data node[" +
                  groupHost + ":" + groupSvc + "]!" ) ;
         throw e ;
      }
   }

   try
   {
      rg.start() ;
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when starting data group[" +
               groupName + "]!" ) ;
      throw e ;
   }
}

function removeTmpCoord( tmpCoordHost, tmpCoordSvc )
{
   var oma = new Oma( tmpCoordHost, CMSVCNAME ) ;

   try
   {
      oma.removeCoord( tmpCoordSvc ) ;
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when removing temp coord[" +
               tmpCoordHost + ":" + tmpCoordSvc + "]!" ) ;
      throw e ;
   }
}

deployDSCluster();
