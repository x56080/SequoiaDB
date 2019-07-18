/******************************************************************************
*@Description : seqDB-18673:节点间index一致性检查（index、LSN） 
*@Author      : 2019-07-13  XiaoNi Huang init
******************************************************************************/
main();

function main()
{  
   if ( commIsStandalone( db ) )
   {
      println("The mode is standalone.");
      return ;
   }  
      
   var csName = "cs18673";
   var clName = "cl";
   
   println("\n---Begin to ready cs / cl.");
   commDropCS( db, csName, true, "Failed to drop cs in the pre-condition." ); 
   var cs = db.createCS( csName );
   var cl = cs.createCL( clName, {ReplSize:0} );
   
   println("\n---Begin to alter cl to sharding.");
   cl.alter({ShardingType:"hash", ShardingKey:{a:1}});
   cl.insert({a:1});
   
   println("\n---Begin to cl.truncate.");
   cl.truncate();
   
   println("\n---Begin to db.sync.");
   db.sync({CollectionSpace: csName });
   
   println("\n---Begin to check results.");
   var fullCLName = csName + "." + clName;
   var groupName = commGetCLGroups( db, fullCLName )[0];
   println("   Begin to get cl group = " + groupName + "." );
   var rg  = db.getRG( groupName );
   var sDB;
   var mDB;
   try 
   {
      sDB = rg.getSlave().connect();
      mDB = rg.getMaster().connect(); 
      
      checkIndex( sDB, mDB, csName, clName );
      checkIndexLSN( sDB, mDB, fullCLName );
   } 
   finally
   {
      sDB.close();
      mDB.close();
   }
   
   commDropCS( db, csName, false, "Failed to drop cs in the end-condition." ); 
}

function checkIndex(sDB, mDB, csName, clName)
{
   println("   Begin to check lob lsn." ); 
   var sCL = sDB.getCS( csName ).getCL( clName );
   var mCL = mDB.getCS( csName ).getCL( clName ); 
   var sCursor = sCL.listIndexes(); 
   var mCursor = mCL.listIndexes(); 
   var sIdxNum = 0;
   var mIdxNum = 0;
   while ( sCursor.next() )
   {
      sIdxNum++;
   }
      
   while ( mCursor.next() )
   {
      mIdxNum++;
   }
   
   var expIdxNum = 2; //index: $id, $shard
   if ( expIdxNum !== mIdxNum || sIdxNum !== mIdxNum )
   {      
      throw buildException( "main", null, "[check index]", 
               "[expIdxNum = " + expIdxNum + "]", 
               "[mIdxNum = " + mIdxNum + ", sIdxNum = " + sIdxNum + "]" );
   }
   
}

function checkIndexLSN(sDB, mDB, fullCLName)
{   
   println("   Begin to check lob lsn." ); 
   var sLSN = sDB.snapshot ( SDB_SNAP_COLLECTIONS, {Name: fullCLName }).current().toObj().Details[0].IndexCommitLSN; 
   var mLSN = mDB.snapshot ( SDB_SNAP_COLLECTIONS, {Name: fullCLName }).current().toObj().Details[0].IndexCommitLSN; 
   if ( sLSN !== mLSN )
   {      
      throw buildException( "main", null, "[check lob lsn]", 
               "[sLSN = " + sLSN + "]", 
               "[mLSN = " + mLSN + "]" );
   }
}