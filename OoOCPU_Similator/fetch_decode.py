import json

class DecodedInstructionRegister:

   def __init__(self, ins: str, pc: int):
      insd = ins.split(' ')
      self.pc = pc
      self.opcode = insd[0]
      self.dest = int(insd[1][1:-1])
      self.opA = int(insd[2][1:-1])
      if insd[0] == 'addi':
         self.opB = int(insd[3])
      else:
         self.opB = int(insd[3][1:])


class FetchDecode:
   def __init__(self, mem: list[str]):
      self.mem = mem
      self.pc = 0
      self.exceptionPC = 0
      self.exception = False
      self.DIR: list[DecodedInstructionRegister] = []
      # freeze when issue queue full
      self.freeze = False

   def stopFetch(self):
      self.freeze = True

   def resumeFetch(self):
      self.freeze = False
   
   def getDIR(self):
      if self.exception:
         self.DIR = []
      return self.DIR

   def decode(self):
      # keep DIR state when fetch freezed
      if self.freeze:
         # will be re-freezed again if still cannot dispatch
         self.resumeFetch()
         return
      self.DIR = []
      # stop fetching when pc out of bound and exception
      if self.pc >= len(self.mem):
         return
      for pc in range(self.pc, min(self.pc+4, len(self.mem))):
         self.DIR.append(DecodedInstructionRegister(self.mem[pc], pc))
      self.pc += len(self.DIR)

   def raiseException(self, pc: int):
      self.exception = True
      self.exceptionPC = pc
      self.pc = 0x10000

   def exitException(self):
      self.exception = False
      # self.exceptionPC = 0
      

def parseInstructions(path: str):
   with open(path, 'r') as f:
      mem = json.load(f)
   return mem