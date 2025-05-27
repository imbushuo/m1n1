/* SPDX-License-Identifier: MIT */

#include "hv.h"
#include "assert.h"
#include "cpu_regs.h"
#include "exception.h"
#include "smp.h"
#include "string.h"
#include "uart.h"
#include "uartproxy.h"
#include "hv_gicv2.h"

// GICv2 global state.
static HV_GICV2_STATE g_GicV2State;

static bool hv_vgicv2_handle_gicd_mmio(struct exc_info *ctx, u64 addr, u64 *val, bool write, int width)
{
    UNUSED(ctx);
    UNUSED(addr);
    UNUSED(val);
    UNUSED(write);
    UNUSED(width);

    return true;
}

static bool hv_vgicv2_handle_gicc_mmio(struct exc_info *ctx, u64 addr, u64 *val, bool write, int width)
{
    UNUSED(ctx);
    UNUSED(addr);
    UNUSED(val);
    UNUSED(write);
    UNUSED(width);

    return true;
}

// Handles inbound FIQ notification regarding timer and convert them to PPI timer interrupt.
void hv_vgicv2_notify_timer_interrupt(int cpu, GICV2_PPI_TIMER_TYPE type, bool set)
{
    // We don't care unset right now
    if (!set) return;

    // Make sure we don't get out of bound
    assert(cpu < NR_GICV2_MAX_CPUS);

    // Hardwired to virt right now
    UNUSED(type);
    int irq = NR_INTC_ARCH_TIMER_NONSECURE_EL1_VIRTUAL;
    irq -= NR_PPI_BEGIN;

    if (!(g_GicV2State.CpuState[cpu].IntcPpiEnabled & BIT(irq)))
    {
        if (!g_GicV2State.CpuState[cpu].DbgVirtSpuriousMarked)
        {
            printf("hv_vgicv2_notify_timer_interrupt: CPU%d: Arch Timer interrupt delivery but not yet enabled\n", cpu);
            g_GicV2State.CpuState[cpu].DbgVirtSpuriousMarked = true;
        }
    }
}

int hv_vgicv2_cpuintf_init(int cpu)
{
    printf("hv_vgicv2_cpuintf_init: GICv2 cpu interface init (CPU ID %d)\n", cpu);
    assert(cpu < NR_GICV2_MAX_CPUS);

    memset(&g_GicV2State.CpuState[cpu], 0, sizeof(HV_GICV2_CPU_STATE));
    g_GicV2State.CpuState[cpu].CpuId = cpu;
    g_GicV2State.CpuState[cpu].DbgVirtSpuriousMarked = false;

    return 0;
}

int hv_vgicv2_init(void)
{
    printf("hv_vgicv2_init: GICv2 global state and distributor init\n");
    memset(&g_GicV2State, 0, sizeof(HV_GICV2_STATE));

    printf("hv_vgicv2_init: map Distributor MMIO\n");
    hv_map_hook((u64) 0xF00000000, hv_vgicv2_handle_gicd_mmio, 0x1000);

    printf("hv_vgicv2_init: map CPU interface MMIO\n");
    hv_map_hook((u64) 0xF10000000, hv_vgicv2_handle_gicc_mmio, 0x1000);

    return 0;
}
