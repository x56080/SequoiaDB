/******************************************************************************
* @Description   : seqDB-29397:相同线程QueryID递增，不同线程QueryID不同
* @Author        : Cheng Jingjing
* @CreateTime    : 2022.12.21
* @LastEditTime  : 2022.12.21
* @LastEditors   : Cheng Jingjing
******************************************************************************/
testConf.clName = COMMCLNAME + "_csName_29397"
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test );
function test( testPara )
{
   try
   {
      // updateconf
      db.updateConf( { mongroupmask: "all:basic", monslowquerythreshold: 0 } );

      //insert
      insertRecs( testPara.testCL );

      // SDB_SNAP_QUERY
      var ret = db.snapshot( SDB_SNAP_QUERIES );
      var queryID = ret.current().toObj()["QueryID"];

      queryID = getNewQueryID( queryID );
      ret = db.snapshot( SDB_SNAP_QUERIES, { QueryID: queryID } );
      if ( !ret.next() )
      {
         throw new Error( "Invalid QueryID in query snapshot. It must be " + queryID );
      }
      // SDB_SNAP_CONTEXTS
      queryID = getNewQueryID( queryID );
      ret = db.snapshot( SDB_SNAP_CONTEXTS, { "$and": [{ "Contexts.QueryID": queryID }, { "Contexts.Type": "COORD" }] } );
      if ( !ret.next() )
      {
         throw new Error( "Invalid QueryID in contexts snapshot. It must be " + queryID );
      }

      // change to a new connection
      var oldQueryIDPrefixStr = queryID.substr( 0, 18 );

      // catalog
      var catalog = db.getCataRG().getMaster().connect();
      ret = catalog.snapshot( SDB_SNAP_QUERIES );
      queryID = ret.current().toObj()["QueryID"];
      newQueryIDPrefixStr = queryID.substr( 0, 18 );
      assert.notEqual( newQueryIDPrefixStr, oldQueryIDPrefixStr );
      catalog.close();

      // data
      var data = db.getRG( testPara.srcGroupName ).getMaster().connect();
      ret = data.snapshot( SDB_SNAP_QUERIES );
      queryID = ret.current().toObj()["QueryID"];
      var newQueryIDPrefixStr = queryID.substr( 0, 18 );
      assert.notEqual( newQueryIDPrefixStr, oldQueryIDPrefixStr );
      data.close();
   }
   finally
   {
      db.deleteConf( { mongroupmask: "all:basic", monslowquerythreshold: 0 } );
   }
}

function getNewQueryID( curQueryID )
{
   var queryIDPrefixStr = curQueryID.substr( 0, 18 );
   var sequence = parseInt( curQueryID.substr( 18, 8 ), 16);
   var newQueryID = queryIDPrefixStr + numToHexStr( ++sequence, 8 ) ;
   return newQueryID ;
}

function numToHexStr( num, totalStrlen )
{
   var numHexStr = num.toString(16);
   if ( totalStrlen < numHexStr.length )
   {
      throw new Error( "Invalid total str len" );
   }
   else if ( totalStrlen == numHexStr.length )
   {
      return numHexStr;
   }
   else
   {
      var ret = "";
      for ( var i = 0; i < totalStrlen - numHexStr.length; i++ )
      {
         ret += "0";
      }
      ret += numHexStr;
      return ret;
   }
}