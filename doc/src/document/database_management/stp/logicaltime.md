逻辑时间是 SequoiaDB 内部用于表示时间先后顺序但区别于实际机器时间的逻辑时间戳

- 本地逻辑时间（Local Logical Time, LLT）：每个时间节点维护自己的本地逻辑时间（单位：纳秒）
- 全局逻辑时间（Universal Logical Time, ULT）：定义 STP server 主节点上的本地逻辑时间为全局逻辑时间
- 逻辑时间容错误差（Logical Time Error）：
   - 表示系统可接受的真实逻辑时间的误差范围区间 `( LLT - LTError, LLT + LTError )`
   - 逻辑时间容错误差是由于时间同步、网络延迟等造成的
   - 各个 STP 节点通过不停与 STP server 主节点同步时间维持一个较小的逻辑时间容错误差
