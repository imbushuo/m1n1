/* SPDX-License-Identifier: MIT */

#include "adt.h"
#include "aic.h"
#include "aic_regs.h"
#include "hv.h"
#include "uartproxy.h"
#include "smp.h"
#include "utils.h"

#define IRQTRACE_IRQ BIT(0)
#define PERCPU(x) pcpu[mrs(TPIDR_EL2)].x

#define MAX_CPUS     24
static u32 trace_hw_num[AIC_MAX_DIES][AIC_MAX_HW_NUM / 32];
extern struct hv_pcpu_data pcpu[MAX_CPUS];

static bool trace_aic_event(struct exc_info *ctx, u64 addr, u64 *val, bool write, int width)
{
    if (addr == (aic->base + aic->regs.event) && !write)
    {
        if (PERCPU(irq_fired))
        {
            *val = PERCPU(irq_reason);
            u64 hcr = mrs(HCR_EL2);
            hv_write_hcr(hcr & ~HCR_VI);
            PERCPU(irq_fired) = false;
        }

        return true;
    }

    if (!hv_pa_rw(ctx, addr, val, write, width))
        return false;

    if (addr != (aic->base + aic->regs.event) || write || width != 2) {
        return true;
    }

    return true;
}

bool hv_trace_irq(u32 type, u32 num, u32 count, u32 flags)
{
    dprintf("HV: hv_trace_irq type: %u start: %u num: %u flags: 0x%x\n", type, num, count, flags);
    if (type == AIC_EVENT_TYPE_HW) {
        u32 die = num / aic->max_irq;
        num %= AIC_MAX_HW_NUM;
        if (die >= aic->max_irq || num >= AIC_MAX_HW_NUM || count > AIC_MAX_HW_NUM - num) {
            printf("HV: invalid IRQ range: (%u, %u) for die %u\n", num, num + count, die);
            return false;
        }
        for (u32 n = num; n < num + count; n++) {
            switch (flags) {
                case IRQTRACE_IRQ:
                    trace_hw_num[die][n / 32] |= BIT(n & 31);
                    break;
                default:
                    trace_hw_num[die][n / 32] &= ~(BIT(n & 31));
                    break;
            }
        }
    } else {
        printf("HV: not handling AIC event type: 0x%02x num: %u\n", type, num);
        return false;
    }

    if (!aic) {
        printf("HV: AIC not initialized\n");
        return false;
    }

    static bool hooked = false;

    if (aic && !hooked) {
        hv_map_hook(aic->base, trace_aic_event, aic->regs.reg_size);
        hooked = true;
    }

    return true;
}

void hv_hook_aic(void)
{
    static bool hooked = false;

    if (aic && !hooked) {
        hv_map_hook(aic->base, trace_aic_event, aic->regs.reg_size);
        hooked = true;
    }
}
