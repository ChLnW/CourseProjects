from fetch_decode import FetchDecode
from execution import IntegerQueue, ALU
from register_file import RegisterFile, ActiveList
import json

class CPU:
   def __init__(self, mem: list[str]):
      self.fetchDecode = FetchDecode(mem)
      self.registerFile = RegisterFile(self.fetchDecode)
      self.integerQueue = IntegerQueue(self.registerFile)
      self.activeList = ActiveList()
      self.ALU = ALU()

   def toJson(self):
      res = {
         'PC': self.fetchDecode.pc,
         'PhysicalRegisterFile': [self.registerFile.readReg(i, False) for i in range(64)],
         'DecodedPCs': [i.pc for i in self.fetchDecode.getDIR()],
         'ExceptionPC': self.fetchDecode.exceptionPC,
         'Exception': self.fetchDecode.exception,
         'RegisterMapTable': self.registerFile.registerMap.copy(),
         'FreeList': self.registerFile.freeList.toJson(),
         'BusyBitTable': self.registerFile.busyBitTable.copy(),
         'ActiveList': [ale.toJson() for ale in self.activeList.list.toJson()],
         'IntegerQueue': self.integerQueue.toJson()
      }
      return res
   
   def dumpStateIntoLog(self, f = None):
      print(self.toJson())
      if f is None:
         print(json.dumps(self.toJson(), indent=3))
      else:
         json.dump(self.toJson(), f, indent=3)

   def exceptionMode(self):
      # roll back in case of exception
      # reset integer queue and execution stage
      # self.integerQueue.clear()
      # self.ALU.clear()
      # roll back up to 4 active list entries per cycle
      i = 4
      if self.activeList.isEmpty():
         # rollback finished
         # saniti checks:
         self.registerFile.sanityChecks()
         self.fetchDecode.exitException()
         return
      while i > 0 and not self.activeList.isEmpty():
         ale = self.activeList.revert()
         newDest = self.registerFile.registerMap[ale.logicalDest]
         # free list and busybit table
         # self.registerFile.freeList.backPush(newDest)
         self.registerFile.freeList.push(newDest)
         self.registerFile.busyBitTable[newDest] = False
         self.registerFile.registerMap[ale.logicalDest] = ale.oldDest
         i -= 1
      

   
   def run(self):
      if self.fetchDecode.exception:
         self.exceptionMode()
         return
      raiseException = False
      # prepare stage outputs for forwarding
      # Commit
      commits = self.activeList.commit()
      for c in commits:
         if c.exception:
            raiseException = True
            break
         self.registerFile.commit(c)
      # F&D: depends on freelist restorage
      fd = self.fetchDecode.getDIR()
      # ALU
      alu = self.ALU.getResult()
      self.registerFile.forward(alu)
      self.activeList.forward(alu)
      # R&D: depends on forwarding of alu results
      rd = self.integerQueue.issue()
      
      # execution after R&D is always safe to proceed
      self.ALU.execute()
      self.ALU.issue(rd)
      al, iq = self.registerFile.dispatch(fd)
      tags = self.activeList.dispatch(al)
      self.integerQueue.dispatch(iq, tags)
      # fetch after dispatch as may be stopped when free list empty
      self.fetchDecode.decode()
      # handle exception
      if raiseException:
         self.fetchDecode.raiseException(commits[-1].pc)
         self.integerQueue.clear()
         self.ALU.clear()