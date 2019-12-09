/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-3 wenjing wang  Init
*******************************************************************************/

/*******************************************************************************
*@Description：测试op为update时, 更新规则字段为空
*@Input：find( {a:1} ).update( {} )
*@Expectation：预期抛出如下异常:
SdbQuery.update(): the 1st param should be non-empty object
********************************************************************************/
function testRuleEmpty ( cl )
{
   var funname = "testRuleEmpty";
   try
   {
      cl.find( { a: 1 } ).update( {} );
   }
   catch( e )
   {
      if( errMsg.FISRTPARAM != e )
      {
         throw buildException( funname, e, "find( {a:1} ).update( {} )", errMsg.FISRTPARAM, e );
      }
   }
}

/*******************************************************************************
*@Description：测试op为update时, returnnew为非boolean值
*@Input：find( {a:1} ).update( {$inc:{a:1}}, 1 )
*@Expectation：预期抛出如下异常:
SdbQuery.update(): the 2nd param should be boolean
********************************************************************************/
function testReturnNewNonBoolean ( cl )
{
   var funname = "testReturnNewNonBoolean";
   try
   {
      cl.find( { a: 1 } ).update( { $inc: { a: 1 } }, 1 ).toArray();
   }
   catch( e )
   {
      if( errMsg.SECONDPARAM != e )
      {
         throw buildException( funname, e, "find( {a:1} ).update( {$inc:{a:1}}, 1 )", errMsg.SECONDPARAM, e );
      }
   }
}

/*******************************************************************************
*@Description：测试op为update时, 正常操作
*@Input：find( {a:1} ).update( {$inc:{a:1}}, false )
*@Expectation：返回文档的a字段为了2，数据库中{a:2}的文档数为1
********************************************************************************/
function testNormal ( cl, useflag )
{
   var funname = "testNormal";
   if( undefined != useflag )
   {
      var returnNew = false;
   }

   try
   {
      loadSingleDoc( cl );
      if( undefined == returnNew )
      {
         var arr = cl.find( { a: 1 } ).update( { $inc: { a: 1 } } ).toArray();
      }
      else
      {
         var arr = cl.find( { a: 1 } ).update( { $inc: { a: 1 } }, returnNew ).toArray();
      }

      var obj = eval( "( " + arr[0] + " )" )
      if( 1 != obj["a"] )
      {
         throw -1
      }

      var recordnum = cl.find( { a: 2 } ).count();
      if( 1 != parseInt( recordnum ) )
      {
         throw -2
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "find( {a:1} ).update( {$inc:{a:1}} )", "{a:1}", "{a:" + obj["a"] + "}" );
      }
      else if( -2 == e )
      {
         buildException( funname, e, "find( {a:2} ).count()", 1, recordnum );
      }
      else
      {
         throw buildException( "testNormal", e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

/*******************************************************************************
*@Description：测试op为update时, returnnew为真
*@Input：find( {a:1} ).update( {$inc:{a:1}}, true )
*@Expectation：返回文档的a字段为了2，数据库中{a:2}的文档数为1
********************************************************************************/
function testReturnNewIsTrue ( cl )
{
   var funname = "testReturnNewIsTrue";
   try
   {
      loadSingleDoc( cl );
      var arr = cl.find( { a: 1 } ).update( { $inc: { a: 1 } }, true ).toArray();
      var obj = eval( "( " + arr[0] + " )" )

      if( 2 != obj["a"] )
      {
         throw -1;
      }

      var recordnum = cl.find( { a: 2 } ).count()
      if( 1 != parseInt( recordnum ) )
      {
         throw -2;
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "find( {a:1} ).update( {$inc:{a:1}}, true )", "{a:2}", "{a:" + obj["a"] + "}" );
      }

      if( -2 == e )
      {
         throw buildException( funname, e, "find( {a:2} ).count()", 1, recordnum );
      }
      else
      {
         throw buildException( funname, e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

/*******************************************************************************
*@Description：测试op为update时, 对返回结果排序，排序字段存在索引
*@Input：find().update( {$set:{c:1}}, true ).sort( {_id:-1} )
*@Expectation：结果按降序输出
********************************************************************************/
function testSortExistIndex ( cl )
{
   var funname = "testSortExistIndex";
   try
   {
      loadMultipleDoc( cl, 10 );
      var arr = cl.find().update( { $set: { c: 1 } }, true ).sort( { _id: -1 } ).hint( { "": "$id" } ).toArray();
      if( 10 != arr.length )
      {
         throw -1;
      }

      var maxval = 0;
      for( var i = 0; i < arr.length; ++i )
      {
         var obj = eval( "( " + arr[i] + " )" );
         if( 0 == i )
         {
            maxval = obj["_id"];
         }
         else if( maxval < obj["_id"] )
         {
            throw -2;
         }
         else if( 1 != obj["c"] )
         {
            throw -3;
         }
      }
   }
   catch( e )
   {
      var oper = find().update( { $set: { c: 1 } }, true ).sort( { _id: -1 } ).toArray();
      if( -1 == e )
      {
         throw buildException( funname, e, oper, "arr.length eq 10", "arr.length eq " + arr.length )
      }
      else if( -2 == e )
      {
         var tmpmsg = "_id:";
         for( var c = arr.length; c >= 0; c-- )
         {
            var obj = eval( "( " + arr[0] + " )" );
            tmpmsg = tmpmsg + obj["_id"];
            if( c != 0 )
            {
               tmpmsg += ", ";
            }
         }
         throw buildException( funname, e, oper, "_id:9, 8, 5, 4, 3, 2, 1", tmpmsg );
      }
      else if( -3 == e )
      {
         throw buildException( funname, e, oper, "{c:1}", "{c:" + obj["c"] + "}" );
      }
      else
      {
         throw buildException( funname, e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

/*******************************************************************************
*@Description：测试op为update时, 对返回结果排序，排序字段不存在索引
*@Input：find().update( {$set:{c:1}}, true ).sort( {c:-1} )
*@Expectation：报-288错误
********************************************************************************/
function testSortNotExistIndex ( cl )
{
   try
   {
      loadMultipleDoc( cl, 10 );
      cl.find().update( { $set: { c: 1 } }, true ).sort( { c: -1 } );
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_SORT_NO_IDX != e )
      {
         throw buildException( funname, e, "find().update( {$set:{c:1}}, true ).sort( {c:-1} )", errCode.SDB_RTN_QUERYMODIFY_SORT_NO_IDX, e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

/*******************************************************************************
*@Description：测试op为update时, 给合count
*@Input：find( {a:1} ).update( {$inc:{a:1}} ).count()
*@Expectation：throw如下异常:
count() cannot be executed with update() or remove()
********************************************************************************/
function testWithCount ( cl )
{
   var funname = "testWithCount";
   try
   {
      loadSingleDoc( cl );
      cl.find( { a: 1 } ).update( { $inc: { a: 1 } } ).count();
   }
   catch( e )
   {
      if( errMsg.WITHCOUNT != e )
      {
         throw buildException( funname, e, "find( {a:1} ).update( {$inc:{a:1} ).count()", errMsg.WITHCOUNT, e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

/*******************************************************************************
*@Description：测试op为update时, 不切分的情况下使用skip
*@Input：find().skip( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function testUsedSkipNonSplit ( cl )
{
   var funname = "testUsedSkipNonSplit";
   try
   {
      loadMultipleDoc( cl, 10 );
      var arrdoc = cl.find().skip( 5 ).update( { $set: { b: 1 } } ).toArray();

      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1;
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "find().skip( 5 ).update( {$set:{b:1}} )", true, false );
      }
      else
      {
         throw buildException( funname, e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

/*******************************************************************************
*@Description：测试op为update时, 不切分的情况下使用limit
*@Input：find().limit( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function testUsedLimitNonSplit ( cl )
{
   var funname = "testUsedLimitNonSplit";
   try
   {
      loadMultipleDoc( cl, 10 );

      var arrdoc = cl.find().limit( 5 ).update( { $set: { b: 1 } } ).toArray();

      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1;
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "find().limit( 5 ).update( {$set:{b:1}} )", true, false );
      }
      else
      {
         throw buildException( funname, e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

/*******************************************************************************
*@Description：测试op为update时, 不切分的情况下使用skip + limit
*@Input：find().skip( 5 ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function testUsedSkipAndLimitNonSplit ( cl )
{
   var funname = "testUsedSkipAndLimitNonSplit";
   try
   {
      loadMultipleDoc( cl, 10 );

      var arrdoc = cl.find().skip( 5 ).limit( 2 ).update( { $set: { b: 1 } } ).toArray();

      if( !checkUpdateResult( cl, arrdoc ) )
      {
         throw -1;
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "find().skip( 5 ).limit( 2 ).update( {$set:{b:1}} )", true, false );
      }
      else
      {
         throw buildException( funname, e );
      }
   }
   finally
   {
      removeAllDoc( cl );
   }
}

function main ()
{
   try
   {
      var db = setUp();
      var cl = getCL( db );
      testRuleEmpty( cl );
      testReturnNewNonBoolean( cl );
      testNormal( cl );
      testNormal( cl, true );
      testReturnNewIsTrue( cl );
      testSortNotExistIndex( cl );
      testSortExistIndex( cl );
      testWithCount( cl )
      testUsedSkipNonSplit( cl );
      testUsedLimitNonSplit( cl );
      testUsedSkipAndLimitNonSplit( cl );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      tearDown( db );
   }
}

main()
