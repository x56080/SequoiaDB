// insert record.
// normal case.
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ) ;
}
catch(e)
{
   println( "unexpected err happened when clear cs:" + e ) ;
   throw e ;
}

try{
   var claSize = new RSize( COMMCSNAME );
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL(COMMCLNAME,{ReplSize:claSize.ReplSize(),
                                          Compressed:true});
}catch( e ){
   throw e ;
}

try
{
   varCL.insert([{"_id":"10000a",str:"abcz",integer:1000,boolean1:true, boolean2:false, nullobj:null,user:{id:0, name:"name"},array:[{name:"qiu",balance:1.2}, {name:"shang", balance:-1.2}]},{"_id":"120000a",no:1002,score:85,interest:["movie","photo"],major:"计算机软件与理论",dep:"计算机学院",info:{name:"Holiday",age:22,sex:">女"}},{"_id":"13000a","°′″＄￡￥‰％℃¤￠":"○一二三四五六七八九百千万亿兆吉太拍艾分厘毫微零壹贰叁肆伍陆柒捌玖佰仟"}]) ;
}
catch ( e )
{
   println( "failed to insert record, rc= " + e ) ;
   throw e ;
}

try
{
   if (varCL.find().count() != 3)
      throw "number error"; 
}
catch ( e )
{
   println( "failed to read record, rc= " + e ) ;
   throw e ;
}

try
{
var lobj1 = {"_id":"10000a",str:"abcz",integer:1000,boolean1:true, boolean2:false, nullobj:null,user:{id:0, name:"name"},array:[{name:"qiu",balance:1.2}, {name:"shang", balance:-1.2}]};
var lobj2 = {"_id":"120000a",no:1002,score:85,interest:["movie","photo"],major:"计算机软件与理论",dep:"计算机学院",info:{name:"Holiday",age:22,sex:">女"}};
var lobj3 = {"_id":"13000a","°′″＄￡￥‰％℃¤￠":"○一二三四五六七八九百千万亿兆吉太拍艾分厘毫微零壹贰叁肆伍陆柒捌玖佰仟"};
var cur = varCL.find({"_id":"10000a"});
if (!compareObj(lobj1, cur.next().toObj()))
   throw "lobj1 verify failed";
var cur = varCL.find({"_id":"120000a"});
if (!compareObj(lobj2, cur.next().toObj()))
   throw "lobj2 verify failed";
var cur = varCL.find({"_id":"13000a"});
if (!compareObj(lobj3, cur.next().toObj()))
   throw "lobj3 verify failed";
cur.close();
}
catch (e)
{
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the beginning" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}
