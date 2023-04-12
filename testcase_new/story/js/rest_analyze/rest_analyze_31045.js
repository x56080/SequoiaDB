/******************************************************************************
 * @Description   :seqDB-31045:getInedxstatistic查询指定索引的统计信息
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.07
 * @LastEditTime  : 2023.04.11
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_31045";
testConf.clName = COMMCLNAME + "_31045";

main( test );

function test ()
{
   var csName = COMMCSNAME + "_31045";
   var clName = COMMCLNAME + "_31045";
   var indexName = "indexName31045"

   var dbcl = testPara.testCL;

   dbcl.createIndex( indexName, { a: 1 } );
   dbcl.insert( { a: 2 } );
   db.analyze( { CollectionSpace: csName } );

   var curlPara = ["cmd=get index statistic&name=" + csName + "." + clName + "&index=" + indexName];
   tryCatch( curlPara, [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Collection, csName + "." + clName );
   assert.equal( JSON.parse( infoSplit[1] ).Index, indexName );
   assert.equal( JSON.parse( infoSplit[1] ).MCV, undefined );

   //测试点单号：SEQUOIADBMAINSTREAM-9404
   // curlPara = ["cmd=get index statistic&name=" + csName + "." + clName + "&index=indexName1"];
   // tryCatch( curlPara, [0] );
   // if( !( JSON.parse( infoSplit[1] ) == undefined ) )
   // {
   //    throw e;
   // }

   curlPara = ["cmd=get index statistic&name=" + csName + "." + clName + "&index=" + indexName + "&detail=true"];
   tryCatch( curlPara, [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Collection, csName + "." + clName );
   assert.equal( JSON.parse( infoSplit[1] ).Index, indexName );
   assert.equal( JSON.parse( infoSplit[1] ).MCV.Values[0].a, 2 );

   curlPara = ["cmd=get index statistic&name=" + csName + "." + clName + "&index=" + indexName + "&detail=false"];
   tryCatch( curlPara, [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Collection, csName + "." + clName );
   assert.equal( JSON.parse( infoSplit[1] ).Index, indexName );
   assert.equal( JSON.parse( infoSplit[1] ).MCV, undefined );

   curlPara = ["cmd=get index statistic&name=" + csName + "." + clName + "&index=" + indexName + "&detail=fal"];
   tryCatch( curlPara, [SDB_INVALIDARG] );
}
