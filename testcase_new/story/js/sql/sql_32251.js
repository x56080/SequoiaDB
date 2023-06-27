/******************************************************************************
 * @Description   : seqDB-32251:执行 inner join查询
 * @Author        : liuli
 * @CreateTime    : 2023.06.21
 * @LastEditTime  : 2023.06.27
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   var clName1 = "cl_32251_1";
   var clName2 = "cl_32251_2";

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2 );

   dbcl1.insert( { a: 1, b: 10 } );
   dbcl1.insert( { a: 2, b: 20 } );
   dbcl1.insert( { a: 3, b: 30 } );

   dbcl2.insert( { a: 1, c: 30 } );
   dbcl2.insert( { a: 2, c: 40 } );

   // inner join查询，where 按照右表的条件过滤，使用 > 过滤
   var sqlCommand = "select T1.a, T1.b, T2.c from " + COMMCSNAME + "." + clName1 +
      " as T1 inner join " + COMMCSNAME + "." + clName2 +
      " as T2 on T1.a = T2.a where T2.a > 1";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 2, "b": 20, "c": 40 }];
   commCompareResults( actResult, expResult );

   // inner join查询，where 按照右表的条件过滤，使用 < 过滤
   sqlCommand = "select T1.a, T1.b, T2.c from " + COMMCSNAME + "." + clName1 +
      " as T1 inner join " + COMMCSNAME + "." + clName2 +
      " as T2 on T1.a = T2.a where T2.a < 2";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1, "b": 10, "c": 30 }];
   commCompareResults( actResult, expResult );

   // inner join查询，where 按照左表的条件过滤，使用 < 过滤
   sqlCommand = "select T1.a, T1.b, T2.c from " + COMMCSNAME + "." + clName1 +
      " as T1 inner join " + COMMCSNAME + "." + clName2 +
      " as T2 on T1.a = T2.a where T1.a < 2";

   var actResult = db.exec( sqlCommand );
   var expResult = [{ "a": 1, "b": 10, "c": 30 }];
   commCompareResults( actResult, expResult );

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}
