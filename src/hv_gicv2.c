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
    UNUSED(cpu);
    UNUSED(type);
    UNUSED(set);
}

int hv_vgicv2_init(void)
{
    printf("hv_vgicv2_init: GICv2 init\n");
    memset(&g_GicV2State, 0, sizeof(HV_GICV2_STATE));

    printf("hv_vgicv2_init: map Distributor MMIO\n");
    hv_map_hook((u64) 0xF00000000, hv_vgicv2_handle_gicd_mmio, 0x1000);

    printf("hv_vgicv2_init: map CPU interface MMIO\n");
    hv_map_hook((u64) 0xF10000000, hv_vgicv2_handle_gicc_mmio, 0x1000);

    return 0;
}
