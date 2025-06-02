/* SPDX-License-Identifier: MIT */

#include "adt.h"
#include "aic.h"
#include "aic_regs.h"
#include "hv.h"
#include "uartproxy.h"
#include "smp.h"
#include "utils.h"
#include "string.h"

#define IRQTRACE_IRQ BIT(0)
#define PERCPU(x) pcpu[mrs(TPIDR_EL2)].x

#define MAX_CPUS     24
static u32 trace_hw_num[AIC_MAX_DIES][AIC_MAX_HW_NUM / 32];
extern struct hv_pcpu_data pcpu[MAX_CPUS];

static bool trace_aic_event(struct exc_info *ctx, u64 addr, u64 *val, bool write, int width)
{
    if (addr == (aic->base + aic->regs.event) && !write && width == 2)
    {
        u32 readout = 0;
        u64 daif = hv_aic_crit_start();
        int64_t cnt_pending_irq = PERCPU(total_pending_irqs)--;
        {
            if (cnt_pending_irq >= 0)
            {
                readout = PERCPU(pending_irq_readouts)[cnt_pending_irq];
                PERCPU(pending_irq_readouts)[cnt_pending_irq] = (u32) 0x0;
            }
            else if (cnt_pending_irq <= -1)
            {
                PERCPU(total_pending_irqs) = -1;
            }
        }        
        hv_aic_crit_end(daif);
        *val = readout;
        // printf("HV: CPU%d AIC event readout: 0x%x from %ld\n", smp_id(), readout, cnt_pending_irq);
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

    printf("Initialize AIC hook and per CPU state on CPU%d\n", smp_id());
    u64 daif = hv_aic_crit_start();
    memset(PERCPU(pending_irq_readouts), 0, sizeof(PERCPU(pending_irq_readouts)));
    PERCPU(total_pending_irqs) = -1;
    hv_aic_crit_end(daif);
    printf("CPU%d: DAIF 0x%lx\n", smp_id(), daif);
}

void hv_interrupt_set_irq_pending(void)
{
    u64 hcr = mrs(HCR_EL2);
    hv_write_hcr(hcr | HCR_VI);
    sysop("isb");
}

void hv_interrupt_clear_irq_pending(void)
{
    u64 hcr = mrs(HCR_EL2);
    if (hcr & HCR_VI)
    {
        hv_write_hcr(hcr & ~HCR_VI);
        sysop("isb");
    }
}

void hv_evaluate_pending_irqs(void)
{
    u64 daif = hv_aic_crit_start();
    {
        if (PERCPU(total_pending_irqs) >= 0)
        {
            hv_interrupt_set_irq_pending();
        }
        else
        {
            hv_interrupt_clear_irq_pending();
        }
    }
    hv_aic_crit_end(daif);
}

u64 hv_aic_crit_start(void)
{
    u64 daif = mrs(DAIF);

    /*u64 hcr = mrs(HCR_EL2);
    if (hcr & HCR_AMO) hcr &= ~HCR_AMO;
    if (hcr & HCR_IMO) hcr &= ~HCR_IMO;
    if (hcr & HCR_FMO) hcr &= ~HCR_FMO;
    hv_write_hcr(hcr);*/

    sysop("msr daifset, 0xf");
    sysop("isb");
    return daif;
}

void hv_aic_crit_end(u64 daif)
{
    /*u64 hcr = mrs(HCR_EL2);
    if (!(hcr & HCR_AMO)) hcr |= HCR_AMO;
    if (!(hcr & HCR_IMO)) hcr |= HCR_IMO;
    if (!(hcr & HCR_FMO)) hcr |= HCR_FMO;
    hv_write_hcr(hcr);*/

    msr(DAIF, daif);
    sysop("isb");
}

void hv_read_pending_irqs(void)
{
    u32 irq = 0;

    do
    {
        u32 irq = read32(aic->base + aic->regs.event);
        bool overflow = false;
        if (irq != 0)
        {
            u64 daif = hv_aic_crit_start();
            int64_t idx = ++PERCPU(total_pending_irqs);
            {
                if (PERCPU(total_pending_irqs) < MAX_ALLOWED_PENDING_INTERRUPTS)
                {
                    PERCPU(pending_irq_readouts)[idx] = irq;
                }
                else
                {
                    overflow = true;
                }
            }
            hv_aic_crit_end(daif);

            if (overflow)
            {
                // printf("Warning: on CPU%d there are too many pending IRQs, 0x%x discarded\n", smp_id(), irq);
                break;
            }
            else
            {
                // printf("CPU%d: IRQ 0x%x pending written to %ld\n", smp_id(), irq, idx);
            }
        }
    }
    while (irq != 0);
}
