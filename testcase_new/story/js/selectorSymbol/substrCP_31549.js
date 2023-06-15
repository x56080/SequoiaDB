/******************************************************************************
 * @Description   : $substrCP，$rightCP、$leftCP 切割大小为16m的字符串
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31549";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var dbcl = testPara.testCL;
   var array = new Array( 16 * 1024 * 1024 - 5000 );
   var str = array.join( "a" );
   dbcl.insert( [{ a: str }] );

   // substrCP切割字符串
   var actRecords = dbcl.find( {}, { a: { $substrCP: [ 15 * 1024, 10] } } );
   commCompareResults( actRecords, [{ a: "aaaaaaaaaa" }] );

   var actRecords = dbcl.find( {}, { a: { $substrCP: 1000 } } );
   array = new Array( 1000 + 1 );
   var expResult = array.join( "a" );
   commCompareResults( actRecords, [{ a: expResult }] );

   // rightCP切割字符串
   actRecords = dbcl.find( {}, { a: { $rightCP: 1000 } } );
   commCompareResults( actRecords, [{ a: expResult }] );

   // leftCP切割字符串
   actRecords = dbcl.find( {}, { a: { $leftCP: 1000 } } );
   commCompareResults( actRecords, [{ a: expResult }] );
}