from cpu import *
import json
import sys

input = sys.argv[1]
output = sys.argv[2]
with open(input, 'r') as f:
   mem = json.load(f)

cpu = CPU(mem)
log = []
log.append(cpu.toJson())
# cpu.dumpStateIntoLog(f)
cpu.run()
# cpu.dumpStateIntoLog(f)
log.append(cpu.toJson())
while (not (cpu.activeList.isEmpty() and len(cpu.fetchDecode.getDIR())==0)) or cpu.fetchDecode.exception:
   # try:
   cpu.run()
   # except Exception:
      # log.append(cpu.toJson())
      # break
   # cpu.dumpStateIntoLog(f)
   log.append(cpu.toJson())

f = open(output, 'w')
json.dump(log, f, indent=2)
f.close()