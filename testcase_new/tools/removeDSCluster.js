function removeDSCluster()
{
   var db1 = new Sdb( DSHOSTNAME, DSSVCNAME );

   // remove data
   var cursor = db1.list( SDB_LIST_COLLECTIONSPACES );
   while (rec = cursor.next())
   {
      var obj = rec.toObj();
      db1.dropCS( obj.Name );
   }

   // clean recycle bin
   db1.getRecycleBin().dropAll();

   // remove data group
   var db1 = new Sdb( DSHOSTNAME, DSSVCNAME );
   var cursor = db1.list( SDB_LIST_GROUPS );
   while (rec = cursor.next())
   {
      var obj = rec.toObj();
      if ( obj.GroupName != "SYSCoord" &&
           obj.GroupName != "SYSCatalogGroup" )
      {
         db1.removeRG( obj.GroupName );
      }
   }
   var catalogNode = db1.getCataRG().getMaster();
   var catalogHost = catalogNode.getHostName();
   var catalogSvc = parseInt( catalogNode.getServiceName() ) + 3;
   var catalogAddr = catalogHost + ":" + catalogSvc;
   db1.close();

   // get an available temp port
   var tmpCoordSvc = 36511;
   var remote = new Remote( COORDHOSTNAME, 11790 );
   var maxStep = 200;
   while( maxStep-- > 0 ) {
      if ( remote.getSystem().sniffPort( tmpCoordSvc ).toObj().Usable ) {
         break;
      }
      tmpCoordSvc++;
   }
   remote.close();

   // remove coord and catalog
   var oma = new Oma( COORDHOSTNAME, 11790 );
   try
   {
      oma.createCoord( tmpCoordSvc, RSRVNODEDIR + "/coord/" + tmpCoordSvc,
                       { catalogaddr: catalogAddr } );
   }
   catch (e)
   {
      if ( e != SDBCM_NODE_EXISTED )
      {
         throw e;
      }
   }
   oma.startNode( tmpCoordSvc );
   var db2 = new Sdb( COORDHOSTNAME, tmpCoordSvc );
   db2.removeCoordRG();
   db2.removeCataRG();

   oma.removeCoord( tmpCoordSvc ) ;
   oma.close();
}
removeDSCluster();