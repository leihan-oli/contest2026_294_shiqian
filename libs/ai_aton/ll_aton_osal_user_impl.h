/****************************************************************************
 * libs/ai_aton/ll_aton_osal_user_impl.h
 *
 * NuttX OSAL glue for the ST Neural-ART (ATON) runtime.
 *
 * The middleware is told to use a custom OSAL by defining
 * LL_ATON_OSAL=LL_ATON_OSAL_USER_IMPL on the command line; its
 * ll_aton_osal.h then includes this header (branch
 * "LL_ATON_OSAL_USER_IMPL").  The hook set is the same one the shipped
 * FreeRTOS/ThreadX ports implement, plus the hooks whose default
 * implementations are CMSIS/NVIC based (ENABLE_IRQ, DISABLE_IRQ,
 * ENTER_CS, EXIT_CS, DSB, SET_PRIORITY) -- those CMSIS symbols are not
 * available in a NuttX build, so they must be provided here as well.
 *
 * The implementation lives in ll_aton_osal_nuttx.c.
 ****************************************************************************/

#ifndef __LL_ATON_OSAL_USER_IMPL_H
#define __LL_ATON_OSAL_USER_IMPL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include <arch/irq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Preconditions before ll_aton_osal.h uses the macros below: the caller has
 * to know which NVIC line the ATON runtime uses.  ATON_STD_IRQ_LINE is
 * defined by the middleware (ll_aton_platform.h, default 0).
 */

#ifndef ATON_STD_IRQ_LINE
#  define ATON_STD_IRQ_LINE 0
#endif

/* STM32N6 NPU interrupt lines: NPU0..NPU3 = NVIC 53..56
 * (arch/arm/include/stm32n6/irq.h).  ATON line n maps to NPU<n>.
 */

#define ATON_OSAL_NUTTX_IRQ(line) \
  (STM32_IRQ_NPU0 + (line))

/* (De-)initialization of the OSAL layer */

#define LL_ATON_OSAL_INIT()         aton_osal_nuttx_init()
#define LL_ATON_OSAL_DEINIT()       aton_osal_nuttx_deinit()

/* Wait for / signal the NPU "epoch complete" event.  SIGNAL_EVENT is called
 * from the NPU interrupt handler, so the implementation posts a semaphore.
 */

#define LL_ATON_OSAL_WFE()          aton_osal_nuttx_wfe()
#define LL_ATON_OSAL_SIGNAL_EVENT() aton_osal_nuttx_signal_event()

/* Locks: ATON serializes epochs with a priority-inheriting mutex, the cache
 * locks are plain mutexes shared by the MCU- and NPU-cache maintenance
 * paths.
 */

#define LL_ATON_LOCK_ATON()         aton_osal_nuttx_lock()
#define LL_ATON_UNLOCK_ATON()       aton_osal_nuttx_unlock()

#define LL_ATON_LOCK_NPU_CACHE()    aton_osal_nuttx_cache_lock()
#define LL_ATON_UNLOCK_NPU_CACHE()  aton_osal_nuttx_cache_unlock()

#define LL_ATON_LOCK_MCU_CACHE()    aton_osal_nuttx_cache_lock()
#define LL_ATON_UNLOCK_MCU_CACHE()  aton_osal_nuttx_cache_unlock()

/* Critical section of the ATON runtime task: the ATON interrupt line is
 * masked (ll_aton_osal.h: "here we need to block also execution of IRQ
 * handler").
 */

#define LL_ATON_OSAL_ENTER_CS()     aton_osal_nuttx_enter_cs()
#define LL_ATON_OSAL_EXIT_CS()      aton_osal_nuttx_exit_cs()

/* Data synchronization barrier */

#define LL_ATON_OSAL_DSB()          aton_osal_nuttx_dsb()

/* NPU interrupt line handling.
 *
 * The ATON runtime installs its own handler from LL_ATON_RT_RuntimeInit()
 * (ll_aton_runtime.c):
 *
 *   LL_ATON_OSAL_INSTALL_IRQ(ATON_STD_IRQ_LINE, ATON_STD_IRQHandler);
 *   LL_ATON_OSAL_ENABLE_IRQ(ATON_STD_IRQ_LINE);
 *
 * where ATON_STD_IRQHandler resolves to CDNN0_IRQHandler -> NPU0_IRQHandler
 * (ll_aton_platform.h).  On an RTOS without a CMSIS vector table that entry
 * has to be wired up by the port, which is what these two hooks do: the
 * handler is attached to the NuttX IRQ for line n through a small adapter
 * (the ATON handler takes no arguments, a NuttX ISR does).  The handler
 * then clears the ATON interrupt controller status and calls
 * LL_ATON_OSAL_SIGNAL_EVENT() to wake up LL_ATON_OSAL_WFE().
 */

#define LL_ATON_OSAL_INSTALL_IRQ(irq_aton_line_nr, handler) \
  aton_osal_nuttx_install_irq(irq_aton_line_nr, handler)
#define LL_ATON_OSAL_REMOVE_IRQ(irq_aton_line_nr) \
  aton_osal_nuttx_remove_irq(irq_aton_line_nr)

#define LL_ATON_OSAL_ENABLE_IRQ(irq_aton_line_nr) \
  aton_osal_nuttx_enable_irq(irq_aton_line_nr, true)
#define LL_ATON_OSAL_DISABLE_IRQ(irq_aton_line_nr) \
  aton_osal_nuttx_enable_irq(irq_aton_line_nr, false)

#define LL_ATON_OSAL_SET_PRIORITY(irq_aton_line_nr, prio) \
  aton_osal_nuttx_set_priority(irq_aton_line_nr, prio)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

void aton_osal_nuttx_init(void);
void aton_osal_nuttx_deinit(void);
void aton_osal_nuttx_wfe(void);
void aton_osal_nuttx_signal_event(void);
void aton_osal_nuttx_lock(void);
void aton_osal_nuttx_unlock(void);
void aton_osal_nuttx_cache_lock(void);
void aton_osal_nuttx_cache_unlock(void);
void aton_osal_nuttx_set_verbose(bool verbose);
void aton_osal_nuttx_enter_cs(void);
void aton_osal_nuttx_exit_cs(void);
void aton_osal_nuttx_dsb(void);
void aton_osal_nuttx_enable_irq(int line, bool enable);
void aton_osal_nuttx_set_priority(int line, int prio);
void aton_osal_nuttx_install_irq(int line, void (*handler)(void));
void aton_osal_nuttx_remove_irq(int line);
uint32_t aton_osal_nuttx_event_count(void);
uint32_t aton_osal_nuttx_irq_count(void);
bool aton_osal_nuttx_storm(void);
int aton_osal_nuttx_stage(void);
void aton_osal_nuttx_report(FAR const char *tag);

/* ISR-safe logging.  The ATON runtime reports epoch errors from interrupt
 * context; printf() is illegal there on NuttX (the console takes a mutex), so
 * the messages go into a lock-free ring that is dumped from task context.
 */

void aton_osal_nuttx_log(const char *fmt, ...);
void aton_osal_nuttx_log_puts(const char *str);
void aton_osal_nuttx_log_dump(void);



#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __LL_ATON_OSAL_USER_IMPL_H */
