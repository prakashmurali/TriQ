import subprocess as sp
import sys
import os
scaffcc_path=""
triq_path=""
out_path=""

out_file=open("compile.log",'w')
sp.call([os.path.join(scaffcc_path, "scaffold.sh"), "-b", sys.argv[1]], stdout=out_file)
base_name = ''.join(sys.argv[1].split('.')[:-1])
ir_name = base_name + ".qasm"
dag_name = base_name + ".in"
out_name = base_name + ".qasm"
sp.call(["python3.6", "./ir2dag.py", ir_name, dag_name])

sp.call([os.path.join(triq_path, "triq"), os.path.join(triq_path, dag_name), os.path.join(triq_path, out_name), sys.argv[2], " 0 "])

