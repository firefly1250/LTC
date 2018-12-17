#ifndef TASK_NOTIFICATION_H_
#define TASK_NOTIFICATION_H_

#include <Arduino.h>

void TaskNotify(TaskHandle_t task_handle){
	xTaskNotify(task_handle,0,eNoAction);
}

void TaskNotifyWait(){
    xTaskNotifyWait(0x00,0xffffffff,nullptr,portMAX_DELAY );
}

#endif