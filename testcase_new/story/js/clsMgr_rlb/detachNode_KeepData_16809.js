/* *****************************************************************************
@discretion: detachNode( )中KeepData参数校验
@author：2018-12-12 wangkexin
***************************************************************************** */
testConf.skipOneGroup = true;
main( test );
function test ()
{
   try
   {
      var groupList = getGroup( db );
      var groupName = groupList[0];

      var hostname = db.getRG( groupName ).getDetail().next().toObj()["Group"][0]["HostName"];
      var port = parseInt( RSRVPORTBEGIN ) + 150;

      db.getRG( groupName ).createNode( hostname, port, RSRVNODEDIR + port );
      db.getRG( groupName ).start();
      //test a : KeepData设为合法值
      db.getRG( groupName ).detachNode( hostname, port, { KeepData: true } );
      db.getRG( groupName ).attachNode( hostname, port, { KeepData: true } );
      //test b : KeepData设为空值
      detachNodeLawfulness( groupName, hostname, port, "" );
      //test c : KeepData设为非布尔值
      detachNodeLawfulness( groupName, hostname, port, "test" );
   }
   catch( e )
   {
      throw new Error( "check detachNode16809" + e );
   } finally
   {
      db.getRG( groupName ).removeNode( hostname, port );
      if( db !== undefined )
      {
         db.close();
      }
   }
}
function detachNodeLawfulness ( groupName, hostname, port, KeepDataMsg )
{
   try
   {
      db.getRG( groupName ).detachNode( hostname, port, { KeepData: KeepDataMsg } );
      throw new Error( "exp fail but found success" );
   } catch( e )
   {
      if( e.message != -6 ) 
      {
         throw new Error( "detachNode with KeepData is " + KeepDataMsg + "fail" + e.message );
      }
   }
}
