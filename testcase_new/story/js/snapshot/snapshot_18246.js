/******************************************************************************
*@Description : seqDB-18246:检查 snapshot(SDB_SNAP_COLLECTIONS) 快照信息
*               TestLink : seqDB-18246
*@auhor       : yinzhen
******************************************************************************/

function main()
{
   if(commIsStandalone( db )){
      println("Deploy is standalone");
      return;
   }
   var clName = COMMCLNAME + "_18246";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   var cur = db.snapshot( SDB_SNAP_COLLECTIONS, {"Name":COMMCSNAME + "." + clName} );
   while( cur.next() )
   {
      var ret = cur.current().toObj();  
      if(ret.Name != COMMCSNAME + "." + clName)
      {
         throw new Error("ExpResult is " + COMMCSNAME + "." + clName + ", but actResult is " + ret.Name);
      }
      recurObj(ret);
   }   
   commDropCL( db, COMMCSNAME, clName );
}

function recurObj(obj){
   try
   {
      for(var item in obj){
         if(typeof(obj[item]) == "object"){
            return recurObj(obj[item]);
         }
         if (obj[item] == null){
            throw "expResult is " + obj[item] + ", but actResult is null";
         }
      }
   }
   catch(e)
   {
      throw new Error(e);
   }
}

try
{
   main(db) ;
}
catch(e)
{
   if(e.constructor === Error)
   {
      println(e.stack);
   }
   throw e;
}
