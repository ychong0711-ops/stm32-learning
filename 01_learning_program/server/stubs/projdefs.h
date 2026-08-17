#ifndef PROJDEFS_H
#define PROJDEFS_H
#define pdFALSE ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)
#define pdPASS  pdTRUE
#define pdFAIL  pdFALSE
#ifndef configASSERT
#define configASSERT(x) do { if (!(x)) { for(;;){} } } while (0)
#endif
#define portYIELD_FROM_ISR(x) do { (void)(x); } while (0)
#define taskDISABLE_INTERRUPTS() do { } while (0)
#endif
