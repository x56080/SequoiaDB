/******************************************************************************
 * @Description   : seqDB-32253:enablemixcmp 设置为 false，查询比较包含不同类型
 * @Author        : liuli
 * @CreateTime    : 2023.06.21
 * @LastEditTime  : 2023.06.27
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32253";

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;

   dbcl.insert( { a: 1, b: 1 } );
   dbcl.insert( { b: 2 } );
   dbcl.insert( { a: "a", b: 3 } );

   // 使用 < 过滤字符串，包含null和数值
   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a < 'abc'";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": "a" }];
   commCompareResults( actResult, expResult );

   // 使用 <= 过滤字符串，包含null和数值
   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a <= 'abc'";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": "a" }];
   commCompareResults( actResult, expResult );

   // 使用 > 过滤数值，包含null和字符串
   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a > 0";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );

   // 使用 >= 过滤数值，包含null和字符串
   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a >= 0";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );

   // 使用 < 过滤数值，包含null和字符串
   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a < 2";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );

   // 使用 <= 过滤数值，包含null和字符串
   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a <= 2";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );
}
