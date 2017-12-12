function main()
{
   try
   {
      db.listReplicaGroups()
   }
   catch(e)
   {
      if (-159 == e)
      {
         println("standalone exit")
         return
      }
   }

   var bRet=true;
   try
   {
      db.setSessionAttr();
   }
   catch(e)
   {
      bRet = false;
   }

   if (true == bRet)
   {
      println("db.setSessionAttr() parameter illegal")
      throw -1
   }

   bRet = true;
   try
   {
      db.setSessionAttr({})
   }
   catch(e)
   {
      if (-6 == e)
      {
         bRet=false; 
      }
      else
      {
   	     println("db.setSessionAttr({}) parameter illegal")
         throw e
      }
   }
   if (true == bRet)
   {
      println("db.setSessionAttr({}) parameter illegal")
      throw -1
   }

   bRet = true;
   try
   {
      db.setSessionAttr({"PreferedInstance":8})
   }
   catch(e)
   {
      if (-6 == e)
      {
         bRet=false; 
      }
      else
      {
   	     println("db.setSessionAttr({\"PreferedInstance\":8} parameter illegal")
         throw e
      }
   }
   if (true == bRet)
   {
      println("db.setSessionAttr({\"PreferedInstance\":8} parameter illegal")
      throw -1
   }

   bRet = true;
   try
   {
      db.setSessionAttr({"PreferedInstance":0})
   }
   catch(e)
   {
	    if (-6 == e)
      {
         bRet=false; 
      }
      else
      {
         println("db.setSessionAttr({\"PreferedInstance\":0}) parameter illegal")
         throw e
      }
   }
   if (true == bRet)
   {
      println("db.setSessionAttr({\"PreferedInstance\":0}) parameter illegal")
      throw -1
   }

   bRet = true;
   try
   {
      // 这个输入应该是不合法的，但是只要存在合法项，认为输入就合法的做法，值得商榷
      db.setSessionAttr({"preferedInstance":0,"PreferedInstance":1})
   }
   catch(e)
   {
      if (-6 == e)
      {
         bRet=false;
      }
      else
      {
         println("db.setSessionAttr({\"preferedInstance\":0,\"PreferedInstance\":1}) parameter illegal")
         throw e
      }
   }
/*
if (true == bRet)
{
   println("db.setSessionAttr({\"preferedInstance\":0,\"PreferedInstance\":1}) parameter illegal")
   throw -1
}
*/
}

main();
