package com.sequoiadb.metaopr.commons;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Domain;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.task.OperateTask;
import org.bson.BSONObject;
import org.bson.util.JSON;

import java.util.List;
import java.util.logging.Logger;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-4-25
 * @Version 1.00
 */
public abstract class DBoperateTask extends OperateTask {
    int breakIndex;
    Sequoiadb db = null;
    private String hostname=null;

    private final static Logger log = Logger.getLogger(DBoperateTask.class.getName());

    public int getBreakIndex() {
        return breakIndex;
    }

    public void setHostname(String hostname) {
        this.hostname = hostname;
    }

    public String getHostname() {
        return hostname;
    }

    @Override
    public void exec() {
        if (hostname == null) {
            db = MyUtil.getMySdb().getSequoiadb();
        }else{
            db=new Sequoiadb(hostname,"","");
        }
        try {
            operate();
        } catch (InterruptedException e) {
            log.warning(e.toString());
        }
        db.close();
    }


    abstract void operate() throws InterruptedException;

    /**
     * @param clNames
     * @param csName
     * @param delayMillis 每循环创建一个cl的睡眠时间，单位是毫秒
     */
    public static DBoperateTask getTaskCreateCLInOneCs(final List<String> clNames, final String csName, final int delayMillis) {
        return new DBoperateTask() {
            @Override
            void operate() throws InterruptedException {
                CollectionSpace cs = db.getCollectionSpace(csName);
                for (int i = 0; i < clNames.size(); i++) {
                    Thread.sleep(delayMillis);
                    cs.createCollection(clNames.get(i));
                    breakIndex = i;
                }
            }
        };
    }

    public static DBoperateTask getTaskCreateCLInOneCs(final List<String> clNames, final String csName) {
        return getTaskCreateCLInOneCs(clNames, csName, 0);
    }

    /**
     * @param clNames
     * @param csName
     * @param delayMillis 每循环删除一个cl的睡眠时间，单位是毫秒
     */
    public static DBoperateTask getTaskDropCLInOneCs(final List<String> clNames, final String csName, final int delayMillis) {
        return new DBoperateTask() {
            @Override
            void operate() throws InterruptedException {
                CollectionSpace cs = db.getCollectionSpace(csName);
                for (int i = 0; i < clNames.size(); i++) {
                    Thread.sleep(delayMillis);
                    cs.dropCollection(clNames.get(i));
                    breakIndex = i;
                }
            }
        };
    }

    public static DBoperateTask getTaskDropCLInOneCs(List<String> clNames, String csName) {
        return getTaskDropCLInOneCs(clNames, csName, 0);
    }

    /**
     * 批量创建domain 指定两个数据组组
     *
     * @param domains
     * @param delayMillis
     * @return
     */
    public static DBoperateTask getTaskCreateDomains(final List<String> domains, final int delayMillis) {
        return new DBoperateTask() {
            @Override
            void operate() throws InterruptedException {
                List<String> groupNames = MyUtil.getDataGroupNames();
                String groupName1 = groupNames.get(0);
                String groupName2 = groupNames.get(1);
                BSONObject options = (BSONObject) JSON.parse("{'Groups':['" + groupName1 + "','" + groupName2 + "']}");

                for (int i = 0; i < domains.size(); i++) {
                    Thread.sleep(delayMillis);
                    db.createDomain(domains.get(i), options);
                    breakIndex = i;
                }
            }
        };
    }

    public static DBoperateTask getTaskCreateDomains(List<String> domains) {
        return getTaskCreateDomains(domains, 0);
    }

    public static DBoperateTask getTaskDropDomains(final List<String> domains, final int delayMillis) {
        return new DBoperateTask() {
            @Override
            void operate() throws InterruptedException {
                for (int i = 0; i < domains.size(); i++) {
                    Thread.sleep(delayMillis);
                    db.dropDomain(domains.get(i));
                    breakIndex = i;
                }
            }
        };
    }

    public static DBoperateTask getTaskDropDomains(final List<String> domains) {
        return getTaskDropDomains(domains, 0);
    }

    public static DBoperateTask getTaskDropCs(final List<String> csNames, final int delayMillis) {
        return new DBoperateTask() {
            @Override
            void operate() throws InterruptedException {
                for (int i = 0; i < csNames.size(); i++) {
                    Thread.sleep(delayMillis);
                    db.dropCollectionSpace(csNames.get(i));
                    breakIndex = i;
                }
            }
        };
    }


    public static DBoperateTask getTaskDropCs(final List<String> csNames) {
        return getTaskDropCs(csNames, 0);
    }

    public static DBoperateTask getTaskCreateCs(final List<String> csNames, final int delayMillis) {
        return new DBoperateTask() {
            @Override
            void operate() throws InterruptedException {
                for (int i = 0; i < csNames.size(); i++) {
                    Thread.sleep(delayMillis);
                    db.createCollectionSpace(csNames.get(i));
                    breakIndex = i;
                }
            }
        };
    }

    public static DBoperateTask getTaskCreateCs(final List<String> csNames) {
        return getTaskCreateCs(csNames, 0);
    }


    /**
     * 批量修改domain属性
     *
     * @param domainName
     * @param num        修改次数
     * @param groupNames
     * @return
     */
    public static DBoperateTask getTaskAlterDomain(final String domainName, final int num, final List<String> groupNames) {
        return new DBoperateTask() {
            @Override
            void operate() throws InterruptedException {
                Domain domain = db.getDomain(domainName);
                String groupName1 = groupNames.get(0);
                String groupName2 = groupNames.get(1);
                String groupName3 = groupNames.get(2);
                for (int i = 0; i < num; i++) {
                    if (i % 3 == 0)
                        MyUtil.alterDomain(domain, groupName1, groupName2);
                    if (i % 3 == 1)
                        MyUtil.alterDomain(domain, groupName1, groupName3);
                    else
                        MyUtil.alterDomain(domain, groupName2, groupName3);
                    breakIndex = i;
                }
            }
        };
    }
}
