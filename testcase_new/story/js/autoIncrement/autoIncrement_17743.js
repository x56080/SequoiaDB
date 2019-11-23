/******************************************************************************
@Description :    seqDB-17743: increment为负值，插入值比序列的MinValue稍大，MinValue为负边界值，再次插入自增字段超过MinValue 
@Modify list :   2018-1-29    Zhao Xiaoni  Init
******************************************************************************/
function main()
{
   var coordNodes = getCoordNodeNames();
   if(coordNodes.length < 3 || commIsStandalone( db ))
   {
      println("Deploy is standalone or coord nodes is less than 3!");
      return;
   }
    
   var clName = COMMCLNAME + "_17743";
   var increment = -2;
   var acquireSize = 100;
   var sortField = 0;
   
   commDropCL( db, COMMCSNAME, clName );
   
   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement : { Field : "id", Increment : increment, 
                                        AcquireSize : acquireSize, Cycled : true } } );
   commCreateIndex( dbcl, "a", {id : 1}, true );
   
   var expRecs = [];
   var cl = new Array();
   var coord  = new Array();
   for(var i = 0; i < coordNodes.length; i++)
   {
      coord[i] = new Sdb(coordNodes[i]);
      cl[i] = coord[i].getCS(COMMCSNAME).getCL(clName);
   }
   cl[1].insert({a : sortField});
   expRecs.push({a : sortField, id : -1});
   sortField++;
   
   //coordB指定自增字段值比MinValue稍大插入记录,此时coord上的所有缓存被丢弃
   var insertR1 = {a : sortField, id : { "$numberLong" : "-9223372036854775807" }};
   cl[1].insert(insertR1);
   expRecs.push(insertR1);
   sortField++;
   
   //coordB不指定自增字段再次插入记录，翻转获取缓存[-200,-1],唯一索引冲突，再次取缓存范围[-400,-201],自增字段生成值由401开始
   for(var i = 0; i < 20; i++)
   {
      cl[1].insert({a : sortField});
      expRecs.push({a : sortField, id : -201 + i*increment});
      sortField++;
   } 
   
   //coordA不指定自增字段插入
   for(var i = 0; i < 50; i++)
   {
      cl[0].insert({a : sortField});
      expRecs.push({a : sortField, id : -401 + i*increment});
      sortField++;
   } 
   
   //coordC不指定自增字段插入
   for(var i = 0; i < 50; i++)
   {
      cl[2].insert({a : sortField});
      expRecs.push({a : sortField, id : -601 + i*increment});
      sortField++;
   } 
   
   var rc = dbcl.find().sort({a:1});
   checkRec(rc, expRecs.sort(compare("a")));
   
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
