/**********************************************************
*@Description: the catalog group add node 
               
*@author:     wangwenjing
***********************************************************/

function createNodeOfCataGroup ( cataGroup )
{
   var hostName = getHostNameOfLocal();
   var port = allocPort();
   var dbPath = buildDeployPath( port, "cata" );

   println( hostName + ":" + port + " " + dbPath );
   var node = new replicaNode( hostName, port, cataGroup );
   node.setDbPath( dbPath );

   node.create();

   return node;
}

function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( commIsStandalone( db ) )
      {
         return;
      }

      var mgr = new groupMgr( db );
      mgr.init();
      var group = mgr.getGroupByName( CATALOG_GROUPNAME );
      var node = createNodeOfCataGroup( group );

      assert( group.checkResult( true, group.checkLSN ), "the LSN is not consistent" );
      assert( group.checkResult( true, group.checkCS ), "system collection space is not consistent" );
      assert( group.checkResult( true, group.checkCL ), "system collection is not consistent" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( undefined !== node )
      {
         node.drop();
      }

      if( undefined !== db )
      {
         db.close();
      }
   }
}

main();
