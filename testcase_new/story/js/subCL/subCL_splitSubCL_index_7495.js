/* *****************************************************************************
@Description: attach hashCL and insert-BoundData with index
@modify list:
   2014-07-30 pusheng Ding  Init
***************************************************************************** */

function test_range_attach_hash_index_BoundTest ()
{
   // Not Error, test mainCL'ShardingType is range and subCL's ShardingType is hash ,insert BoundData's result
   MainCL_Name = CHANGEDPREFIX + "year";
   subCl_Name = CHANGEDPREFIX + "month";
   try
   {
      commDropCL( db, COMMCSNAME, subCl_Name + "1", true, true,
         "clean sub collection" );
      commDropCL( db, COMMCSNAME, subCl_Name + "2", true, true,
         "clean sub collection" );
      commDropCL( db, COMMCSNAME, MainCL_Name, true, true,
         "clean main collection" );
   }
   catch( e )
   {
      println( "failed to drop main and sub cl, rc = " + e );
      throw e;
   }

   try
   {
      db.setSessionAttr( { PreferedInstance: "M" } );
      var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );
   }
   catch( e )
   {
      println( "failed to create cs, rc = " + e );
      throw e;
   }
   println( COMMCSNAME );

   try
   {
      var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1, b: -1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true } );
      println( "mainCL" );
      var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
      println( "subCL1" );
      var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
      println( "subCL2" );
      mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "1", { LowBound: { a: 0 }, UpBound: { a: 10 } } );
      println( "attach subCL1" );
      mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "2", { LowBound: { a: 10 }, UpBound: { a: 20 } } );
      println( "attach subCL2" );
   }
   catch( e )
   {
      throw e;
   }

   //insert records
   try
   {
      for( var i = 0; i < 20; ++i )
      {
         mainCL.insert( { a: i } );
      }
   }
   catch( e )
   {
      throw e;
   }

   try
   {
      var subCL = [];
      subCL.push( subCL1 );
      subCL.push( subCL2 );
      var numberOfsubCl = 2;
      for( var i = 0; i < numberOfsubCl; ++i )
      {
         var sourceDataGroupName = getSourceGroupName_alone( COMMCSNAME, subCl_Name + ( i + 1 ) );
         println( "sourceDataGroupName is : " + sourceDataGroupName );

         var desDataGroupName = getOtherDataGroups( sourceDataGroupName );
         println( "desDataGroupName is " + desDataGroupName );

         var Partition = getPartition( COMMCSNAME, subCl_Name + ( i + 1 ) );
         println( "Partition is : " + Partition );

         if( !subCL_split_hash( subCL[i], sourceDataGroupName, desDataGroupName, Partition ) )
         {
            println( "************SPLIT SUCCED***************" );
         }
      }
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " CreateIndex test 1: " );
   try
   {
      mainCL.createIndex( "aIndex", { a: -1, b: 1 } );
      println( "************mainCL.createIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " Query records with index key, and check results" );
   try
   {
      for( var i = 0; i < 20; ++i )
      {
         var actCnt = mainCL.find( { a: i } ).count();
         var expCnt = 1;
         if( expCnt !== parseInt( actCnt ) )
         {
            println( "Failed to check results. Except [i,cnt]: [" + i + "," + expCnt + "], actual [i,cnt]:[" + i + "," + parseInt( actCnt ) + "]" );
            throw "Error.";
         }
      }
   }
   catch( e )
   {
      throw e;
   }

   println( " Dropndex : " );
   try
   {
      mainCL.dropIndex( "aIndex" );
      println( "************mainCL.dropIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " CreateIndex test 2: " );
   try
   {
      mainCL.createIndex( "aIndex", { a: 1, b: 1 } );
      println( "************mainCL.createIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " Query records with index key, and check results" );
   try
   {
      for( var i = 0; i < 20; ++i )
      {
         var actCnt = mainCL.find( { a: i } ).count();
         var expCnt = 1;
         if( expCnt !== parseInt( actCnt ) )
         {
            println( "Failed to check results. Except [i,cnt]: [" + i + "," + expCnt + "], actual [i,cnt]:[" + i + "," + parseInt( actCnt ) + "]" );
            throw "Error.";
         }
      }
   }
   catch( e )
   {
      throw e;
   }

   println( " Dropndex : " );
   try
   {
      mainCL.dropIndex( "aIndex" );
      println( "************mainCL.dropIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " CreateIndex test 3: " );
   try
   {
      mainCL.createIndex( "aIndex", { a: 1, b: -1 } );
      println( "************mainCL.createIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " Query records with index key, and check results" );
   try
   {
      for( var i = 0; i < 20; ++i )
      {
         actCnt = mainCL.find( { a: i } ).count();
         var expCnt = 1;
         if( expCnt !== parseInt( actCnt ) )
         {
            println( "Failed to check results. Except [i,cnt]: [" + i + "," + expCnt + "], actual [i,cnt]:[" + i + "," + parseInt( actCnt ) + "]" );
            throw "Error.";
         }
      }
   }
   catch( e )
   {
      throw e;
   }

   println( " Dropndex : " );
   try
   {
      mainCL.dropIndex( "aIndex" );
      println( "************mainCL.dropIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " CreateIndex test 4: " );
   try
   {
      mainCL.createIndex( "aIndex", { a: -1, b: -1 } );
      println( "************mainCL.createIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }

   println( " Query records with index key, and check results" );
   try
   {
      for( var i = 0; i < 20; ++i )
      {
         actCnt = mainCL.find( { a: i } ).count();
         var expCnt = 1;
         if( expCnt !== parseInt( actCnt ) )
         {
            println( "Failed to check results. Except [i,cnt]: [" + i + "," + expCnt + "], actual [i,cnt]:[" + i + "," + parseInt( actCnt ) + "]" );
            throw "Error.";
         }
      }
   }
   catch( e )
   {
      throw e;
   }

   println( " Dropndex : " );
   try
   {
      mainCL.dropIndex( "aIndex" );
      println( "************mainCL.dropIndex SUCCED***************" );
   }
   catch( e )
   {
      println( " Error: " + e );
      throw e;
   }
   // clean
   commDropCL( db, COMMCSNAME, subCl_Name + "1", false, false,
      "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCl_Name + "2", false, false,
      "clean sub collection" );
   commDropCL( db, COMMCSNAME, MainCL_Name, false, false,
      "clean main collection" );
}

// Add inspect standalone run mode
try
{
   // Inspect the run mode is standalone or not
   if( true == commIsStandalone( db ) )
      throw "ModeStandAlone";
   test_range_attach_hash_index_BoundTest();
}
catch( e )
{
   if( "ModeStandAlone" == e )
      println( "The run mode is standalone" );
   else
      throw e;
}
