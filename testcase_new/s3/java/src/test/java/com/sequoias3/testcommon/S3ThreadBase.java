package com.sequoias3.testcommon;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.lang.Thread.State;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

public abstract class S3ThreadBase implements Runnable {
    private static final int MAX_THREAD_NUMBER = 100;
    private static ExecutorService service = Executors
            .newFixedThreadPool( MAX_THREAD_NUMBER );
    private List< Throwable > exceptionList = Collections
            .synchronizedList( new ArrayList< Throwable >() );
    private Integer syncRes = new Integer( 0 );
    private Integer syncRunning = new Integer( 0 );
    private Object result = null;
    private AtomicInteger count = new AtomicInteger( 0 );
    private Thread thread = null;

    public static void shutdown() {
        service.shutdown();
    }

    public static void main( String[] args ) {
        Thread t = Thread.currentThread();
        Thread t1 = t;
        t = null;
        t1.getStackTrace();

        S3ThreadBase base = new S3ThreadBase() {

            @Override
            public void exec() throws Exception {
                // TODO Auto-generated method stub
                Thread.sleep( 5000 );
            }

        };

        base.start();
        base.matchBlockingMethod( base.getClass().getName(), "exec" );

        try {
            Thread.sleep( 10000 );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        base.matchBlockingMethod( base.getClass().getName(), "exec" );

    }

    public void start() {
        start( 1 );

    }

    public void start( int threadNum ) {
        count.set( threadNum );
        synchronized ( service ) {
            for ( int i = 0; i < threadNum; i++ ) {
                service.execute( this );
            }
        }
    }

    /*
     * -------------------------------------------------------------------------
     * - getExecResult -- 获取线程的执行结果，只适应启动一个线程的情况 必须在线程结束前通过setExecResult进行设置
     * 如果线程没有开始或者已经结束调用直接返回，否则会阻塞到结果被设置 Parameters: Returns:
     * 结果对象实例，如果是一个DBCursor，返回后通过 Object ret = getExecResult() ; if ( ret
     * instanceof DBCursor){ DBCursor cursor = (DBCursor)ret; }
     * -------------------------------------------------------------------------
     * -
     */
    public Object getExecResult() throws InterruptedException {
        if ( thread == null || thread.getState() == State.NEW
                || thread.getState() == State.TERMINATED ) {
            return this.result;
        }

        synchronized ( syncRes ) {
            syncRes.wait();
        }
        return this.result;
    }

    /*
     * -------------------------------------------------------------------------
     * - setExecResult -- 设置线程的执行结果，只适合启单个线程的情况 Parameters: result: Object
     * 可以是任意对象类型 Returns: void
     * -------------------------------------------------------------------------
     * -
     */
    public void setExecResult( Object result ) {
        assert thread != null;
        this.result = result;
        synchronized ( syncRes ) {
            syncRes.notifyAll();
        }
    }

    // 返回结果集
    public List< Throwable > getExceptions() {
        join();
        return exceptionList;
    }

    public String getErrorMsg() {
        join();
        StringBuilder buffer = new StringBuilder();
        for ( Throwable exception : exceptionList ) {
            buffer.append( getErrorMsg( exception ) );
        }
        return buffer.toString();
    }

    private String getErrorMsg( Throwable e ) {
        if ( e == null )
            return "";
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        PrintStream printStream = new PrintStream( bytes );
        printStream.println();
        printStream.println( "------  err msg start: " );
        e.printStackTrace( printStream );
        printStream.println( "------  err msg end." );
        printStream.flush();
        return bytes.toString();
    }

    // join所有线程
    public void join() {
        synchronized ( this ) {
            try {
                if ( count.get() != 0 ) {
                    this.wait();
                }
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    public boolean isSuccess() {
        join();
        if ( exceptionList.size() != 0 ) {
            return false;
        }
        return true;
    }

    public void run() {
        try {
            if ( count.get() == 1 ) {
                thread = Thread.currentThread();
                synchronized ( syncRunning ) {
                    syncRunning.notifyAll();
                }
            }
            exec();
        } catch ( Throwable e ) {
            exceptionList.add( e );
        } finally {
            if ( 0 == count.decrementAndGet() ) {
                synchronized ( this ) {
                    this.notify();
                    thread = null;
                }
            }
        }
    }

    /*
     * -------------------------------------------------------------------------
     * - matchBlockingMethod -- 当前线程是否阻塞在相应的调用上 Parameters: className: 类名
     * (DBCollection.class.getName()) methodName: 方法名(query ...) Returns:
     * 如果当前线程执行CL.update()阻塞 matchBlockingMethod(cl.getClass().getName(),
     * "update")则返回true 如果当前线程执行CL.query()阻塞，则返回true
     * matchBlockingMethod(cl.getClass().getName(), "query")则返回true 否则返回false
     * -------------------------------------------------------------------------
     * -
     */
    public boolean matchBlockingMethod( String className, String methodName ) {
        if ( thread == null ) {
            synchronized ( syncRunning ) {
                try {
                    syncRunning.wait( 1000 );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }

        final int fiftySeonds = 50000;
        final int totalTimes = 3;
        // int nonMatchTimes = 0 ;
        int matchTimes = 0;
        int alreadyWaitTime = 0;
        boolean ret = true;

        int pos = 0;
        do {
            Thread traceThread = null;
            synchronized ( this ) {
                traceThread = thread;
            }
            if ( traceThread == null ) {
                ret = false;
                break;
            }

            if ( alreadyWaitTime >= fiftySeonds ) {
                ret = false;
                break;
            }

            if ( traceThread.getState() == State.TERMINATED ) {
                ret = false;
                break;
            }

            try {
                Thread.sleep( 5 );
                alreadyWaitTime += 5;
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }

            if ( traceThread.getState() == State.NEW ) {
                continue;
            }

            StackTraceElement[] stackElem = traceThread.getStackTrace();
            if ( pos != 0 ) {
                stackElem = traceThread.getStackTrace();
                if ( stackElem.length == 0 || stackElem.length <= pos ) {
                    ret = false;
                    break;
                }

                if ( stackElem[ pos ].getClassName().equals( className )
                        && stackElem[ pos ].getMethodName()
                                .equals( methodName ) ) {
                    ++matchTimes;
                }
            } else {
                for ( pos = 0; pos < stackElem.length; ++pos ) {
                    if ( stackElem[ pos ].getClassName().equals( className )
                            && stackElem[ pos ].getMethodName()
                                    .equals( methodName ) ) {
                        ++matchTimes;
                        break;
                    }
                }

                if ( pos == stackElem.length ) {
                    // nonMatchTimes++;
                    pos = 0;
                }
            }

            if ( matchTimes >= totalTimes ) {
                break;
            }

        } while ( true );

        return ret;
    }

    public abstract void exec() throws Exception;
}
