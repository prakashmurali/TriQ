import sys

global_gate_id = 0
prev_gate = {}
gate_names = {"cx":"CNOT", "cz":"CZ", "h":"H", "x":"X", "y":"Y", "z":"Z", "rx":"RX", "ry":"RY", "rz":"RZ", "s":"S", "sdg":"Sdag", "t":"T", "tdg":"Tdag", "measure":"MeasZ"}
gset1 = ['x', 'y', 'z', 'h', 's', 'sdg', 't', 'tdg', 'measure']
gset2 = ['rx', 'ry', 'rz']
gset3 = ['cx']
 
def find_dep_gate(qbit):
    if qbit in prev_gate.keys():
        return [prev_gate[qbit]]
    else:
        return []

def update_dep_gate(qbit, gate_id):
    prev_gate[qbit] = gate_id

def check_valid_gate(line):
    flag = 0
    gset = []
    gset.extend(gset1)
    gset.extend(gset2)
    gset.extend(gset3)
    for g in gset:
        if line.startswith(g):
            flag = 1
            break
    return flag

def process_gate(line, f_out):
    global gset1
    global gset2
    global gset3
    gflag = 0
    dep_gates = []
    for g in gset1:
        if line.startswith(g):
            qbit = line.split()[1].split('[')[1].split(']')[0]
            gflag = 1
            #print(global_gate_id, g, qbit)
            f_out.write(str(global_gate_id) + ' ' + gate_names[g] + ' 1 ' + qbit)
            dep_gates = find_dep_gate(qbit)
            if len(dep_gates) == 1:
                f_out.write(' 1 ' + str(dep_gates[0]) + '\n')
            elif len(dep_gates) == 0:
                f_out.write(' 0\n')
            else:
                assert(0)
            update_dep_gate(qbit, global_gate_id)

            break
    for g in gset2:
        if line.startswith(g):
            qbit = line.split()[1].split('[')[1].split(']')[0]
            angle = line.split()[0].split('(')[1].split(')')[0]
           
            print(global_gate_id, g, qbit, angle)
            f_out.write(str(global_gate_id) + ' ' + gate_names[g] + ' 1 ' + qbit)
            dep_gates = find_dep_gate(qbit)
            if len(dep_gates) == 1:
                f_out.write(' 1 ' + str(dep_gates[0]) + ' ' + str(angle) + '\n')
            elif len(dep_gates) == 0:
                f_out.write(' 0 ' + str(angle) + '\n')
            else:
                assert(0)
            update_dep_gate(qbit, global_gate_id)
            gflag = 1
            break
    for g in gset3:
         if line.startswith(g):
            base = ''.join(line.split()).split(',')
            qbit1 = base[0].split('[')[1].split(']')[0]
            qbit2 = base[1].split('[')[1].split(']')[0]
            #print(global_gate_id, g, qbit1, qbit2)
            f_out.write(str(global_gate_id) + ' ' + gate_names[g] + ' 2 ' + qbit1 + ' ' + qbit2)
            dep_gates1 = find_dep_gate(qbit1)
            dep_gates2 = find_dep_gate(qbit2)
            dep_gates1.extend(dep_gates2)
            update_dep_gate(qbit1, global_gate_id)
            update_dep_gate(qbit2, global_gate_id)
            f_out.write(' ' + str(len(dep_gates1)) + ' ')
            for item in dep_gates1:
                f_out.write(str(item) + ' ')
            f_out.write('\n')
            gflag = 1
            break
     
def parse_ir(fname, outfname):
    global global_gate_id
    f = open(fname, 'r')
    l = f.readlines()
    f_out = open(outfname, 'w')

    qcnt = 0
    gcnt = 0
    for line in l:
        line = ' '.join(line.split())
        if line.startswith("OPENQASM"):
            continue
        elif line.startswith("include"):
            continue
        elif line.startswith("qreg"):
            sline = line.split()
            qcnt = int(sline[1].split('[')[1].split(']')[0])
        elif line.startswith("creg"):
            continue
        else:
            if check_valid_gate(line):
                gcnt += 1
    
    f_out.write(str(qcnt) + ' ' + str(gcnt) + '\n')

    for line in l:
        line = ' '.join(line.split())
        if line.startswith("OPENQASM"):
            continue
        elif line.startswith("include"):
            continue
        elif line.startswith("qreg"):
            continue
        elif line.startswith("creg"):
            continue
        else:
            if check_valid_gate(line):
                process_gate(line, f_out)
                global_gate_id += 1

parse_ir(sys.argv[1], sys.argv[2])
