#import "template.typ": *

#show: project.with(
  title: "Laboratory 6",
  authors: (
    (
      name: "龚开宸",
      email: "gongkch2024@shanghaitech.edu.cn"
    ),
  ),
)

= Data Structures

== Re-Order Buffer

#table(
  columns: 2,
  [*Field*], [*Description*],
  [Entry], [The index of the ROB entry.],
  [Ready], [Indicates if the result is ready to use.],
  [Busy], [Indicates if the instruction is currently under execution.],
  [Instruction], [The type of instruction being executed.],
  [Destination], [The destination of the instruction (e.g., register index or ROB entry index).],
  [Value], [The result value of the instruction.],
  [Address], [Holds information for the memory address calculation.]
)


== Reservation Station Structure

#table(
  columns: 2,
  [*Field*], [*Description*],
  [Function Unit], [The functional unit (FU) this reservation station (RS) entry represents.],
  [Busy], [Indicates whether the functional unit (FU) is currently busy.],
  [Op], [The type of instruction being executed.],
  [Vj], [The value of one source operand.],
  [Vk], [The value of the other source operand.],
  [Qj], [The index of the ROB entry producing the value for Vj.],
  [Qk], [The index of the ROB entry producing the value for Vk.],
  [Dest], [The destination of the instruction (e.g., register index or ROB entry index).],
  [A], [Holds information for memory address calculations (e.g., offset or effective address).]
)

== Register Status Table

#table(
  columns: 5,
  [], [x0], [x1], [...], [x31],
  [Reorder: ROB entry], [], [], [], [],
  [Busy: whether in ROB], [], [], [], [],
)


= Tomasulo integrity design

To illustrate the whole process, let us introduce some naming conventions, I will use the symbol in integrity design later.

#table(
  columns: 2,
  [*Field*], [*Description*],
  [rd], [The destination of the issuing instruction.],
  [rs, rt], [The sources of the issuing instruction.],
  [r], [The reservation station allocated for the instruction.],
  [b], [The assigned entry in the Re-Order Buffer (ROB) for the instruction.],
  [h], [The head entry of the ROB, indicating the instruction ready to commit.],
  [RS], [The reservation station data structure, which manages operands and tracks readiness.],
  [result], [The value returned by a reservation station after execution.],
  [RegisterStat], [The data structure used to track the status of registers.],
  [Regs], [Represents the actual registers in the architecture.],
  [ROB], [The reorder buffer data structure, managing instruction reordering and in-order commitment.]
)

== Issue

Issue stage

 - Get an instruction from the instruction queue. Issue the instruction if there is an empty reservation station entry and an empty slot in the ROB.
 - update reservation station
 - stall if either structure does not have empty entries

== Execute

ex stage

 - scan whole rob to check if any operand of an entry is ready, then we could execute.
 
== WriteBack

writeback

 - when the result is available, write it to ROB and update corresponding reservation station

== Commit

commit 

 - The normal commit case occurs when an instruction reaches the head of the ROB and its result is present in the buffer; at this point, the processor updates the register with the result and removes the instruction from the ROB.
 - Committing a store is similar except that memory is updated rather than a result register.

