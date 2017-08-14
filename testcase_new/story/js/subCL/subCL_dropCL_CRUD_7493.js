/******************************************************************************
*@Description: drop mainCL/subCL, then insert/delete/query/update
*              [ JIRA: SEQUOIADBMAINSTREAM-835 ]
*@Modify list:
*              2014-5-5  xiaojun Hu   Init
******************************************************************************/

/*******************************************************************************
*@Description: 创建主子表, 主子表不同空间, 删除子表, 在主表上做count操作
*@Input: mainCL.count()
*@Expectation: getcount操作得到的结果为0. 且count操作未hang住. CPU占比不高
********************************************************************************/
function testDropSubCLThenCount()
{
   try
   {
      var funcName = "testDropSubCLThenCount";
      var mainClName = CHANGEDPREFIX + "_MainCl";
      var subCsName1 = CHANGEDPREFIX + "_SubCs_1";
      var subCsName2 = CHANGEDPREFIX + "_SubCs_2";
      var optionObj = { "ShardingKey": {a:1}, "IsMainCL": true };
      var times = 5;
      for( var i = 0; i < times; ++i )
      {
         commDropCL( db, COMMCSNAME, mainClName, true, true,
                     "drop main collection begin, " + funcName );
         commDropCL( db, subCsName1, COMMCLNAME, true, true,
                     "drop sub sub1 collection begin, " + funcName );
         commDropCL( db, subCsName2, COMMCLNAME, true, true,
                     "drop sub sub2 collection begin, " + funcName );
         var cl = commCreateCLByOption( db, COMMCSNAME, mainClName, optionObj, true,
                                        false,
                                        "failed create main cl in the beginning" );
         commCreateCL( db, subCsName1, COMMCLNAME, 0, true, true,false,
                       "failed create sub1 collection in the beginning" );
         commCreateCL( db, subCsName2, COMMCLNAME, 0, true, true,false,
                       "failed create sub2 collection in the beginning" );
         try
         {
            cl.attachCL( subCsName1 + "." + COMMCLNAME,
                         {LowBound:{a:1}, UpBound:{a:10}} );
            cl.attachCL( subCsName2 + "." + COMMCLNAME,
                         {LowBound:{a:10}, UpBound:{a:20}} );
            println("success to attach cl");
            cl.insert({a:1});
            cl.insert({a:19});
         }
         catch( e )
         {
            if( -6 != e )
            {
               println( "failed to test create index, rc = " + e );
               throw e;
            }
         }
         commDropCS( db, subCsName1, false, "drop sub cs 1" );
         commDropCS( db, subCsName2, false, "drop sub cs 2" );
         try
         {
            println("begin to count");
            var count = cl.count();
            println("end to count");
            if( 0 != count )
            {
               throw "failed to get count from main cl";
            }
         }
         catch( e )
         {
            println( "expect count: 0, actual count: " + count );
            throw e;
         }
      }
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
   finally
   {
      commDropCL( db, COMMCSNAME, mainClName, false, false,
                  "drop main collection end, " + funcName );
   }
}

/*******************************************************************************
*@Description: 创建主子表, 主子表不同空间, 删除子表, 在主表上做
*              insert/find/update/upsert/remove操作
*@Input: mainCL.count()
*@Expectation: 除了find外的操作, 其余的操作均会报-135错误.
********************************************************************************/
function testDropSubCLThenCURD( db )
{
   try
   {
      var funcName = "testDropSubCLThenCURD";
      var mainClName = CHANGEDPREFIX + "_MainCl";
      var subCsName1 = CHANGEDPREFIX + "_SubCs_1";
      var subCsName2 = CHANGEDPREFIX + "_SubCs_2";
      var optionObj = { "ShardingKey": {a:1}, "IsMainCL": true };
      var times = 5;
      for( var i = 0; i < times; ++i )
      {
         commDropCL( db, COMMCSNAME, mainClName, true, true,
                     "drop main collection begin, " + funcName );
         commDropCL( db, subCsName1, COMMCLNAME, true, true,
                     "drop sub sub1 collection begin, " + funcName );
         commDropCL( db, subCsName2, COMMCLNAME, true, true,
                     "drop sub sub2 collection begin, " + funcName );
	     db.setSessionAttr( { PreferedInstance: "M" } );
         var cl = commCreateCLByOption( db, COMMCSNAME, mainClName, optionObj, true,
                                        false,
                                        "failed create main cl in the beginning" );
         commCreateCL( db, subCsName1, COMMCLNAME, 0, true, true,false,
                       "failed create sub1 collection in the beginning" );
         commCreateCL( db, subCsName2, COMMCLNAME, 0, true, true,false,
                       "failed create sub2 collection in the beginning" );
         try
         {
            cl.attachCL( subCsName1 + "." + COMMCLNAME,
                         {LowBound:{a:1}, UpBound:{a:10}} );
            cl.attachCL( subCsName2 + "." + COMMCLNAME,
                         {LowBound:{a:10}, UpBound:{a:20}} );
            println("success to attach cl");
            cl.insert({a:1});
            cl.insert({a:19});
         }
         catch( e )
         {
            if( -6 != e )
            {
               println( "failed to test create index, rc = " + e );
               throw e;
            }
         }
         commDropCS( db, subCsName1, false, "drop sub cs 1" );
         commDropCS( db, subCsName2, false, "drop sub cs 2" );
         // insert
         try
         {
            cl.insert({a:1});
            throw "need throw -135, but not";
         }
         catch( e )
         {
            if( -135 != e )
            {
               println( "failed to test insert, rc = " + e );
               throw e;
            }
            else
            {
               println( "test insert() over" );
            }
         }
         // find
         try
         {
            var cursor = cl.find();
            var count = cursor.count();
            if( 0 != count )
            {
               throw "failed to get count from main cl";
            }
            println( "test find() over" );
         }
         catch( e )
         {
            println( "expect count: 0, actual count: " + count );
            throw e;
         }
         // update
         try
         {
            cl.update({$set: {a:1}});
         }
         catch( e )
         {
            if( -135 != e )
            {
               println( "failed to test update, rc = " + count );
               throw e;
            }
         }
         // upsert
         try
         {
            cl.upsert({$set: {b: 2}});
            throw "need throw -135, but not";
         }
         catch( e )
         {
            if( -135 != e )
            {
               println( "failed to test upsert{b:2}, rc = " + e );
               throw e;
            }
            else
            {
               println( "test upsert({$set: {b: 2}}) over" );
            }
         }
         // remove
         try
         {
            cl.remove();
         }
         catch( e )
         {
            if( -135 != e )
            {
               println( "failed to test remove, rc = " + e );
               throw e;
            }
         }
      }
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
   finally
   {
      commDropCL( db, COMMCSNAME, mainClName, false, false,
                  "drop main collection end, " + funcName );
   }

}

function main()
{
   try
   {
      if( false == commIsStandalone( db ) )
      {
         testDropSubCLThenCount( db );
         testDropSubCLThenCURD( db );
      }

   }
   catch( e )
   {
      throw e;
   }
}

main();