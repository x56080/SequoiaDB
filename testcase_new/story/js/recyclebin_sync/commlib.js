/******************************************************************************
 * @Description   :
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2024.07.01
 * @LastEditors   : fangjiabin
 ******************************************************************************/
import( "../lib/recyclebin_commlib.js" );
import( "../lib/lobSubCL_commlib.js" );
import( "../lib/basic_operation/commlib.js" );
testConf.testGroups = ["recycleBin"];
testConf.skipStandAlone = true;

function checkRecycleRecover( dbcs, dbcl, csName, clName, opName )
{
   commCheckLSN( db );
   var expCursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, RawData: true },
      { "Details.UpdateTime": { "$include": 0 },
        "Details.DataCommitLSN": { "$include": 0 } },
      { "Details.NodeName": 1 } );

   if ( "Drop" == opName )
   {
      dbcs.dropCL( clName );
   }
   else if ( "Truncate" == opName )
   {
      dbcl.truncate();
   }
   else
   {
      throw error( "Invalid op name " + opName );
   }

   var recycleName = getOneRecycleName( db, csName + "." + clName, opName );
   db.getRecycleBin().returnItem( recycleName );

   commCheckLSN( db );
   var actCursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, RawData: true },
      { "Details.UpdateTime": { "$include": 0 },
        "Details.DataCommitLSN": { "$include": 0 } },
      { "Details.NodeName": 1 } );

   if ( actCursor.size() != expCursor.size() )
   {
      throw error( "Invalid snapshot cl result size[ act: " + actCursor.size() +
                   ", exp: " + expCursor.size() + " ]" );
   }

   while( actCursor.next() && expCursor.next() )
   {
      assert.equal( actCursor.current().toObj(), expCursor.current().toObj() );
   }
}