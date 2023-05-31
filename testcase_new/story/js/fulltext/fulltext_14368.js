/******************************************************************************
 * @Description   : seqDB-14368 :创建全文索引，固定集合名验证 
 * @Author        : YinZhen 
 * @CreateTime    : 2018.10.25
 * @LastEditTime  : 2023.05.19
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var clName = COMMCLNAME + "_ES_14368";
   var csName = "testCS_ES_14368";
   dropCL( db, COMMCSNAME, clName, true, true );
   dropCS( db, csName, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var dbcl1 = commCreateCL( db, csName, clName );
   var dbcl2 = commCreateCL( db, csName, clName + "_2", { ShardingKey: { content: 1 }, ShardingType: "hash" } )
   var dbcl3 = commCreateCL( db, csName, clName + "_3", { ShardingKey: { content: 1 }, ShardingType: "range" } )
   var subCL1 = commCreateCL( db, csName, clName + "_4_1" );
   var subCL2 = commCreateCL( db, csName, clName + "_4_2" );
   var mainCL = commCreateCL( db, csName, clName + "_4", { ShardingKey: { content: 1 }, ShardingType: "range", IsMainCL: true } )
   mainCL.attachCL( csName + "." + clName + "_4_1", { LowBound: { content: "a" }, UpBound: { content: "f" } } );
   mainCL.attachCL( csName + "." + clName + "_4_2", { LowBound: { content: "x" }, UpBound: { content: "z" } } );

   //在不同的集合空间，不同的集合创建全文索引
   var indexName = "a_14368";
   commCreateIndex( dbcl, indexName, { content: "text" } );
   commCreateIndex( dbcl1, indexName, { content: "text" } );
   commCreateIndex( dbcl2, indexName, { content: "text" } );
   commCreateIndex( dbcl3, indexName, { content: "text" } );
   commCreateIndex( mainCL, indexName, { content: "text" } );

   //获取固定集合名 
   var cappedArray = new Array();
   var cappedCLName = dbOpr.getCappedCLName( dbcl, indexName );
   cappedArray.push( cappedCLName );
   var cappedCLName1 = dbOpr.getCappedCLName( dbcl1, indexName );
   cappedArray.push( cappedCLName1 );
   var cappedCLName2 = dbOpr.getCappedCLName( dbcl2, indexName );
   cappedArray.push( cappedCLName2 );
   var cappedCLName3 = dbOpr.getCappedCLName( dbcl3, indexName );
   cappedArray.push( cappedCLName3 );
   var cappedSubCLName1 = dbOpr.getCappedCLName( subCL1, indexName );
   cappedArray.push( cappedSubCLName1 );
   var cappedSubCLName2 = dbOpr.getCappedCLName( subCL2, indexName );
   cappedArray.push( cappedSubCLName2 );

   //检查固定集合名均不一致
   for( var i in cappedArray )
   {
      var cpName = cappedArray[i];
      if( cappedArray.indexOf( cpName ) != cappedArray.lastIndexOf( cpName ) )
      {
         throw new Error( "exists duplicate capped cl name, cappedName: " + cpName + "in cappedArray: " + JSON.stringify( cappedArray ) );
      }
   }

   //检查索引属性的ExtDataName和固定集合名一致
   checkExtDataName( dbcl, indexName, cappedCLName );
   checkExtDataName( dbcl1, indexName, cappedCLName1 );
   checkExtDataName( dbcl2, indexName, cappedCLName2 );
   checkExtDataName( dbcl3, indexName, cappedCLName3 );
   checkExtDataName( subCL1, indexName, cappedSubCLName1 );
   checkExtDataName( subCL2, indexName, cappedSubCLName2 );

   dropCL( db, COMMCSNAME, clName, true, true );
   dropCS( db, csName, true );
}

function checkExtDataName ( dbcl, indexName, cappedCLName )
{
   var index = dbcl.getIndex( indexName ).toObj();
   var extDataName = index["ExtDataName"];
   if( cappedCLName != extDataName )
   {
      throw new Error( "index's property ExtDataName is not equal to cappedCLName, cappedName: " + cappedCLName + ",extDataName: " + extDataName );
   }
}
