// GPIO ISR for the rotary encoder and buttons. Posts InputEvent to inputQueue.
#pragma once

// Call after create_app_queues() and before vTaskStartScheduler().
void input_isr_init();
