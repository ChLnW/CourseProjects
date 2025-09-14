import sys

if len(sys.argv) > 1:
   fn = sys.argv[1]
else:
   fn = "outs"

results = []

with open(fn, 'r') as f:
   line = f.readline()
   while line:
      # print(line)
      if "summary of summaries:" in line:
         for i in range(3):
            f.readline()
         res = []
         for i in range(2):
            line = f.readline()
            if "verified" in line:
               res.append(True)
            elif "falsified" in line:
               res.append(False)
            else:
               res.append("Unknown")
               break
         
         results.append(tuple(res))

      line = f.readline()
      # break
# print(results)
# for res in results:
#    print(res)
bools = ["True", "False"]

iocaps = ["DisplayOnly",
          "DisplayYesNo",
          "KeyboardOnly",
          "NoInputNoOutput",
          "KeyboardDisplay"]
i = 0
print(f"IRK,MITM,IOCap,Careful,Careless")
for irk in bools:
   for mitm in bools:
      for iocap in iocaps:
         # BR/EDR pairing doesn't have KeyboardDisplay option for IOcap
         if iocap=="KeyboardDisplay" and ("br" in fn):
            continue
         print(f"{irk},{mitm},{iocap},{results[i][0]},{results[i][1]}")
         i += 1