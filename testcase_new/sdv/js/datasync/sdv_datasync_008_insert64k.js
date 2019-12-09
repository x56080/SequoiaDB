/************************************
*@Description: insert doc that is size 64k
               final whether the data is consistent 
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

      var clName = COMMCLNAME + "_testins64k8";
      var mgr = new groupMgr( db );
      mgr.init();

      var nodeNum = 2;
      var group = selectGroupByNodeNum( mgr, nodeNum );

      var cl = new collection( COMMCSNAME, clName, w.ALL );
      cl.drop( db );
      cl.create( db, group.name );
      var pageSize = 64 * 1024;
      cl.insert( pageSize );
      assert( group.checkConsistency( cl ), "data is not consistency" );
      assert( group.checkResult( false, group.checkDoc, cl, { id: 1 } ),
         "data is not consistency" )

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
