/***************************************************************************
@Description : seqDB-15956:指定快照查询参数Mode为Run查询配置快照信息
@Modify list :
              2019-4-17  wangkexin  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var groups = commGetGroups( db );
   var hostName = groups[0][1].HostName;
   var svcname = groups[0][1].svcname;
   var nodeName = hostName + ":" + svcname;

   changeConf( nodeName );
   var nodeAddresses = [{ "hostName": hostName, "svcName": svcname }];
   stopNodes( nodeAddresses );
   startNodes( nodeAddresses )

   var expResult = [{ "transactionon": "FALSE" }];
   var option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "run", "expand": false } );
   checkResult( option, expResult );

   assert.tryThrow( -322, function()
   {
      db.deleteConf( { transactionon: 1 }, { 'NodeName': nodeName } );
   } );

   expResult = [{ "transactionon": "FALSE" }];
   option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "run", "expand": false } );
   checkResult( option, expResult );

   expResult = [{}];
   option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "local", "expand": false } );
   checkResult( option, expResult );
}

function changeConf ( nodeName )
{
   try
   {
      db.updateConf( { transactionon: false }, { NodeName: nodeName } );
   }
   catch( e )
   {
      if( e.message != -322 )
      {
         throw new Error( e );
      }
   }

}

function checkResult ( option, expResult )
{
   var actResult = [];
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, option );
   while( cursor.next() )
   {
      actResult.push( { "transactionon": cursor.current().toObj().transactionon } );
   }
   assert.equal( actResult.length, expResult.length );
   if( JSON.stringify( actResult ) !== JSON.stringify( expResult ) )
   {
      throw new Error( "expectResult is " + JSON.stringify( expResult ) + ", but actResult is " + JSON.stringify( actResult ) );
   }

}
