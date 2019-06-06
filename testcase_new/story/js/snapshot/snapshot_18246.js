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
         throw buildException("compare name", "", "snapshot( SDB_SNAP_COLLECTIONS, option)", COMMCSNAME + "." + clName, ret.Name);
      }
      recurObj(ret);
   }   
   commDropCL( db, COMMCSNAME, clName );
}

function recurObj(obj){
   for(var item in obj){
      if(typeof(obj[item]) == "object"){
         return recurObj(obj[item]);
      }
      if (obj[item] == null){
         println(item + ": " + obj[item]);
         throw buildException("compare item", "", "snapshot( SDB_SNAP_COLLECTIONS, option)", "null", obj[item]);
      }
   }
}

main(db) ;