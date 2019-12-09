/************************************
*@Description: put 2m lob
               
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

      var clName = COMMCLNAME + "_testputlob10";
      var mgr = new groupMgr( db );
      mgr.init();

      var nodeNum = 2;
      var group = selectGroupByNodeNum( mgr, nodeNum );

      var cl = new collection( COMMCSNAME, clName, w.ONE );
      cl.drop( db );
      cl.create( db, group.name );

      var lob = new testFile( "/tmp", "2m" );
      var lobSize = 2 * 1024 * 1024;
      lob.generator( lobSize );

      cl.putLob( "/tmp/2m" );

      assert( group.checkConsistency( cl ), "data is not consistency" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( undefined !== lob )
      {
         lob.delete();
      }

      if( undefined !== db )
      {
         db.close();
      }
   }
}

main();

