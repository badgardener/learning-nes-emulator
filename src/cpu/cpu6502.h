#ifndef CPU6502_H
#define CPU6502_H

#include <stdint.h>

/*
 * Boolean type used by the CPU model.
 *
 * false = 0
 * true  = 1
 */
#define bool uint8_t
#define true 1
#define false 0

/*
 * External CPU bus.
 *
 * Represents the signals exchanged between the CPU and the
 * NES motherboard during a CPU clock cycle.
 */
typedef struct CPU_Bus {
  /* Current 16-bit address driven on A0-A15. */
  uint16_t addr;

  /* Current 8-bit value present on D0-D7. */
  uint8_t data;

  /* Bus direction: false = read, true = write. */
  bool write;
} CPU_Bus;

/*
 * CPU cycle and timing state.
 */
typedef struct CPU_CycleStatus {
  /* Current cycle within the active instruction. */
  uint8_t instruction_cycle;

  /* Current cycle within an IRQ sequence. */
  uint8_t interrupt_cycle_irq;

  /* Current cycle within an NMI sequence. */
  uint8_t interrupt_cycle_nmi;

  /* Current cycle within the reset sequence. */
  uint8_t interrupt_cycle_reset;

  /* Total number of CPU clock cycles elapsed. */
  uint64_t total_cycles;
} CPU_CycleStatus;

/*
 * OAM DMA execution state.
 */
typedef struct CPU_OAM_DMA_Status {
  /* Indicates whether OAM DMA is currently active. */
  bool oam_dma_enabled;

  /* Current DMA bus direction: false = read, true = OAM write. */
  bool write_line;

  /* Current cycle within the DMA transfer. */
  uint16_t oam_dma_cycle;

  /* Current 16-bit source address used by DMA. */
  uint16_t oam_dma_addr;

  /* Byte temporarily held between DMA read and OAM write. */
  uint8_t write_value;
} CPU_OAM_DMA_Status;

/*
 * CPU interrupt state.
 */
typedef struct CPU_InterruptStatus {
  /* Indicates that an NMI request is pending. */
  bool nmi_pending;

  /* Current IRQ request from the APU. */
  bool irq_line_apu;

  /* Current IRQ request from the cartridge mapper. */
  bool irq_line_mapper;

  /* Indicates that the CPU is currently handling reset. */
  bool reset_pending;
} CPU_InterruptStatus;

/*
 * Temporary state used by the instruction timing logic.
 *
 * These fields are internal execution state and are not
 * architectural registers visible to software.
 */
typedef struct CPU_InstructionStatus {
  /* Opcode currently being processed. */
  uint8_t opcode;

  /* Temporary low byte of an effective address. */
  uint8_t address_lo;

  /* Temporary high byte of an effective address. */
  uint8_t address_hi;

  /* Temporary operand fetched from the bus. */
  uint8_t operand;

  /* Temporary result produced by the ALU. */
  uint8_t alu_result;

  /* Temporary carry state used by arithmetic/address operations. */
  bool carry;

  /* Indicates that an additional page-crossing cycle is required. */
  bool page_crossed;

  /* Indicates that the current cycle is a dummy bus operation. */
  bool dummy_cycle;
} CPU_InstructionStatus;

/*
 * Complete internal CPU execution state.
 */
typedef struct CPU_Status {
  /* CPU clock and instruction timing state. */
  CPU_CycleStatus cycle_status;

  /* CPU interrupt request and sequencing state. */
  CPU_InterruptStatus interrupt_status;

  /* OAM DMA timing and transfer state. */
  CPU_OAM_DMA_Status oam_dma_status;

  /* Temporary state belonging to the current instruction. */
  CPU_InstructionStatus instruction_status;
} CPU_Status;

/*
 * NES 6502 CPU state.
 *
 * Contains architectural registers, external bus state,
 * and internal timing/execution state.
 */
typedef struct CPU {
  /* 8-bit accumulator register. */
  uint8_t accumulator;

  /* 8-bit X index register. */
  uint8_t register_X;

  /* 8-bit Y index register. */
  uint8_t register_Y;

  /* 8-bit stack pointer addressing page $01. */
  uint8_t stack_pointer;

  /* 8-bit processor status register. */
  uint8_t flag_status;

  /* Low byte of the 16-bit program counter. */
  uint8_t program_counter_lo;

  /* High byte of the 16-bit program counter. */
  uint8_t program_counter_hi;

  /* Current externally visible CPU bus signals. */
  CPU_Bus bus_state;

  /* Internal CPU timing, interrupt, DMA, and instruction state. */
  CPU_Status cpu_state;
} CPU;

/*
 * Creates a newly initialized CPU instance.
 *
 * Returns:
 *     Pointer to the new CPU, or NULL on allocation failure.
 *
 * The CPU is not connected to motherboard memory by this function.
 */
CPU *get_a_fresh_cpu(void);

/*
 * Resets an existing CPU.
 *
 * Initializes the CPU reset state and begins the hardware reset
 * sequence. Reset timing and bus activity are handled by the
 * CPU implementation.
 *
 * Parameters:
 *     cpu - CPU instance to reset.
 */
void reset_a_cpu(CPU *cpu);

/*
 * Advances the CPU by exactly one CPU clock cycle.
 *
 * The function does not execute an entire instruction at once.
 * It advances the internal CPU state by one clock and updates
 * the externally visible bus state for that cycle.
 *
 * Parameters:
 *     cpu - CPU instance to advance.
 */
void clock_a_cpu(CPU *cpu);

#endif
