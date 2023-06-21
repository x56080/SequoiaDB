/******************************************************************************
 * @Description   : seqDB-32252:enablemixcmp 设置为 true
 * @Author        : liuli
 * @CreateTime    : 2023.06.21
 * @LastEditTime  : 2023.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32252";

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;

   dbcl.insert( { a: 1, b: 1 } );
   dbcl.insert( { b: 2 } );
   dbcl.insert( { a: "a", b: 3 } );

   try
   {
      db.updateConf( { enablemixcmp: true } );

      var sqlCommand = "select * from ( select first(a) as a from " +
         COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a < 'abc' order by T.a";

      var actResult = db.exec( sqlCommand );
      var expResult = [{ "a": null }, { "a": 1 }, { "a": "a" }];
      commCompareResults( actResult, expResult );

      var sqlCommand = "select * from ( select first(a) as a from " +
         COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a <= 'abc' order by T.a";

      var actResult = db.exec( sqlCommand );
      var expResult = [{ "a": null }, { "a": 1 }, { "a": "a" }];
      commCompareResults( actResult, expResult );

      var sqlCommand = "select * from ( select first(a) as a from " +
         COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a > 0 order by T.a";

      var actResult = db.exec( sqlCommand );
      var expResult = [{ "a": 1 }, { "a": "a" }];
      commCompareResults( actResult, expResult );

      var sqlCommand = "select * from ( select first(a) as a from " +
         COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a >= 0 order by T.a";

      var actResult = db.exec( sqlCommand );
      var expResult = [{ "a": 1 }, { "a": "a" }];
      commCompareResults( actResult, expResult );

      var sqlCommand = "select * from ( select first(a) as a from " +
         COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a < 2 order by T.a";

      var actResult = db.exec( sqlCommand );
      var expResult = [{ "a": null }, { "a": 1 }];
      commCompareResults( actResult, expResult );

      var sqlCommand = "select * from ( select first(a) as a from " +
         COMMCSNAME + "." + testConf.clName + " group by b ) as T where T.a <= 2 order by T.a";

      var actResult = db.exec( sqlCommand );
      var expResult = [{ "a": null }, { "a": 1 }];
      commCompareResults( actResult, expResult );
   } finally
   {
      db.deleteConf( { enablemixcmp: 1 } );
   }
}
