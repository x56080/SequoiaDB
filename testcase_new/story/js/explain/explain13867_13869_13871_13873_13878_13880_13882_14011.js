/************************************
*@Description: seqDB-13867:ÇÐ·Ö±íÊ¹ÓÃSearch²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13869:ÇÐ·Ö±íÊ¹ÓÃEvaluate²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13871:ÇÐ·Ö±íÊ¹ÓÃEstimate²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13873:ÇÐ·Ö±íÊ¹ÓÃExpand²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13878:ÇÐ·Ö±íÊ¹ÓÃFilter²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13880:ÇÐ·Ö±íÊ¹ÓÃDetail²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13882:ÇÐ·Ö±íÊ¹ÓÃRun²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-14011:ÇÐ·Ö±íÊ¹ÓÃFlatten²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
*@author:      zhaoyu
*@createdate:  2019.7.13
*@testlinkCase: seqDB-13867
**************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "------Deploy is standalone" );
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      println( "Deploy is only one group!" );
      return;
   }

   var configPath = "./config.txt";
   var clName = COMMCLNAME + "13867";
   commDropCL( db, COMMCSNAME, clName, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 }, AutoSplit: true } );

   var doc = [];
   for( var i = 0; i < 30000; i++ )
   {
      doc.push( { a: i, b: i, c: i, d: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" + i } )
   }
   dbcl.insert( doc );

   //¶ÁÈ¡ÅäÖÃÎÄ¼þconfig.txtÖÐµÄ²ÎÊý£¬½øÐÐ·ÃÎÊ¼Æ»®Õ¹Ê¾
   var file = new File( configPath );
   while( true )
   {
      try
      {
         var explainObj = JSON.parse( file.readLine().split( "\n" )[0] );
         var explainCursor = dbcl.find( { a: { $in: [1, 10000] } } ).explain( explainObj );
         while( explainCursor.next() ) { };

      }
      catch( e )
      {
         if( e === -9 )
         {
            break;
         }
         else
         {
            throw e;
         }
      }
   }

   commDropCL( db, COMMCSNAME, clName, true );
}
main(); 
