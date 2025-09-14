from fetch_decode import *
from execution import IntegerQueueEntry
import json

class FIFO:
   def __init__(self, init: list, initSize: int):
      self.list = init
      self.head = initSize % len(init)
      self.tail = 0
      self.empty = initSize == 0
   
   def push(self, item) -> int:
      if self.head == self.tail and not self.empty:
         raise Exception("Pushing to full FIFO")
      r = self.head
      self.list[self.head] = item
      self.head = (self.head + 1) % len(self.list)
      self.empty = False
      return r

   def pop(self):
      if self.empty:
         raise Exception("Poping from empty FIFO")
      r = self.list[self.tail]
      self.tail = (self.tail + 1) % len(self.list)
      if self.tail == self.head:
         self.empty = True
      return r
   
   def peek(self):
      if self.empty:
         raise Exception("Peeking from empty FIFO")
      return self.list[self.tail]
   
   def revert(self):
      if self.empty:
         raise Exception("Reverting from empty FIFO")
      self.head = (len(self.list) + self.head - 1) % len(self.list)
      if self.head == self.tail:
         self.empty = True
      return self.list[self.head]
   
   def backPush(self, item):
      if self.head == self.tail and not self.empty:
         raise Exception("Back Pushing to full FIFO")
      self.tail = (len(self.list) + self.tail - 1) % len(self.list)
      self.list[self.tail] = item
      self.empty = False
   
   def len(self) -> int:
      if self.head == self.tail and not self.empty:
         return len(self.list)
      return (len(self.list) + self.head - self.tail) % len(self.list)
   
   def emptySlots(self) -> int:
      return len(self.list) - self.len()
   
   def toJson(self):
      res = []
      # if self.empty:
      #    return res
      for i in range(self.len()):
         res.append(self.list[(self.tail + i) % len(self.list)])
      return res
   
   def __str__(self):
      return str(self.toJson)


class ActiveListEntry:
   def __init__(self, logicalDest: int, oldDest: int, pc: int):
      self.done = False
      self.exception = False
      self.logicalDest = logicalDest
      self.oldDest = oldDest
      self.pc = pc

   def toJson(self): 
      res = {
         "Done": self.done,
         "Exception": self.exception,
         "LogicalDestination": self.logicalDest,
         "OldDestination": self.oldDest,
         "PC": self.pc
      }
      return res
   
   def __str__(self):
      return json.dumps(self.toJson())
   

class RegisterFile:
   def __init__(self, fd: FetchDecode, nPhysReg: int = 64, nLogiReg: int = 32):
      self.physicalRF: list[bytes] = [int.to_bytes(0, 8, 'little', signed=False)] * nPhysReg
      self.busyBitTable = [False] * nPhysReg
      self.registerMap: list[int] = list(range(nLogiReg))
      self.freeList = FIFO([i for i in range(32, 64)], 32)
      self.FD = fd

   def readReg(self, physTag: int, signed: bool) -> int:
      return int.from_bytes(self.physicalRF[physTag], 'little', signed=signed)
   
   def writeReg(self, physTag: int, val: int):
      self.physicalRF[physTag] = int.to_bytes(val, 8, 'little', signed=False)
      # mark as not busy
      self.busyBitTable[physTag] = False

   def isBusy(self, physTag: int) -> bool:
      return self.busyBitTable[physTag]
   
   def forward(self, aluResults: list[tuple[int, int, int, bool]]):
      # write back takes place before commit
      for dest, val, _, exception in aluResults:
         if exception:
            continue
         self.writeReg(dest, val)
   
   def dispatch(self, dirs: list[DecodedInstructionRegister]
                ) -> tuple[list[ActiveListEntry], list[IntegerQueueEntry]]:
      al = []
      iq = []
      # only dispatch all incoming instructions
      if self.freeList.len() < len(dirs):
         # freeze fetch/decode unit
         self.FD.stopFetch()
         return al, iq
      for ins in dirs:
         # renaming
         regTag: int = self.freeList.pop()
         self.busyBitTable[regTag] = True
         al.append(ActiveListEntry(ins.dest, self.registerMap[ins.dest], ins.pc))
         opA = self.registerMap[ins.opA]
         opB = ins.opB if ins.opcode == 'addi' else self.registerMap[ins.opB]
         iq.append(IntegerQueueEntry(regTag, opA, opB, ins.opcode, ins.pc))
         # registerMap can only be updated after operand tag reading in case operand == dest
         self.registerMap[ins.dest] = regTag
      return al, iq
   
   def commit(self, committed: ActiveListEntry):
      # This shall always be safe and overflow-free
      self.freeList.push(committed.oldDest)

   def sanityChecks(self):
      assert self.freeList.len() == 32
      for b in self.busyBitTable:
         assert not b
      for r in self.registerMap:
         assert r not in self.freeList.list


class ActiveList:
   def __init__(self, nPhysReg: int = 64, nLogiReg: int = 32):
      self.list = FIFO([None]*(nPhysReg - nLogiReg), 0)

   def isEmpty(self) -> bool:
      return self.list.empty

   def dispatch(self, l: list[ActiveListEntry]) -> list[int]:
      # add instructions to FIFO
      tags = [self.list.push(a) for a in l]
      return tags
   
   def commit(self) -> list[ActiveListEntry]:
      # at most 4 instructions can be committed each cycle
      i = 0
      res = []
      while i < 4 and not self.list.empty:
         r: ActiveListEntry = self.list.peek()
         if not r.done:
            break
         res.append(r)
         if r.exception:
            # handle exception
            return res
         # does not pop out the exception entry, for rollback
         self.list.pop()
         i += 1
      return res
   
   def forward(self, aluResults: list[tuple[int, int, int, bool]]):
      # write back takes place before commit
      l: list[ActiveListEntry] = self.list.list
      for _, _, tag, exception in aluResults:
         l[tag].done = True
         l[tag].exception = exception

   def revert(self) -> ActiveListEntry:
      return self.list.revert()