/* SPDX-License-Identifier: MIT */

#ifndef HV_GICV2_H
#define HV_GICV2_H

#include "exception.h"
#include "iodev.h"
#include "types.h"
#include "uartproxy.h"

#define NR_GICV2_MAX_CPUS 8

#define NR_INTC_ARCH_TIMER_HYPERVISOR_EL2_PHYSICAL 26
#define NR_INTC_ARCH_TIMER_NONSECURE_EL1_VIRTUAL 27
#define NR_INTC_ARCH_TIMER_SECURE_EL1_PHYSICAL 29
#define NR_INTC_ARCH_TIMER_NONSECURE_EL1_PHYSICAL 30

#define NR_SGI_BEGIN 0
#define NR_PPI_BEGIN 16
#define NR_SPI_BEGIN 32

typedef struct _HV_GICV2_CPU_STATE {
    u64 CpuId;
    // Pending Interrupt state, basically this is GICC_IAR
    u64 PendingInterruptState;
    // 16 SGIs, represented by bitmap of u64
    u64 IntcSgiEnabled;
    u64 IntcSgiBits;
    // 16 PPIs, only timer is useful right now
    u64 IntcPpiEnabled;
    u64 IntcPpiBits;
    // Debug stuff
    bool DbgVirtSpuriousMarked;
} HV_GICV2_CPU_STATE, *PHV_GICV2_CPU_STATE;

typedef struct _HV_GICV2_DISTRIBUTOR_STATE {
    // 987 SPIs that shared with 16 partitions
    u64 IntcSpiEnabled[16];
    u64 IntcSpiBits[16];
} HV_GICV2_DISTRIBUTOR_STATE, *PHV_GICV2_DISTRIBUTOR_STATE;

typedef struct _HV_GICV2_STATE {
    bool Enabled;
    HV_GICV2_CPU_STATE CpuState[NR_GICV2_MAX_CPUS];
    HV_GICV2_DISTRIBUTOR_STATE DistributorState;
} HV_GICV2_STATE, *PHV_GICV2_STATE;

typedef enum _GICV2_PPI_TIMER_TYPE {
    Physical = 0,
    Virtual = 1
} GICV2_PPI_TIMER_TYPE;

// GICv2 initialization
int hv_vgicv2_init(void);
int hv_vgicv2_cpuintf_init(int cpu);

// Handler for redirecting FIQ to IRQ (timer)
void hv_vgicv2_notify_timer_interrupt(int cpu, GICV2_PPI_TIMER_TYPE type, bool set);

// Handler for IPI
void hv_vgicv2_kick_sgi(int cpu, int target_cpu, int id);

#endif
