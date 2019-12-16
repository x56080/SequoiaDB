/******************************************************************************
*@Description : seqDB-18280:垂直/水平分区表，创建索引指定NotNull 
*@Author      : 2019-5-6  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   }

   var mclName = "mcl_18280";
   var sclName = "scl_18280";
   var mIdxName = "mIdx";
   var sIdxName = "sIdx";

   // ready cl
   commDropCL( db, COMMCSNAME, mclName, true, true, "Failed to drop CL in the pre-condition." );
   commDropCL( db, COMMCSNAME, sclName, true, true, "Failed to drop CL in the pre-condition." );

   /************************* test1, main cl[ NotNull:true ], sub cl[ NotNull:false ] *******************/
   println( "\n---Test1, main cl[ NotNull:true ], sub cl[ NotNull:false ]." );
   println( "   Create main cl, and create index." );
   var mCL = commCreateCL( db, COMMCSNAME, mclName, { ShardingKey: { a: 1 }, IsMainCL: true }, true, true );
   mCL.createIndex( mIdxName, { b: 1 }, { NotNull: true } );
   // check index of main cl 
   try 
   {
      mCL.getIndex( mIdxName );
   }
   catch( e ) 
   {
      if( e !== -47 )
      {
         throw buildException( "checkResult", null, "", -47, "  " + e );
      }
   }

   println( "   Create sub cl, and create index." );
   var sCL = commCreateCL( db, COMMCSNAME, sclName, { ShardingKey: { a: 1 } }, true, true );
   sCL.createIndex( sIdxName, { b: 1 }, { NotNull: false } );

   println( "   Attach sub cl." );
   mCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: { $minKey: 0 } }, UpBound: { a: { $maxKey: 0 } } } );

   println( "   Insert." );
   var recs = [{ a: 1, b: 1 }, { a: 2 }, { a: 3, b: null }];
   mCL.insert( recs );

   checkIndex( mCL, sIdxName, false );
   checkRecords( mCL, recs );

   // clean index
   mCL.dropIndex( sIdxName );
   mCL.remove();

   // clean env
   commDropCL( db, COMMCSNAME, mclName, false, false, "Failed to drop CL in the end-condition" );


   /************************* test2, main cl[ NotNull:false ], sub cl[ NotNull:true ] *******************/
   println( "\n---Test2, main cl[ NotNull:false ], sub cl[ NotNull:true ]." );
   println( "   Create main cl, and create index." );
   var mCL = commCreateCL( db, COMMCSNAME, mclName, { ShardingKey: { a: 1 }, IsMainCL: true }, true, true );
   mCL.createIndex( mIdxName, { b: 1 }, { NotNull: false } );

   println( "   Create sub cl, and create index." );
   var sCL = commCreateCL( db, COMMCSNAME, sclName, { ShardingKey: { a: 1 } }, true, true );
   sCL.createIndex( sIdxName, { b: 1 }, { NotNull: true } );

   println( "   Attach sub cl." );
   mCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: { $minKey: 0 } }, UpBound: { a: { $maxKey: 0 } } } );

   println( "   Insert." );
   var valRecs = [{ a: 1, b: 1 }];
   var invRecs = [{ a: 2 }, { a: 3, b: null }];
   mCL.insert( valRecs );
   for( i = 0; i < invRecs.length; i++ ) 
   {
      try
      {
         mCL.insert( invRecs[i] );
      }
      catch( e ) 
      {
         if( e !== -339 )
         {
            throw buildException( "checkResult", null, "", -339, "  " + e );
         }
      }
   }

   checkIndex( mCL, sIdxName, true );
   checkRecords( mCL, valRecs );

   // clean env
   commDropCL( db, COMMCSNAME, mclName, false, false, "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expNot ) 
{
   println( "   Check index." );
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actNot = indexDef.NotNull;
   if( actNot !== expNot )
   {
      throw buildException( "checkResult", null, "", expNot, "  " + actNot );
   }
}

function checkRecords ( cl, expRecs ) 
{
   println( "   Check records." );
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }

   if( JSON.stringify( expRecs ) !== JSON.stringify( actRecs ) )
   {
      throw buildException( "checkResult", null, "", JSON.stringify( expRecs ), "  " + JSON.stringify( actRecs ) );
   }
}