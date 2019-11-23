/******************************************************************************
@Description :   seqDB-16042:  Generated字段参数校验  
@Modify list :   2018-10-24    xiaoni Zhao  Init
******************************************************************************/
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
      return;
   } 
    
   var clName = COMMCLNAME + "_16042";
   
   commDropCL( db, COMMCSNAME, clName );
   
   //create autoIncrement Generated "default"
   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement : { Field : "id1" } } );
   
   //create autoIncrement Generated "always"
   dbcl.createAutoIncrement({ Field : "id2", Generated : "always" });
   
   //create autoIncrement Generated "strict"
   dbcl.createAutoIncrement({ Field : "id3", Generated : "strict" });
   
   //create autoIncrement Generated "a"
   try
   {
      dbcl.createAutoIncrement({ Field : "id3", Generated : "a" });
      throw "create error!";
   }catch(e)
   {
      if(e !== -6)
      {
         throw new Error(e);
      } 
   }
   
   //check autoIncrement
   var cursor = db.snapshot(8, { Name : COMMCSNAME + "." + clName });
   var generated = ["default", "always", "strict"];
   var cur = cursor.current().toObj().AutoIncrement;
   cur.sort(function(a, b){return a.Field > b.Field});
   for(var i in generated)
   {
      if( cur.length !== 3)
      {
         throw new Error( "Expect is 3, but act is " + cur.length);
      }
      if( cur[i].Generated !== generated[i])
      {
         throw new Error("cur[" + i + "].Generated is " + cur[i].Generated + ", but generated[" + i + "] is " + generated[i]);
      }   
   }
   
   //insert records and check
   dbcl.insert({ "q" : 1, "id2" : 5, "id3" : 5 });
   dbcl.insert({ "q" : 2, "id1" : 5});
   try
   {
      dbcl.insert({ "q" : 2, "id1" : 2, "id2" : 5, "id3" : "f" });
      throw "need error for insert";     
   }catch(e)
   {
      if(e !== -6)
      {
         throw new Error( e );
      }
   }
   
   var rc = dbcl.find().sort( { "id1" : 1 } );
   var expRecs = [ { "q" : 1, "id1" : 1, "id2" : 1, "id3" : 5 },
                   { "q" : 2, "id1" : 5, "id2" : 2, "id3" : 6 }];
   checkRec( rc, expRecs );
  
   commDropCL( db, COMMCSNAME, clName );
}
try
{
   main();
}
catch(e)
{
   if ( e.constructor === Error )
   {
      println(e.stack) ;  
   }
   throw e ;
}
