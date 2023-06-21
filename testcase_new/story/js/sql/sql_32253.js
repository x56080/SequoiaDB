/******************************************************************************
 * @Description   : seqDB-32253:enablemixcmp 设置为 false
 * @Author        : liuli
 * @CreateTime    : 2023.06.21
 * @LastEditTime  : 2023.06.21
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

   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a < 'abc'";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": "a" }];
   commCompareResults( actResult, expResult );

   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a <= 'abc'";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": "a" }];
   commCompareResults( actResult, expResult );

   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a > 0";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );

   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a >= 0";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );

   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a < 2";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );

   var sqlCommand = "select * from ( select first(a) as a from " +
      COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a <= 2";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1 }];
   commCompareResults( actResult, expResult );
}
