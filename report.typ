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

- fetch new inst on pc. decode inst to get instType, rd, rs and rt
 - if there is any prior branch or jump instruction, just stall, do nothing
 - Try allocate a ROB (b) and a RS (r) entry for rd, If either is failed (full, how to tell is full?), stall execution.
 - Perform the following check

=== If inst have rs field

1. check `RegisterStat[rs]`
  - if busy, get the ROB entry `h` by `RegisterStat[rs].reorder` and check `ready` field of `ROB[h]` 
    - Ready: set `Vj` field of `RS[r]` with `ROB[h].value`, and clear `Qj` bit
    - Not-Ready: set `Qj` field of `RS[r].Qj` to h.
  - Not busy: 
    - set `Vj` field of `RS[r]` with register file 
    - clear `Qj` bit
2. initialize RS
  - set `RS[r].Busy` to True 
  - set `RS[r].Dest` to b 
3. initialize ROB
  - set `ROB[b].inst` to `opcode of inst`
  - set `ROB[b].Dest` to rd
  - set `ROB[b].ready` to False, set to True in ex stage.

=== If inst have rt field

1. check `RegisterStat[rt]`
  - if busy, get the ROB entry `h` by `RegisterStat[rt].reorder` and check `ready` field of `ROB[h]`
    - Ready: set `Vk` field of `RS[r]` with `ROB[h].value`, and clear `Qk` bit
    - Not-Ready: set `Qk` field of `RS[r].Qk` to h.
  - Not busy:
    - set `Vk` field of `RS[r]` with register file
    - clear `Qk` bit
2. initialize RS
  - set `RS[r].Busy` to True
  - set `RS[r].Dest` to b
3. initialize ROB
  - set `ROB[b].inst` to `opcode of inst`
  - set `ROB[b].Dest` to rd
  - set `ROB[b].ready` to False, set to True in ex stage.

=== If inst is arthimetic type (need to add a helper function about it)

1. record ROB entry in `RegisterStat[rd]`
2. set it to busy.

=== If inst is Load type

0. record immediate in `RS[r]`
1. record ROB entry in `RegisterStat[rd]`
2. set it to busy.

=== If inst is Store type

since store does not have rd, only record immdidate in `RS[r]`

== Execute

Iterate through the RS table, change inst state based on their type, and dispatch function unit if possible.
If no function unit available, suspend exeution. 
we have 4 alu and 4 mul and 1 memory unit. which means we can run 4 instrucion that uses alu or mul in paralllel.
the mul takes 4 cycle to complete, we need to track this somewhere in structure.

=== For arthimetic

when `RS[r].Qj` and `RS[r].Qk` is all zero, begin compute

=== For Load

when `RS[r].Qj` is clear and all store eariler in ROB have different address , calulate the address and perform memory memory read

=== For Store

when `RS[r].Qj` is clear and store at queue head (?), calculate the address and save. 

== WriteBack

Iterate through the RS table, Check if any instruction finnishs executing

=== ALL inst but store

For RS
- Get corresponding ROB entry b based by accessing `RS[r].Dest`
- set RS[r].Busy to false
- Iterate through RS table, if any entry has Qj or Qk equals to b, clear it, and set the value in field Vj or Vk

For ROB
- set `ROB[b].Value`
- set ready bit in `ROB[b]`

=== Store

check `RS[r].Qk`, if ready, set the `ROB[b].Value` using `RS[r].vk`

== Commit

commit stage
fetch the head of the ROB buffer h

For ROB:
- if `ROB[h].inst` is store, writr to memory, otherwise write to regfile
- set `ROB[h].Busy` to False

For RegisterStat:
if `RegisterStat[ROB[h].Dest].Reorder` == h
  - set `RegisterStat[ROB[h].Dest].busy` to False

update the header of ROB