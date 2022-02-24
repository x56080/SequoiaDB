/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.flink.sink.writer;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.flink.client.SDBClient;
import com.sequoiadb.flink.client.SDBSinkClient;
import com.sequoiadb.flink.codec.SDBDataConverter;
import com.sequoiadb.flink.config.SDBSinkOptions;
import com.sequoiadb.flink.exception.SDBException;
import com.sequoiadb.flink.sink.Executor.WriteThreads;
import com.sequoiadb.flink.sink.state.SDBBulk;

import org.apache.flink.api.connector.sink.Sink.InitContext;
import org.apache.flink.api.connector.sink.SinkWriter;
import org.apache.flink.table.data.RowData;
import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SDBSinkWriter<IN> implements SinkWriter<IN, SDBBulk, SDBBulk> {

    private InitContext context;
    private SDBSinkOptions sdbSinkOptions;

    private SDBBulk currentBulk;
    private List<SDBBulk> pendingBulks = new ArrayList<>();
    private int bulkMaxSize;
    private long currentBulkTTL;
    private long currentBulkSpawnTime;
    
    private boolean ignoreNullField = false;
    private final Boolean transactionOn;
    private final Boolean indempotent;
    
    private final int THREAD_NUMBER;
    private final SDBClient client;
    private final ExecutorService executorService;
    
    private final String SDB_BSON_OID = "_id";
    private final SDBDataConverter dataConverter;
    private static final Logger LOG = LoggerFactory.getLogger(SDBSinkWriter.class);
    
      /*
     * constructor of SDBSinkWriter
     * @param sdbSinkOptions        pass down user options
     * @param dataConverter         converter than convert Rowdata to BSON
     * @param context               InitConext for sinkwriter
     * @param states                data from last checkpoint, for retry
     */
    public SDBSinkWriter(SDBSinkOptions sdbSinkOptions, SDBDataConverter dataConverter, InitContext context,
                         List<SDBBulk> states) {
        this.sdbSinkOptions = sdbSinkOptions;
        this.dataConverter = dataConverter;
        this.context = context;
        this.bulkMaxSize = sdbSinkOptions.getBulkSize();
        this.currentBulk = new SDBBulk(this.bulkMaxSize);
        //adding last checkpoint data to pending bulks, so it will be sent next checkpoint
        this.pendingBulks.addAll(states);
        this.ignoreNullField = sdbSinkOptions.getIgnoreNullField();
        // timer here for two things: business allowed latency and lingering daata
        this.currentBulkSpawnTime = getCurrentTimeSeconds();
        this.currentBulkTTL = sdbSinkOptions.getMaxBulkFillTime();
        this.THREAD_NUMBER = sdbSinkOptions.getHosts().size();
        this.transactionOn = sdbSinkOptions.getTransactionOn();
        this.indempotent = sdbSinkOptions.getIndempotent();

        // select one coord node for this writer to use, when write transaction with unique index or nontransaction
        List<String> hosts = sdbSinkOptions.getHosts();
        String host = sdbSinkOptions.getHosts().get(context.getSubtaskId() % hosts.size());
        List<String> currentHost = new ArrayList<>();
        currentHost.add(host);
        
        // create client, this client wont connect to SDB until its first insert
        this.client = SDBSinkClient.createClientWithHost(sdbSinkOptions, currentHost);

        // create thread pool for write out, when it is transaction and don't have unique index
        executorService = Executors.newFixedThreadPool(THREAD_NUMBER); 

        //log info
        LOG.info("Sink Writer {} started", context.getSubtaskId());

    }

    /*
     * called when closing a sink writer
     */
    @Override
    public void close() throws Exception {
        executorService.shutdown();
        client.close();
        LOG.info("Sink Writer {} Closed ", context.getSubtaskId());
    }

    /*
     * write function write out data
     * here we are not write everytime a rowdata is arrived
     * insert with minibatch is more effcient
     * @param element           incomming data
     * @param context           contains infos that useful for watermarks
     */
    @Override
    public void write(IN element, Context context) throws IOException, InterruptedException {
        // here the context is InitCotext from constructer
        LOG.debug("Sink Writer write {}" , this.context.getSubtaskId());
        
        // convert data from Rowdata to Bson
        BSONObject bsonObject = dataConverter.toExternal((RowData) element, ignoreNullField);
        //if dont have unique index, create one 
        if (transactionOn && !indempotent) {
            bsonObject.put(SDB_BSON_OID, ObjectId.get());
        }

        // Write data out

        /* For transaction and dont have unique index
         * When incomming data don't have unique index, we created for them,
         * to make sure indempotent insert, we need to keep it in the state and checkpoint first
         * so when it don't have unique index, we write them out in committer
         */
        if (checkTransactionWriteOutCondition() 
            && !indempotent) {
            pushToPendingBulks();

        } // For transaction and have unique index 
        else if (checkTransactionWriteOutCondition() 
            && indempotent){
                dataInsert(transactionOn);
        } // For non transaction when error, it requres user to stop and clear tables from sdb and restart flink task
        else if (!transactionOn && checkPushToPendingCondition()) {
            dataInsert(transactionOn);
        }
      
        currentBulk.add(bsonObject);
    }
    /*
     * return true if a transaction sink is about to write
     * @return Boolean
     */
    private Boolean checkTransactionWriteOutCondition(){
        return transactionOn && checkPushToPendingCondition();
    }
    /*
     * return true currentBulk is full or timer is up
     * @return Boolean
     */
    private Boolean checkPushToPendingCondition(){
        long currentTime = getCurrentTimeSeconds();
        return currentBulk.isFull() || currentTime - currentBulkSpawnTime >= currentBulkTTL;
    }

    /*  
        prepareCommit function is called by sink operator when creating a checkpoint.
        prepareCommit comes with a boolean value flush, this will be triggered
        if there is no more data streaming in, for example end of a stream or bounded
        data. The reason for this boolean value is that, currently Flink is not able
        to `commit` at the end of process, the last batch of data will never committed
        a bandit solution here is when reach end of input, flush is set to true, do a
        commit at prepareCommit, since committer will be closed and committer.commit will
        never called.
        Flink issue #: FLINK-23883 and FLINK-2491 current plan for resolve this issue: 1.15
        @param flush            set to true when it is to the end of bounded data
        returns List<SDBBulk>   List of SDBBulks that is about to be committed
    */
    @Override
    public List<SDBBulk> prepareCommit(boolean flush) throws IOException, InterruptedException {
        LOG.info("Sink writer {} prepareCommit" , context.getSubtaskId());
        // when transaction and no unique index, and need to be flushed
        // since it is aready end of data (flush set) we need to flush manually
        if (transactionOn && !indempotent && flush) {
            pushToPendingBulks();
            flush();    
        } // when transaction and no unique index, times up it will be pushed to pendingbulks
          // committer will flush them out
        else if (transactionOn && !indempotent && checkPushToPendingCondition()) {
            pushToPendingBulks();
        } // when non transaction, this is to ensure lingering data get flushed at least every checkpoint.
        else if (!transactionOn && (flush || checkPushToPendingCondition())){
            if (currentBulk.size() > 0) {
                dataInsert(transactionOn);
            }
        } // when transaction with unique index, same reason as before, 
          // this will ensure lingering data get flushed as lease every checkpoint
        else if (transactionOn && indempotent && (flush || checkPushToPendingCondition())) {
            if (currentBulk.size() > 0) {
                dataInsert(transactionOn);
            }
        } 
        // otherwise, just a normal checkpoint
        // for non transaction and transaction with unique index, it will send an empty pendingbulks to committer
        
        return pendingBulks;
    }

    /*
     * write data to sdb
     * @param transaction           boolean to show if the data is sent out with unique index or not
     */
    private void dataInsert(boolean transaction){
        try{
            if (transaction){
                client.getCL().insert(currentBulk.getBsonObjects(), DBCollection.FLG_INSERT_CONTONDUP);
            } else {
                client.getCL().insert(currentBulk.getBsonObjects());
            }
        } catch (BaseException ex) {
            // when insert without transaction, error message is requred
            // help user retry
            LOG.error("Insert failed on CS: {}, CL: {}",
            sdbSinkOptions.getCollectionSpace(),
            sdbSinkOptions.getCollection());
            client.close();
            throw ex;
        }
        currentBulk.clear();
        currentBulkSpawnTime = getCurrentTimeSeconds();

    }

    /*
     * snapshotState is called to prepare snapshot for checkpoint.
     * it will return current sinkwriter state to sink operator.
     * this state will be saved to checkpoint
     * @param checkpointId          given checkpoint id, it is not used
     * @return List<SDBBulk>        current sinkwriter state
     */
    @Override
    public List<SDBBulk> snapshotState(long checkpointId) throws IOException {
        LOG.info("Sink Writer {} snapshotState" , context.getSubtaskId());
        ArrayList<SDBBulk> state = new ArrayList<>();        
        state.add(currentBulk);
        state.addAll(pendingBulks);
        pendingBulks.clear();
        return state;
    }

    /*
     * flush function to flush last patch of data to SDB
     * it uses same logic as committer multi-threaded insert
     */
    private void flush() throws InterruptedException {
        LOG.debug("Sink Writer {} flush" , context.getSubtaskId());
        int numOfBulks = pendingBulks.size();
        int numThread = 1 * THREAD_NUMBER;
        int numOfLatch = numOfBulks < THREAD_NUMBER ? numOfBulks : THREAD_NUMBER; 
        int dividedBulkListSize = numOfBulks < numThread ? numOfBulks : numOfBulks / numThread;

        CountDownLatch latch = new CountDownLatch(numOfLatch);   
        List<Future<?>> threadStatus = new ArrayList<>();
        for (int i = 0; i < numThread; i++) {
            int start = 0 + dividedBulkListSize * i;
            int end = dividedBulkListSize + dividedBulkListSize * i;
            end = end > numOfBulks ? numOfBulks : end;
            List<SDBBulk> sublist =  pendingBulks.subList(start, end);
            List<String> host = new ArrayList<String>();
            host.add(sdbSinkOptions.getHosts().get(i % THREAD_NUMBER));
            SDBClient client = SDBSinkClient.createClientWithHost(sdbSinkOptions, host);
            WriteThreads thread = new WriteThreads(client, sublist, latch);
            /*
             * Here added a future. the reason is if using .execute, it will throw a unexcepted exception by default
             * flink will simply log it and ignore, we want it to be catched and handled by flink so instead of throw
             * unexcepted exception, we use future to create a SDB exception this will catched and handled by flink and
             * trigger retry
             */
            try {
               Future<?>submitted =  executorService.submit(thread);
               threadStatus.add(submitted);
            } catch (Exception e) {
                throw new SDBException("Thread exception:", e);
            }
        }
        latch.await();
        // catch exception here
        try {
            for (Future<?> submitted: threadStatus){
                submitted.get();
            }
        }catch (Exception e) {
            throw new SDBException("Thread exceptions", e);
        }
    }

    /*
     * push currentbulk to pendingbulks
     * it happends when currentBulk is full 
     * or timer is up 
     * when transaction write 
     */
    private void pushToPendingBulks() {
        pendingBulks.add(currentBulk);
        currentBulk = new SDBBulk(bulkMaxSize);
        currentBulkSpawnTime = getCurrentTimeSeconds();
    }

    private long getCurrentTimeSeconds(){
        return System.currentTimeMillis() / 1000;
    }
}
