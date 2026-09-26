#include "cpu6502.h"

#include <stdlib.h>

enum {
  FLAG_N = 0x80,
  FLAG_V = 0x40,
  FLAG_U = 0x20,
  FLAG_B = 0x10,
  FLAG_D = 0x08,
  FLAG_I = 0x04,
  FLAG_Z = 0x02,
  FLAG_C = 0x01,
};

static uint16_t program_counter(CPU *cpu) {
  return cpu->program_counter_lo | (cpu->program_counter_hi << 8);
}

static void write_cpu(CPU *cpu, uint16_t addr, uint8_t val) {
  cpu->bus_state.addr = addr;
  cpu->bus_state.data = val;
  cpu->bus_state.write = true;
}

static void read_cpu(CPU *cpu, uint16_t addr) {
  cpu->bus_state.addr = addr;
  cpu->bus_state.write = false;
}

static void run_reset_cycle(CPU *cpu) {
  switch (cpu->cpu_state.cycle_status.interrupt_cycle_reset) {
  case 0:
    cpu->cpu_state.interrupt_status.reset_pending = false;
    cpu->cpu_state.interrupt_status.nmi_pending = false;
    cpu->cpu_state.interrupt_status.irq_line_apu = false;
    cpu->cpu_state.interrupt_status.irq_line_mapper = false;

    cpu->cpu_state.cycle_status.interrupt_cycle_irq = 0;
    cpu->cpu_state.cycle_status.interrupt_cycle_nmi = 0;
    cpu->cpu_state.cycle_status.total_cycles = 0;
    cpu->cpu_state.cycle_status.interrupt_cycle_irq = 0;

    cpu->cpu_state.instruction_status.address_lo = 0;
    cpu->cpu_state.instruction_status.address_hi = 0;
    cpu->cpu_state.instruction_status.alu_result = 0;
    cpu->cpu_state.instruction_status.operand = 0;
    cpu->cpu_state.instruction_status.operand = 0;
    cpu->cpu_state.instruction_status.carry = false;
    cpu->cpu_state.instruction_status.dummy_cycle = false;
    cpu->cpu_state.instruction_status.page_crossed = false;

    cpu->cpu_state.oam_dma_status.oam_dma_enabled = false;
    cpu->cpu_state.oam_dma_status.write_line = false;
    cpu->cpu_state.oam_dma_status.oam_dma_cycle = 0;
    cpu->cpu_state.oam_dma_status.oam_dma_addr = 0;
    cpu->cpu_state.oam_dma_status.write_value = 0;
    read_cpu(cpu, program_counter(cpu));
    break;

  case 1:
    cpu->flag_status |= FLAG_I | FLAG_U;
    read_cpu(cpu, 0x100 | cpu->stack_pointer);
    break;

  case 2:
    cpu->stack_pointer--;
    read_cpu(cpu, 0x100 | cpu->stack_pointer);
    break;

  case 3:
    cpu->stack_pointer--;
    read_cpu(cpu, 0x100 | cpu->stack_pointer);
    break;

  case 4:
    cpu->stack_pointer--;
    read_cpu(cpu, 0xFFFC);
    break;

  case 5:
    cpu->program_counter_lo = cpu->bus_state.data;
    read_cpu(cpu, 0xFFFD);
    break;

  case 6:
    cpu->program_counter_hi = cpu->bus_state.data;
    read_cpu(cpu, program_counter(cpu));
    break;
  }

  cpu->cpu_state.cycle_status.interrupt_cycle_reset++;
}

static void run_oam_dma_cycle(CPU *cpu) {}

static void run_irq_cycle(CPU *cpu) {}

static void run_nmi_cycle(CPU *cpu) {}

static void run_instruction_cycle(CPU *cpu) {}

CPU *get_a_fresh_cpu(void) {
  CPU *cpu = malloc(sizeof(CPU));

  if (!cpu) {
    return NULL;
  }

  cpu->accumulator = 0;
  cpu->register_X = 0;
  cpu->register_Y = 0;
  cpu->stack_pointer = 0;
  cpu->flag_status = 0;
  cpu->program_counter_lo = 0;
  cpu->program_counter_hi = 0;

  cpu->bus_state = (CPU_Bus){0, 0, false};
  cpu->cpu_state = (CPU_Status){
      (CPU_CycleStatus){0, 0, 0, 0},
      (CPU_InterruptStatus){false, false, false, true},
      (CPU_OAM_DMA_Status){false, false, 0, 0, 0},
      (CPU_InstructionStatus){0, 0, 0, 0, 0, false, false, false},
  };

  return cpu;
}

void reset_a_cpu(CPU *cpu) {
  cpu->cpu_state.cycle_status.interrupt_cycle_reset = 0;
  cpu->cpu_state.interrupt_status.reset_pending = true;
  read_cpu(cpu, program_counter(cpu));
}

void clock_a_cpu(CPU *cpu) {
  if (cpu->cpu_state.interrupt_status.reset_pending ||
      cpu->cpu_state.cycle_status.interrupt_cycle_reset < 7) {
    run_reset_cycle(cpu);
  } else if (cpu->cpu_state.oam_dma_status.oam_dma_enabled) {
    run_oam_dma_cycle(cpu);
  } else if (cpu->cpu_state.interrupt_status.nmi_pending ||
             cpu->cpu_state.cycle_status.interrupt_cycle_nmi > 0 &&
                 cpu->cpu_state.cycle_status.interrupt_cycle_irq == 0 &&
                 cpu->cpu_state.cycle_status.instruction_cycle == 0) {
    run_nmi_cycle(cpu);
  } else if ((cpu->cpu_state.interrupt_status.irq_line_apu ||
              cpu->cpu_state.interrupt_status.irq_line_mapper) ||
             cpu->cpu_state.cycle_status.interrupt_cycle_nmi > 0 &&
                 cpu->cpu_state.cycle_status.instruction_cycle == 0 &&
                 !(cpu->flag_status & FLAG_I)) {
    run_irq_cycle(cpu);
  } else {
    run_instruction_cycle(cpu);
  }

  cpu->cpu_state.cycle_status.total_cycles++;
}
