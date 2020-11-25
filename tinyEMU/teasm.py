#!/usr/bin/python3

labels = {}
compiled = []
address = 0
loadAddress = -1
lineNumber = 1

def do_file(fileName):
    global compiled, address, loadAddress, lineNumber
    textFound = False
    with open(fileName) as f: # build symbol table
        for line in f:
            try:
                line = line.strip()
                if line[0] == '#':
                    pass # skip comment lines
                elif line[0:5] == '.data' or line[0:5] =='.text':
                    sl = line.split()
                    if len(sl) == 2:
                        address = int(sl[1],16)
                    if loadAddress == -1:
                        loadAddress = address
                elif line[0:6] == '.label':
                    labels[line.split()[1]] = address
                else:
                    address += 4
                lineNumber += 1
            except:
                print('Bad line:', lineNumber, line)
                exit()
    #f.close()
    
    lineNumber = 1
    compiled.append(hex(loadAddress)[2:]+'\n')
    with open(fileName) as f:
        for line in f:
            try:
                line = line.strip()
                if line[0] == '#':
                    pass # skip comment lines
                elif line[0:5] == '.data' or line[0:5] =='.text':
                    textFound = False if line[0:5] == '.data' else True
                elif line[0:6] == '.label':
                    pass # skip labels, already processed
                else:
                    compiled.append(translate(line) if textFound == True else line+'\n')
                lineNumber += 1
            except:
                print('Baad line #'+str(lineNumber), line)
                exit()
                
    compiled = ''.join(compiled)[:-1]
    outfileName = fileName.split('.')[0] + '.tisa'
    with open(outfileName, 'a') as f:
        f.seek(0)
        f.truncate()
        f.write(compiled)
    print(compiled)
    #for k,v in labels.items():
    #    print(k, v)


lkupReg = [ 'r10','r11','r12','r13','r14','r15',
            'r0','r1','r2','r3','r4','r5','r6','r7','r8','r9',
            'pc','lr','sp', 
            '\n',',','[',']']

hexReg  = [ '0A', '0B', '0C', '0D', '0E', '0F',
            '00','01','02','03','04','05','06','07','08','09',
            '0F','0E','0D',
            '','','','']

mapInst = { 'ldr':'11', 'str':'12', 'ldx':'13', 'stx':'14', 'mov':'21',
            'add':'31', 'sub':'32', 'mul':'33', 'div':'34', 'and':'35', 'orr':'36', 'eor':'37',
            'cmp':'41', 'b':'51',   'beq':'51', 'bne':'51', 'ble':'51', 'blt':'51', 'bge':'51', 'bgt':'51', 'bl':'51' }

def translate(asmInst):
    hexInst  = ['0x','','','','']
    asmInst = asmInst.lower()
    tasmInst = asmInst.split()
    hasreg = False
    if (tasmInst[0] == 'cmp' or tasmInst[0] == 'mov') and tasmInst[2][0] == 'r':
        hasreg = True
    for i in range(len(lkupReg)):
        asmInst = asmInst.replace(lkupReg[i], hexReg[i])
    asmInst = asmInst.split()
    hexInst[1] = mapInst[asmInst[0]]
    hexInst[2] = asmInst[1]
    if asmInst[0] == 'ldr' or asmInst[0] == 'str':
        if asmInst[2][0].isdigit():
            hexInst[3] = asmInst[2]
        elif asmInst[2][0] == "#":
            hexInst[3] = asmInst[2].replace('#', '')
        else:
            hexInst[3] = hex(labels[asmInst[2]])[2:]
        while(len(hexInst[3]) < 4):
            hexInst[3] = '0' +  hexInst[3]
    elif asmInst[0] == 'ldx' or asmInst[0] == 'stx':
        if asmInst[3][0] == '#':
            hexInst[3] = asmInst[3].replace('#', '')
        else:
            hexInst[3] = asmInst[3]
        while(len(hexInst[3]) < 2):
            hexInst[3] = '0' +  hexInst[3]
        hexInst[4] = asmInst[2]
    elif asmInst[0] == 'add' or asmInst[0] == 'sub' or asmInst[0] == 'mul' or asmInst[0] == 'div' or asmInst[0] == 'and' or asmInst[0] == 'orr' or asmInst[0] == 'eor':
        hexInst[3] = asmInst[2]
        hexInst[4] = asmInst[3]
    elif asmInst[0] == 'cmp' or asmInst[0] == 'mov':
        if hasreg:
            hexInst[3] = '80'
        else:
            hexInst[3] = '00'
        if asmInst[2][0] == '#':
            hexInst[4] = asmInst[2].replace('#', '')
        else:
            hexInst[4] = asmInst[2]
        while(len(hexInst[4]) < 2):
            hexInst[4] = '0' +  hexInst[4]
    elif asmInst[0] == 'b' or asmInst[0] == 'beq' or asmInst[0] == 'bne' or asmInst[0] == 'ble' or asmInst[0] == 'blt' or asmInst[0] == 'bge' or asmInst[0] == 'bgt' or asmInst[0] == 'bl':
        if asmInst[0] == 'b':
            hexInst[2] = '00'
        elif asmInst[0] == 'beq':
            hexInst[2] = '01'
        elif asmInst[0] == 'bne':
            hexInst[2] = '02'
        elif asmInst[0] == 'ble':
            hexInst[2] = '03'
        elif asmInst[0] == 'blt':
            hexInst[2] = '04'
        elif asmInst[0] == 'bge':
            hexInst[2] = '05'
        elif asmInst[0] == 'bgt':
            hexInst[2] = '06'
        elif asmInst[0] == 'bl':
            hexInst[2] = '80'
        #hexInst[3] = str(labels[asmInst[1]]) # for int
        if asmInst[1][0].isdigit():
            hexInst[3] = asmInst[1]
        else:
            hexInst[3] = hex(labels[asmInst[1]])[2:] # for hex, hex() returns a string
        while(len(hexInst[3]) < 4):
            hexInst[3] = '0' +  hexInst[3]
    hexInst.append('\n') # newLine
    hexInst = ''.join(hexInst)
    return hexInst

def main(args):
    if os.path.isfile(args[1]):
        do_file(args[1])
    else:
        print('file does not exist')
    return 0

if __name__ == '__main__':
    import sys, os
    sys.exit(main(sys.argv))
