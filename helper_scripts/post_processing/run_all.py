import sys
import os
import glob
import subprocess
import md5
import argparse
out_dir = 'out/'
xml_dir = 'xml/'
generics_dir = 'generics/'
commons_dir = 'common/'

class Struct(object):
    def __init__(self):
        self.vals = []

    def __getitem__(self, idx):
        return self.vals[idx]
        
    def add_val(self, val):
        self.vals.append(val)

class Union(object):
    def __init__(self):
        self.vals = []

    def __getitem__(self, idx):
        return self.vals[idx]

    def add_val(self, val):
        self.vals.append(val)

def my_listdir(path):
    return glob.glob(os.path.join(path, '*'))

def get_devpath(fname):
    marker = 'Device name finder'
    hit = 'Device Name'
    i = 0
    f = open(fname, 'r')
    data = f.read()
    f.close()
    lines = data.splitlines()
    for line in lines:
        if marker in line:
            return None
        elif hit in line:
            name = line[line.rfind(' ')+1::]
            type_line = lines[i+1]
            dev_type = type_line[type_line.rfind(' ')+1::]
            if dev_type == 'proc':
                dev_loc = dev_type
            else:
                dev_loc = '/dev/'
            dev_loc += name
            return dev_loc

        i += 1

    print "Something is weird about the ioctl out file.."
    import ipdb;ipdb.set_trace()
    return None

def get_struct_name(line):
    struct_name = line[line.find('.')+1:line.find('=')-1]
    return struct_name

def get_cmd_val(line):
    val_str = line[line.find(':')+1:line.rfind(':')]

    if 'BR' in line:
        first,second = val_str.split(',')
        if first == second:
            val_str = first
        # TODO: Check with machiry
        else:
            val_str = second

    try:
        val = int(val_str)
        val = abs(val)
    except ValueError:
        print "Bad val_str: %s" % val_str
        import ipdb;ipdb.set_trace()
        sys.exit(1)
    
    return val
        

def emit_records_popping_cmd(popd_cmd, cmd_stack, records, global_type):

    # check if we have a record for our cmd, if so, just return
    for record in records:
        cmd = record[0]
        if popd_cmd == cmd:
            return

    # if not, check our parents records
    parent_had_record = False
    for parent in cmd_stack[:-1:]:
        parent_records = []
        parent_had_record = False
        for record in records:
            cmd = record[0]
            arg = record[1]
            if cmd == parent:
                parent_had_record = True
                new_record = [popd_cmd, arg]
                records.append(new_record)
        if parent_had_record:
            break

    # last hope, check global type
    if parent_had_record == False:
        if global_type is None:
            return
        print '[*] Using global type! %s' % global_type
        new_record = [popd_cmd,global_type]
        records.append(new_record)
        

    return
    
def emit_records_saw_type(cmd_stack, cmd_type, records):
    new_record = [cmd_stack[-1], cmd_type]
    records.append(new_record)
    return


# get preprocessed files. Also serves as a precheck..
def get_pre_procs(lines):
    # first get the start idx
    idx = 0
    for line in lines:
        if 'ALL PREPROCESSED FILES' in line:
            break
        idx += 1
    if idx == len(lines):
        print "[!] Couldn't find preprocessed files!"
        return -1

    if 'Preprocessed' not in lines[idx-1]:
        return -1

    main_pre_proc = lines[idx-1][lines[idx-1].find(":")+1::]

    additional_pre_procs = []
    for line in lines[idx+1:-2:]:
        additional_pre_procs.append(line[line.find(":")+1::])
    
    if main_pre_proc in additional_pre_procs:
        additional_pre_procs.remove(main_pre_proc)
    to_ret = [main_pre_proc]
    to_ret.extend(additional_pre_procs)
    return to_ret
    
def get_ioctl_name_line(all_lines):
    for curr_l in  all_lines:
        if "Provided Function Name" in  curr_l:
            return curr_l
    return None

def algo(fname):
    records = []
    cmd_stack = []
    global_type = None
    cur_cmd = None
    cur_type = None
    in_type = False
    in_anon_type = False

    f = open(fname, 'r')
    data = f.read()
    f.close()
    lines = data.splitlines()
    print '[+] Running on file %s' % fname
    name_line = get_ioctl_name_line(lines)
    if name_line is None:
        name_line = lines[1]
    ioctl_name = name_line[name_line.find(': ')+2::]
    print '[+] ioctl name: %s' % ioctl_name

    pre_proc_files = get_pre_procs(lines)
    if pre_proc_files == -1:
        print "[*] Failed precheck"
        return records, [], ioctl_name

    # probably an uncessary sanity check
    if len(pre_proc_files) == 0:
        print "[*] Failed to find preproc files"
        import ipdb;ipdb.set_trace()

    for line in lines:
        if 'Found Cmd' in line:
            cmd_val = get_cmd_val(line)

            if 'START' in line:
                cmd_stack.append(cmd_val)
                cur_cmd = cmd_val

            elif 'END' in line:
                # the cmd val that's END'ing should always be the top guy on the stack
                if cmd_val != cmd_stack[-1]:
                    print "Fucked up cmd stack state!"
                    import ipdb;ipdb.set_trace()
                popd_cmd = cmd_stack.pop()
                emit_records_popping_cmd(popd_cmd, cmd_stack, records, global_type)
                cur_cmd = None

        elif 'STARTTYPE' in line:
            in_type = True

        # just saw a type, so time to emit a record
        elif 'ENDTYPE' in line:
            # THIS IS POSSIBLE -- IF THE COPY_FROM_USER IS ABOVE THE SWITCH.
            if cur_cmd is None:
                # push 
                # Fuck....global anon
                print 'Setting global type'
                global_type = cur_type
                in_type = False
                cur_type = None
                continue

            if cur_type is None:
                print 'wtf. cur_type is None..'
                import ipdb;ipdb.set_trace()

            emit_records_saw_type(cmd_stack, cur_type, records)
            in_type = False
            cur_type = None

        elif in_type:
            # check if the type is a fuggin anon guy
            if 'anon' in line and 'STARTELEMENTS' in line:
                in_anon_type = True
                if 'struct' in line:
                    cur_type = Struct()
                elif 'union' in line:
                    cur_type = Union()
                else:
                    print "Unknown anon type! %s" % line
                    import ipdb;ipdb.set_trace()

            elif 'anon' in line and 'ENDELEMENTS' in line:
                in_anon_type = False
                pass

            elif in_anon_type:
                cur_type.add_val(line.strip())

            else:
                cur_type = line

    return records, pre_proc_files, ioctl_name

def get_relevant_preproc(struct_name, pre_procs, folder_name):
    found = False
    # a horrible hack..
    for pre in pre_procs:
        stuff = subprocess.Popen(['grep', struct_name, pre], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = stuff.communicate()
        if len(stdout) > 0:
            found = True
            break

    if found == False:
        return -1
    
    # /home/jay/ioctl_stuff/llvm_out_new/drivers/hisi/tzdriver/tc_client_driver.preprocessed
    pre_fname = pre[pre.rfind('/')+1:pre.rfind('.'):]
    # check if we've parsed the found file
    path = out_dir + folder_name + '/' + xml_dir
    abs_path = os.path.abspath(path)

    potential_outfile = abs_path + '/' + pre_fname + '_out.xml'
    if os.path.exists(potential_outfile):
        return potential_outfile
    
    else:
        print '[@] Setting up a new out file: %s' % pre_fname
        return setup_files(pre, folder_name)

    
def setup_files(pre_proc, folder_name):
    out = out_dir + folder_name + '/' + xml_dir
    # yeah yeah, this is pretty stupid but it's a simple unique check since there may be filename collisions
    md5_sum = md5.new(pre_proc).digest().encode('hex')
    output_base_name = pre_proc[pre_proc.rfind('/')+1:pre_proc.rfind('.'):]
    # make sure the xml file exists
    xml_out_path = os.path.abspath(out)
    if os.path.exists(xml_out_path) == False:
        os.mkdir(xml_out_path)

    # first check the commons file
    if os.path.exists(out_dir + '/' + commons_dir + output_base_name + '_' + md5_sum + '_out.xml') == True:
    	print '[+] Found a match in commons!'
        return out_dir + commons_dir + output_base_name + '_' + md5_sum + '_out.xml'
    
    # create the c2xml file
    c2xml_cmd = './c2xml ' + pre_proc + ' > ' + out + output_base_name + '.xml'
    print c2xml_cmd
    os.system(c2xml_cmd)

    # pre_parse the file
    pre_parse_cmd = 'python pre_parse.py ' + out + output_base_name + '.xml'
    print pre_parse_cmd
    os.system(pre_parse_cmd)

    # parse the file
    parse_cmd = 'python parse.py ' + out + output_base_name + '_fixup.xml > ' + out + output_base_name + '_out.xml'
    print parse_cmd
    os.system(parse_cmd)
    out_file = out + output_base_name + '_out.xml'
    
    # cp it to the commons guy
    cp_cmd = 'cp ' + out_file + ' ' + out_dir + '/' + commons_dir + output_base_name + '_' + md5_sum + '_out.xml'
    print cp_cmd
    os.system(cp_cmd)

    return out_file


# TODO: This is slow and stupid, fix it
def pre_check(records):
    for record in records:
        cmd_type = record[1]
        if type(cmd_type) is str:
            if 'struct' in cmd_type or cmd_type in ['i16','i32','i64']:
                return True

    return False

def is_array(cmd_type):
    if '[' in cmd_type and 'x' in cmd_type and ']' in cmd_type:
        return True
    else:
        return False

def process_records(records, pre_procs, folder_name, device_name, dev_num):
    i = 0
    for record in records:
        cmd = record[0]
        cmd_type = record[1]

        file_name = folder_name + '_' + str(i) + '_' + str(cmd) + device_name.replace('/','-') + '.xml'

        # just accept normal structs for now
        if type(cmd_type) is str:
            # normal structs
            if 'struct' in cmd_type:
                struct_name = get_struct_name(cmd_type)
                out_file = get_relevant_preproc(struct_name, pre_procs, folder_name)
                if out_file == -1:
                    print "[&] Couldn't find relevant out file!"
                    import ipdb;ipdb.set_trace()
                # post_parse command
                post_parse_cmd = 'python post_parse.py ' + out_file + ' ' + device_name + ' ' + str(cmd) + ' ' + struct_name + ' > ' + out_dir + folder_name + '/' + file_name
                full_out_file = os.path.abspath(out_dir + folder_name + '/' + file_name)
                print post_parse_cmd
                os.system(post_parse_cmd)
                
                # check if we fucked up
                if os.path.getsize(full_out_file) == 0:
                    print '[-] Created 0 size file! Removing!'
                    os.remove(full_out_file)
                

            # generics
            elif cmd_type in ['i16', 'i32', 'i64']:
                struct_name = 'foo'
                post_parse_cmd = 'python post_parse.py ' + generics_dir + 'generic_' + cmd_type + '.xml ' + device_name + ' ' + str(cmd) + ' ' + struct_name + ' > ' + out_dir + folder_name + '/' + file_name
                full_out_file = os.path.abspath(out_dir + folder_name + '/' + file_name)
                print post_parse_cmd
                os.system(post_parse_cmd)

            # array
            elif is_array(cmd_type):
                struct_name = 'foo'
                file_name = folder_name + '_' + str(i) + '_arr' + device_name.replace('/','-') + '.xml'
                post_parse_cmd = 'python post_parse.py ' + generics_dir + 'generic_arr.xml ' + device_name + ' ' + str(cmd) + ' ' + struct_name + ' > ' + out_dir + folder_name + '/' + file_name
                full_out_file = os.path.abspath(out_dir + folder_name + '/' + file_name)
                print post_parse_cmd
                os.system(post_parse_cmd)
                

        # TODO: we need to create a custom header with either a union or struct
        else:
            pass
        i+=1


def main():
    global out_dir
    parser = argparse.ArgumentParser(description="run_all options")
    parser.add_argument('-f', type=str, help="Filename of the ioctl analysis output OR the entire output directory created by the system", required=True)
    parser.add_argument('-o', type=str, help="Output directory to store the results. If this directory does not exist it will be created", required=True)
    parser.add_argument('-n', type=str, help="Specify devname options. You can choose manual (specify every name manually), auto (skip anything that we don't identify a name for), or hybrid (if we detected a name, we use it, else we ask the user)", default="manual", choices=['manual', 'auto', 'hybrid'])
    parser.add_argument('-m', type=int, help="Enable multi-device output most ioctls only have one applicable device node, but some may have multiple. (0 to disable)", default=1)
    args = parser.parse_args()
    path = args.f
    out_dir = os.path.abspath(args.o)
    name_mode = args.n
    multi_dev = args.m

    if out_dir[-1] != '/':
        out_dir = out_dir + '/'

    if os.path.exists(out_dir) == False:
        print "[+] Creating your out directory for you"
        os.mkdir(out_dir)

    if os.path.isfile(path) == False:
        files = my_listdir(path)
    else:
        files = [path]

    if os.path.exists(out_dir + commons_dir) == False:
        os.mkdir(out_dir + commons_dir)
    print "[+] About to run on %d ioctl info file(s)" % len(files)
    processed_files = []
    algo_dict = {}
    for fname in files:
        if fname == "common":
            continue
        if os.path.isdir(fname):
            print "[^] Hit the V4L2 guy!"
            continue
        # Really we just need the preprocessed file at this point
        records, pre_proc_files, ioctl_name = algo(fname)

        if len(records) > 0:

            # check if device path exists
            if not os.path.exists(out_dir + ioctl_name):
                os.mkdir(out_dir + ioctl_name)
            else:
                print "[!] Skipping %s. out file exists" % ioctl_name
                continue

            # run a pre_check to make sure we have struct or generic args
            if pre_check(records) == False:
                print '[!] Skipping %s. No struct or generic args.' % ioctl_name
                continue

            # if we're running in full automode, make sure the device name has been recovered
            if name_mode == 'auto':
                dev_path = get_devpath(fname)
                if dev_path == None:
                    print '[!] Skipping %s. Name was not recovered and running in auto mode.' % ioctl_name
                    continue

            # setup files. This is done once per device/ioctl
            for record in records:
                struct_type = record[1]
                # nothing to process for generics
                if 'struct' not in struct_type:
                    continue
                get_relevant_preproc(get_struct_name(struct_type), pre_proc_files, ioctl_name)
            #out_file = setup_files(pre_proc_files[0], ioctl_name)
            processed_files.append(fname)
            algo_dict[fname] = (records, pre_proc_files, ioctl_name)

        else:
            print '[!] Skipping %s. No commands found' % ioctl_name


    # pass #2
    for fname in processed_files:
        # parse the output
        records = algo_dict[fname][0]
        pre_proc_files = algo_dict[fname][1]
        ioctl_name = algo_dict[fname][2]

        # good to go
        print '[#] Operating on: %s' % fname

        num_devs = 1
        if multi_dev:
            print "Multiple Devices? (y/n):"
            multi_dev = raw_input("> ")
            if multi_dev == 'y':
                num = raw_input("num devices: ")
                num_devs = int(num)

        for x in range(num_devs):
            # if we're running in manual mode, ask for the device name regardless
            if name_mode == 'manual':
                    print "Please enter a device name:"
                    device_name = raw_input("> ")
            else:
                # if we're in auto mode and we've reached here, the name must exist
                # so no need to distinguish between auto and hybrid
                device_name = get_devpath(fname)
                if device_name == None:
                    print "Please enter a device name:"
                    device_name = raw_input("> ")

            process_records(records, pre_proc_files, ioctl_name, device_name, x)


if __name__ == '__main__':
    main()
