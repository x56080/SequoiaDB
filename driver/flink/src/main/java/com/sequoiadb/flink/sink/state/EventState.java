package com.sequoiadb.flink.sink.state;

import org.apache.flink.table.data.TimestampData;

public class EventState {

    private final TimestampData eventTime;
    private TimestampData processingTime;

    public EventState(TimestampData eventTime, TimestampData processingTime) {
        this.eventTime = eventTime;
        this.processingTime = processingTime;
    }

    public TimestampData getEventTime() {
        return eventTime;
    }

    public TimestampData getProcessingTime() {
        return processingTime;
    }

    public void setProcessingTime(TimestampData processingTime) {
        this.processingTime = processingTime;
    }

}
