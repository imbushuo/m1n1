# AIC to GIC compatibility layer

## Contract

- SGI (0-15) is emulated via the IPI mechanism with some mux similar to how Asahi handles IPI in Linux.
- PPI (16-31) is hardwired for timer only. m1n1 will ack the timer FIQ on behalf of the guest OS, then signal the underlying VM regarding timer PPI with hardwired interrupt ID. Only EL1 virt timer is about to be supported presently (no plan for PMU.)
- SPI (32 and beyond) are handled similar to regular AIC's interrupt with a quirk. To make the device tree transition easier, all existing non-PPI AIC interrupts will all be mapped to SPI in Device Tree binding, which means they will have a starting point of 32. However for real AIC interrupt there are a few devices with interrupt ID lower than 32. To make them work, the compatibility layer will subtract 32 when mapping them to the real AIC interrupt.

## GIC features

- Scoping to GICv2 presently as my main testing target is MacBook Air M2 with only 8 cores.
- Preempt and Binary points are ignored as AIC has automatic priority adjustment.

## Interrupt Flows

### FIQ (Timer)

- Ack FIQ on behalf of the guest.
- Read the FIQ reason, if that's timer, mark timer interrupt as pending.
- Deliver timer interrupt to the FIQ's corresponding CPU core via PPI.

### IRQ

- Ack the IRQ on behalf of the guest.  Retrieve the guest-facing IRQ number. AIC will mask the interrupt for now.
- Put it in the associating CPU's GICC (GICC_IAR)
- Upon leaving the hypervisor and entering the guest, evanlate the situation and signal the hypervisor regarding this interrupt's arrival.
- Guest will read it and complete acknowledgement by writing to GICC_EOIR.
- Once GICC_EOIR is written, notify AIC to unmask the interrupt.

### SGI / IPI

- Note the inbound IPI to the CPU core and source via a mux mechanism.
- Kick the IPI to the target CPU core.
- The target CPU core ack the IPI, read the mux to know which SGI was signaled, then deliver the interrupt to the SGI's corresponding CPU core.
