/*******************************************************************************
*@Description : 匹配数组类型的记录（走索引、不走索引）
*@Modify List : 2014-02-03   Pusheng Ding   Init
                2016-03-17   Ting YU        Modify
*******************************************************************************/
main();

function main()
{	  
	try
	{	
      var csName = COMMCSNAME;
      var clName = COMMCLNAME;
      
      //create cl
      var clObj = new Collection( csName, clName, {ReplSize:0} );
      var cl = clObj.create();
      
      //insert records included array and other
      var recs = [ {a:[1,2,3],b:1}, {a:[],b:2}, {a:[1,3,2],b:3}, 
                   {a:1, b:4}, {a:{a1:1,a2:2},b:5} ];
      clObj.insertRecs( recs );
      
      //query without index
      println("---begin to exec: cl.find({a:[1,2,3]})");
      var rc = cl.find({a:[1,2,3]});
      var expRecs = [ recs[0] ];
      checkRec( rc, expRecs );
      
      println("---begin to exec: cl.find({a:[]})");
      var rc = cl.find({a:[]});
      var expRecs = [ recs[1] ];
      checkRec( rc, expRecs );
      
      println("---begin to exec: cl.find({a:1})");
      var rc = cl.find({a:1}).sort({b:1});
      var expRecs = [ recs[0], recs[2], recs[3] ];
      checkRec( rc, expRecs );
      
      println("---begin to exec: cl.find({a:{a1:1,a2:2}})");
      var rc = cl.find({a:{a1:1,a2:2}});
      var expRecs = [ recs[4] ];
      checkRec( rc, expRecs );
      
      //query with index
      var idxName = 'a';
      clObj.createIndex( idxName, {a:1} );
      
      println("---begin to exec: cl.find({a:[1,2,3]})");
      var rc = cl.find({a:[1,2,3]});
//      checkExplain( rc, idxName ); by tbscan
      var expRecs = [ recs[0] ];      
      checkRec( rc, expRecs );
      
      println("---begin to exec: cl.find({a:[]})");
      var rc = cl.find({a:[]});
//      checkExplain( rc, idxName );   //by tbscan
      var expRecs = [ recs[1] ];
      checkRec( rc, expRecs );
      
      println("---begin to exec: cl.find({a:1})");
      var rc = cl.find({a:1}).sort({b:1});
      checkExplain( rc, idxName );
      var expRecs = [ recs[0], recs[2], recs[3] ];
      checkRec( rc, expRecs );
      
      println("---begin to exec: cl.find({a:{a1:1,a2:2}})");
      var rc = cl.find({a:{a1:1,a2:2}});
      checkExplain( rc, idxName );
      var expRecs = [ recs[4] ];
      checkRec( rc, expRecs );
      
   }
   catch( e )
   {
      throw e ;
   }
}
