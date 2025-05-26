/* SPDX-License-Identifier: MIT */

#ifndef HV_GICV2_H
#define HV_GICV2_H

#include "exception.h"
#include "iodev.h"
#include "types.h"
#include "uartproxy.h"

typedef struct _HV_GICV2_CPU_STATE {
    u64 CpuId;
    // 16 SGIs, represented by bitmap of u64
    u64 IntcSgiEnabled;
    u64 IntcSgiBits;
    // 16 PPIs, only timer is useful right now
    u64 IntcPpiEnabled;
    u64 IntcPpiBits;
} HV_GICV2_CPU_STATE, *PHV_GICV2_CPU_STATE;

typedef struct _HV_GICV2_DISTRIBUTOR_STATE {
    // 987 SPIs that shared with 16 partitions
    u64 IntcSpiEnabled[16];
    u64 IntcSpiBits[16];
} HV_GICV2_DISTRIBUTOR_STATE, *PHV_GICV2_DISTRIBUTOR_STATE;

typedef struct _HV_GICV2_STATE {
    bool Enabled;
    HV_GICV2_CPU_STATE CpuState[8];
    HV_GICV2_DISTRIBUTOR_STATE DistributorState;
} HV_GICV2_STATE, *PHV_GICV2_STATE;

typedef enum _GICV2_PPI_TIMER_TYPE {
    Physical = 0,
    Virtual = 1
} GICV2_PPI_TIMER_TYPE;

int hv_vgicv2_init(void);
void hv_vgicv2_notify_timer_interrupt(int cpu, GICV2_PPI_TIMER_TYPE type, bool set);

#endif
