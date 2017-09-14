import os

def rucioMakeList(dsname):
    l = os.popen("rucio list-file-replicas --protocol root "+dsname+" | grep MWT2_UC_LOCALGROUPDISK")
    f = open("temp.txt","w")
    for line in l:
        line = line.split("MWT2_UC_LOCALGROUPDISK: ")[-1]
        line = line.split()[0]
        f.write(line+"\n")
        print(line)
    f.close()
