class IntegerQueueEntry:
   def __init__(self, destReg: int, opATag: int, opBTag: int, opcode: str, pc: int):
      self.destReg = destReg
      self.opATag = opATag
      self.opAVal = None
      if opcode == 'addi':
         self.opBTag = None
         self.opBVal = opBTag
      else:
         self.opBTag = opBTag
         self.opBVal = None
      self.opcode = opcode
      self.pc = pc
      self.tag = None

from register_file import RegisterFile

class IntegerQueue:
   def __init__(self, rf: RegisterFile):
      self.queue: list[IntegerQueueEntry] = []
      self.RF = rf
   
   def dispatch(self, l: list[IntegerQueueEntry], tags: list[int]):
      for iqe, t in zip(l, tags):
         iqe.tag = t
         opSigned = iqe.opcode == 'add' or iqe.opcode == 'addi' or iqe.opcode == 'sub'
         if not self.RF.isBusy(iqe.opATag):
            iqe.opAVal = self.RF.readReg(iqe.opATag, signed=opSigned)
            # Clears tag after value read
            iqe.opATag = None
         # opB may be constant
         if iqe.opBVal is None and not self.RF.isBusy(iqe.opBTag):
            iqe.opBVal = self.RF.readReg(iqe.opBTag, signed=opSigned)
            # Clears tag after value read
            iqe.opBTag = None
      self.queue += l

   def issue(self) -> list[IntegerQueueEntry]:
      res = []
      i = 0
      while i < len(self.queue):
         iqe = self.queue[i]
         opSigned = iqe.opcode == 'add' or iqe.opcode == 'addi' or iqe.opcode == 'sub'
         if iqe.opAVal is None and not self.RF.isBusy(iqe.opATag):
            iqe.opAVal = self.RF.readReg(iqe.opATag, signed=opSigned)
            # Clears tag after value read
            iqe.opATag = None
         # opB may be constant
         if iqe.opBVal is None and not self.RF.isBusy(iqe.opBTag):
            iqe.opBVal = self.RF.readReg(iqe.opBTag, signed=opSigned)
            # Clears tag after value read
            iqe.opBTag = None
         if iqe.opAVal is not None and iqe.opBVal is not None and len(res) < 4:
            res.append(iqe)
            self.queue.pop(i)
         else:
            i += 1
         # i not incremented as item poped off
         # if len(res) == 4:
         #    break
      return res
   
   def clear(self):
      self.queue.clear()

   def toJson(self):
      res = []
      for i in self.queue:
         res.append({
            "DestRegister": i.destReg,
            "OpAIsReady": i.opAVal is not None,
            "OpARegTag": i.opATag,
            "OpAValue": i.opAVal,
            "OpBIsReady": i.opBVal is not None,
            "OpBRegTag": i.opBTag,
            "OpBValue": i.opBVal,
            "OpCode": 'add' if i.opcode == 'addi' else i.opcode,
            "PC": i.pc
         })
      return res
         

class ALU:
   RESMASK = (1 << 64) - 1
   def __init__(self):
      # stage buffer for issue
      self.instructions: list[IntegerQueueEntry] = []
      # stage buffer for cycle 1
      self.valBuffer: list[int] = []
      self.destBuffer: list[int] = []
      self.tags: list[int] = []
      self.exceptions: list[bool] = []
      # stage buffer for cycle 2
      # self.results: list[tuple[int, int, int, bool]] = []

   # Always getResult() before issue()
   def getResult(self):
      return list(zip(self.destBuffer, self.valBuffer, self.tags, self.exceptions))
   
   # always issue after execute
   def issue(self, instructions: list[IntegerQueueEntry]):
      self.instructions = instructions

   def execute(self):
      # self.results = list(zip(self.destBuffer, self.valBuffer, self.tags, self.exceptions))
      # safe to reuse internal lists
      self.valBuffer.clear()
      self.destBuffer.clear()
      self.tags.clear()
      self.exceptions.clear()
      for ins in self.instructions:
         self.destBuffer.append(ins.destReg)
         self.tags.append(ins.tag)
         self.exceptions.append(False)
         # ALU operations by opcode
         if ins.opcode == 'add':
            r = ins.opAVal + ins.opBVal
         elif ins.opcode == 'addi':
            r = ins.opAVal + ins.opBVal
         elif ins.opcode == 'sub':
            r = ins.opAVal - ins.opBVal
         elif ins.opcode == 'mulu':
            r = ins.opAVal * ins.opBVal
         elif ins.opcode == 'divu':
            # div by 0 exception
            if ins.opBVal == 0:
               r = 0
               self.exceptions[-1] = True
            else:
               r = ins.opAVal // ins.opBVal
         elif ins.opcode == 'remu':
            # div by 0 exception
            if ins.opBVal == 0:
               r = 0
               self.exceptions[-1] = True
            else:
               r = ins.opAVal % ins.opBVal
         else:
            raise Exception(f'Unknown instruction {ins.opcode}')
         self.valBuffer.append(r & ALU.RESMASK)

   def clear(self):
      self.instructions.clear()
      self.destBuffer.clear()
      self.valBuffer.clear()
      self.tags.clear()
      self.instructions.clear()