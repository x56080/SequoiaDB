/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-7 wenjing wang  Init
*******************************************************************************/
/*******************************************************************************
*@Description：测试op为update时, 设置setSessionAttr({PreferedInstance:"S"})
*@Input：find().update({$inc:{a:1}})
*@Expectation：更新在主节点上生效  
********************************************************************************/
function test_SetSessionAttrIsSlaveWithUpdate(db, cl)
{
   var funname = "test_SetSessionAttrIsSlaveWithUpdate";
   try
   {
      loadSingleDoc(cl)
      db.setSessionAttr({PreferedInstance:"S"});
      var arr = cl.find().update({$inc:{a:1}}).toArray();
      
      db.setSessionAttr({PreferedInstance:"M"});
      var recordnum = cl.find({a:2}).count();
      if ( 1 != parseInt(recordnum) )
      {
         throw -1;
      }
   }
   catch(e)
   {
      if (-1 == e)
      {
         throw buildException(funname, e, "find({a:2}).count()", 1, recordnum);
      }
      else
      {
         throw buildException(funname, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

/*******************************************************************************
*@Description：测试op为remove时, 设置setSessionAttr({PreferedInstance:"S"})
*@Input：find({a:1}).remove()
*@Expectation：更新在主节点上生效  
********************************************************************************/
function test_SetSessionAttrIsSlaveWithRemove(db, cl)
{
   var funname = "test_SetSessionAttrIsSlaveWithRemove";
   try
   {
      loadSingleDoc(cl)
      db.setSessionAttr({PreferedInstance:"S"});
      var arr = cl.find({a:1}).remove().toArray();
      
      db.setSessionAttr({PreferedInstance:"M"});
      var recordnum = cl.find({a:1}).count();
      if ( 0 != parseInt(recordnum) )
      {
         throw -1;
      }
   }
   catch(e)
   {
      if (-1 == e)
      {
         throw buildException(funname, e, "find({a:1}).count()", 0, recordnum);
      }
      else
      {
         throw buildException(funname, e);
      }
   }
   finally
   {
      removeAllDoc(cl);
   }
}

function main()
{
   try
   {
      var db = setUp();
      if (commIsStandalone(db))
      {
         return;
      }
      var cl = getCL(db);
      test_SetSessionAttrIsSlaveWithUpdate(db, cl);
      test_SetSessionAttrIsSlaveWithRemove(db, cl);
   }
   catch(e)
   {
      throw e;
   }
   finally
   {
      tearDown(db);
   }
}

main()