/******************************************************************************
 * @Description   : seqDB-26735:find匹配条件使用BSONObj
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.07.21
 * @LastEditTime  : 2022.07.22
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26735";
main( test );

function test ( args )
{
   var cl = args.testCL;
   var insertNum = 100;
   var docs = [];
   for( var i = 1; i < insertNum; i++ )
   {
      docs.push( { a: i } );
   }
   cl.insert( docs );
   var expRecs = [{ a: 1 }];
   var cond = new BSONObj( { a: 1 } );
   var cursor = cl.find( cond );
   commCompareResults( cursor, expRecs );
}
