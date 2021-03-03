##语法##
***db.collectionspace.collection.splitAsync(\<source group\>, \<target group\>, \<percent\>, [options] )***  
***db.collectionspace.collection.splitAsync(\<source group\>, \<target group\>, \<condition\>, [endcondition], [options] )***

该操作与 [split()](reference/Sequoiadb_command/SdbCollection/split.md) 功能相同，但该操作为异步分区操作，分区任务建立后立即返回任务 ID。
