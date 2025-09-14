import os

# Uncomment one of these.
# src = "smp.tg"
src = "br.tg"

bools = ["True", "False"]

iocaps = ["DisplayOnly",
          "DisplayYesNo",
          "KeyboardOnly",
          "NoInputNoOutput",
          "KeyboardDisplay"]

lemmas = (
"""lemma IRKTheftProofCareless = 
   All pid irk mitm iocap #i #k. 
      (Distribute_IdKey(pid, irk, mitm, iocap) @ #i 
      & Careless_Implementation() @ #k
      )
        ==>
        (Ex #j. User_Confirmation(pid) @ #j
                & j < i)

lemma IRKTheftProofCareful = 
   All pid irk mitm iocap #i #k. 
      (Distribute_IdKey(pid, irk, mitm, iocap) @ #i 
      & Careful_Implementation() @ #k
      )
        ==>
        (Ex #j. User_Confirmation(pid) @ #j
                & j < i)


lemma SanityCheck = 
   exists-trace
   // Ex #i. Response_Received() @ #i
   Ex pid irk mitm iocap #i. Distribute_IdKey(pid, irk, mitm, iocap) @ #i"""
)

out = src+".outs"

os.system(f"rm -rf {out} tmp;mkdir tmp")


for irk in bools:
   for mitm in bools:
      for iocap in iocaps:
         # BR/EDR pairing doesn't have KeyboardDisplay option for IOcap
         if iocap=="KeyboardDisplay" and src=="br.tg":
            continue
         process = (
f"""process {iocap}_{"No" if mitm=="False" else ""}Mitm_{"No" if irk=="False" else ""}Irk = 
   [ Fr(~id) ] 
      --> 
      [ 'idkey := {irk}(), 'mitm := {mitm}(), 'iocap := {iocap}(), 'process_id := ~id,
        'irk_a_rsp := tbi(), 'mitm_b := tbi(), 'iocap_b := tbi(), 'protocol := tbi() ];
   Initiator('process_id, 'idkey, 'mitm, 'iocap, 'irk_a_rsp, 'mitm_b, 'iocap_b, 'protocol)
""")     
         fn = "./tmp/tmp_"+src
         os.system(
         # print(
f"""cp {src} {fn} && 
echo >> {fn} && 
echo \"{process}\" >> {fn} && 
rm -f {fn}.spthy && 
~/tamgram/tamgram compile {fn} {fn}.spthy && 
tamarin-prover {fn}.spthy --prove >> {out}
""")
   #       break
   #    break
   # break


         