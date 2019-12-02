/*******************************************************************************
*@Description: findandremove basic testcases
*@Modify list:
*   2014-4-3 wenjing wang  Init
*******************************************************************************/


/***********************************************************************
*@Description：op为remove时, 正常操作
*@Input：find( {a:1} ).remove()
*@Expectation：删除存在{a:1}的文档
*************************************************************************/
function testNormal( cl )
{
   var funname = "testNormal"; 
   try
   {
      loadSingleDoc( cl ); 
      var arr = cl.find( {a:1} ).remove().toArray(); 
      var obj = eval( "( " + arr[0] + " )" )
      if( 1 != obj["a"] )
      {
         throw -1
      }
      
      var recordnum = cl.find( {a:1} ).count()
      if( 0 != parseInt( recordnum ) )
      {
         throw -2
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "find( {a:1} ).remove()", "{a:1}", "{a" + obj["a"] + "}" ); 
      }
      else if( -2 == e )
      {
         throw buildException( funname, e, "find( {a:1} ).count()", 0, recordnum ); 
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

/****************************************************************
*@Description：op为remove时，针对结果做排序，排序字段上存在索引
*@Input：find().remove().sort( {_id:-1} )
*@Expectation：所有记录被删除，结果按降序输出
*****************************************************************/
function testSortExistIndex( cl )
{
   var funname = "testSortExistIndex"; 
   try
   {
      loadMultipleDoc( cl, 10 ); 
      var arr = cl.find().remove().sort( {_id:-1} ).hint( {"":"$id"} ).toArray(); 
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
            maxval = obj._id; 
         }
         else if( maxval < obj._id )
         {
            throw -2; 
         }
      }
      
      var recordnumber = cl.find().count(); 
      if( 0 != parseInt( recordnumber ) )
      {
         throw -3; 
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "cl.find().remove().sort( {_id:-1} ).toArray()", "arr.length eq 10", "arr.length eq" + arr.length ); 
      }
      else if( -2 == e )
      {
         var tmpmsg = "_id:"
         for( var i = 0; i < arr.length; ++i )
         {
            var obj = eval( "( " + arr[i] + " )" ); 
            tmpmsg += obj._id; 
            if( 0 == i )
            {
               tmpmsg += ", "
            }
         }
         throw buildException( funname, e, "find().remove().sort( {_id:-1} )", "_id:9, 8, 7, 6, 5, 4, 3, 2, 1", tmpmsg ); 
      }
      else if( -3 == e )
      {
         throw buildException( funname, e, "find().count()", 0, recordnumber ); 
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

/****************************************************************
*@Description：op为remove时，针对结果做排序，排序字段上不存在索引
*@Input：find().remove().sort( {c:-1} )
*@Expectation：报-288错误
*****************************************************************/
function testSortNotExistIndex( cl )
{
   var funname = "testSortNotExistIndex"; 
   try
   {
      loadMultipleDoc( cl, 10 ); 
      cl.find().remove().sort( {c:-1} ).toArray(); 
   }
   catch( e )
   {
      if( errCode.SDB_RTN_QUERYMODIFY_SORT_NO_IDX != e )
      {
         throw buildException( funname, "find().remove().sort( {c:-1} )", errCode.SDB_RTN_QUERYMODIFY_SORT_NO_IDX, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/****************************************************************
*@Description：op为remove时，针对结果做count
*@Input：find( {a:1} ).remove().count()
*@Expectation：报
count() cannot be executed with update() or remove()
*****************************************************************/
function testWithCount( cl )
{
   var funname = "testWithCount"; 
   try
   {
      loadSingleDoc( cl ); 
      cl.find( {a:1} ).remove().count(); 
   }
   catch( e )
   {
      if( errMsg.WITHCOUNT != e )
      {
         throw buildException( funname, "find( {a:1} ).remove().count()", errMsg.WITHCOUNT, e ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/****************************************************************
*@Description：op为remove时，给合skip
*@Input：find().skip( 5 ).remove()
*@Expectation：返回的记录被删除
*****************************************************************/
function testUsedSkipNonSplit( cl )
{
   var funname = "testUsedSkipNonSplit"; 
   try
   {
      loadMultipleDoc( cl, 10 ); 
      var arrdoc = cl.find().skip( 5 ).remove().toArray(); 
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, "find().skip( 5 ).remove()", true, false ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/****************************************************************
*@Description：op为remove时，给合limit
*@Input：find().limit( 5 ).remove()
*@Expectation：返回的记录被删除
*****************************************************************/
function testUsedLimitNonSplit( cl )
{
   var funname = "testUsedLimitNonSplit"; 
   try
   {
      loadMultipleDoc( cl, 10 ); 
      
      var arrdoc = cl.find().limit( 5 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, "find().limit( 5 ).remove()", true, false ); 
      }
   }
   finally
   {
      removeAllDoc( cl ); 
   }
}

/****************************************************************
*@Description：op为remove时，给合limit
*@Input：find().skip( 5 ).limit( 2 ).remove()
*@Expectation：返回的记录被删除
*****************************************************************/
function testUsedSkipAndLimitNonSplit( cl )
{
   try
   {
      loadMultipleDoc( cl, 10 ); 
      
      var arrdoc = cl.find().skip( 5 ).limit( 2 ).remove().toArray(); 
      
      if( !checkRemoveResult( cl, arrdoc ) )
      {
         throw -1; 
      }
   }
   catch( e )
   {
      if( -1 == e )
      {
         throw buildException( funname, e, "find().skip( 5 ).limit( 2 ).remove()", true, false ); 
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

function main()
{
   try
   {
      var db = setUp(); 
      var cl = getCL( db ); 
      testNormal( cl ); 
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
