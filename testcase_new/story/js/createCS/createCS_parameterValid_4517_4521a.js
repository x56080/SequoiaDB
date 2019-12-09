/****************************************************
@description: seqDB-4517:createCS��createCS��options:PageSize is ""
seqDB-4521:createCS��createCS��options:LobPageSize is ""
@author:
2019-6-4 wuyan init
****************************************************/
main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      var csName = "cs4517a";
      commDropCS( db, csName, true, "clear cs in the beginning." );

      println( "---Begin to test testcase-4517/4521, with pageSize and lobPageSize is ''. " );
      var pageSize = "";
      var lobPageSize = "";
      var options = { LobPageSize: lobPageSize, PageSize: pageSize };
      db.createCS( csName, options );

      //check the options
      var cursor = db.snapshot( 5, { Name: csName } );
      var actPageSize = 0;
      var actLobPageSize = 0;
      while( cursor.next() )
      {
         var curInfo = cursor.current();
         actPageSize = curInfo.toObj().PageSize;
         actLobPageSize = curInfo.toObj().LobPageSize;
         println( "---snapshotInfo: \n" + curInfo );
      }

      //�մ�ȡĬ��ֵ��"LobPageSize": 262144��"PageSize": 65536
      var expPageSize = 65536;
      var expLobPageSize = 262144;
      if( Number( expPageSize ) !== Number( actPageSize ) || Number( expLobPageSize ) !== Number( actLobPageSize ) )
      {
         throw buildException( "check option", "", "", "", "actPageSize:" + actPageSize + " actLobPageSize:" + actLobPageSize );
      }
   }
}

