/******************************************************************************
 * @Description   : seqDB-29862:使用listReplicaGroups查看节点instanceid
 * @Author        : liuli
 * @CreateTime    : 2023.01.10
 * @LastEditTime  : 2023.01.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var cata = db.getCataRG().getSlave();
   var cataNodeID = cata.getDetailObj().NodeID;
   try
   {
      var instanceid = 16;
      try
      {
         db.updateConf( { instanceid: instanceid }, { NodeID: cataNodeID } );
      } catch( e )
      {
         if( e != SDB_RTN_CONF_NOT_TAKE_EFFECT && e != SDB_COORD_NOT_ALL_DONE )
         {
            throw new Error( e );
         }
      }
      cata.stop();
      cata.start();
      commCheckBusinessStatus( db );

      var cursor = db.listReplicaGroups();
      while( cursor.next() )
      {
         var obj = cursor.current().toObj();
         if( obj.GroupName == CATALOG_GROUPNAME )
         {
            if( obj["Group"][0]["NodeID"] == cataNodeID )
            {
               assert.equal( obj["Group"][0]["NodeID"], instanceid, "detailed information : " + JSON.stringify( obj ) );
            }
         }
      }
      cursor.next();

   }
   finally
   {
      try
      {
         db.deleteConf( { instanceid: 1 }, { NodeID: cataNodeID } );
      } catch( e )
      {
         if( e != SDB_RTN_CONF_NOT_TAKE_EFFECT && e != SDB_COORD_NOT_ALL_DONE )
         {
            throw new Error( e );
         }
      }
      cata.stop();
      cata.start();
      commCheckBusinessStatus( db );
   }
}