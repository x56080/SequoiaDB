/************************************
*@Description: insert doc that is size large than 64k 
               strong data consistency
*@author:     wangwenjing
**************************************/
function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( commIsStandalone( db ) )
      {
         return;
      }

      var clName = COMMCLNAME + "_testinsdoc7";
      var mgr = new groupMgr( db );
      mgr.init();

      var nodeNum = 2;
      var group = selectGroupByNodeNum( mgr, nodeNum );

      var cl = new collection( COMMCSNAME, clName, w.ONE );
      cl.drop( db );
      cl.create( db, group.name );
      var pageSize = 64 * 1024;
      cl.insert( 3 * pageSize );
      assert( group.checkConsistency( cl ), "data is not consistency" );
      assert( group.checkResult( true, group.checkDoc, cl, { id: 1 } ),
         "data is not consistency" );

   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( undefined !== db )
      {
         db.close();
      }
   }
}

main();
