from components.base_component import log_success, log_info, log_error, log_warning
import os
import sys
import json
from multiprocessing import Pool, cpu_count


class TypeLines(object):
    def __init__(self):
        self.all_lines = []
        self.constraints = {}

    def add_type_line(self, curr_line):
        self.all_lines.append(curr_line)

    def is_same(self, other_type):
        if other_type is not None:
            if len(self.all_lines) == len(other_type.all_lines):
                i = 0
                while i < len(self.all_lines):
                    if self.all_lines[i] != other_type.all_lines[i]:
                        return False
                    i += 1
                return True
        return False

    def get_as_json(self):
        to_ret = "{\"type_info\":" + json.dumps(self.all_lines)
        if len(self.constraints) > 0:
            to_ret += ", \"constraints\":" + json.dumps(self.constraints)
        to_ret += "}"
        return to_ret


class IoctlCmd(object):
    def __init__(self, cmd_val):
        self.cmd_val = cmd_val
        self.possible_types = []

    def add_possible_type(self, curr_type):
        for curr_t in self.possible_types:
            if curr_t.is_same(curr_type):
                return
        self.possible_types.append(curr_type)

    def get_as_json(self):
        to_ret = "{\"cmd_info\":{\"cmd_val\":" + json.dumps(self.cmd_val) + ", \"possible_types\":["
        add_comma = False
        for curr_type in self.possible_types:
            if add_comma:
                to_ret += ",\n"
            to_ret += curr_type.get_as_json()
            add_comma = True
        to_ret += "]}}"
        return to_ret


class EntryPointObject(object):
    def __init__(self, entry_point_name):
        self.ep_name = entry_point_name
        self.possible_cmds = []
        self.dev_type = ''
        self.dev_name = "UNIDENTIFIED"
        self.all_preprocessed_files = []

    def add_possible_cmd(self, curr_cmd):
        self.possible_cmds.append(curr_cmd)

    def get_as_json(self):
        to_ret = "{\"entry_point_name\":" + json.dumps(self.ep_name) + ", \"device_type\":" + json.dumps(self.dev_type)
        to_ret += ", \"device_name\":" + json.dumps(self.dev_name) + ", \"commands\":["
        add_comma = False
        for curr_cmd in self.possible_cmds:
            if add_comma:
                to_ret += ",\n"
            to_ret += curr_cmd.get_as_json()
            add_comma = True
        to_ret += "],"
        to_ret += "\"all_pre_processed_files\":" + json.dumps(self.all_preprocessed_files)
        to_ret += "}"
        return to_ret


def parse_device_info(all_lines, ep_object):
    target_lines = []
    st_idx = -1
    curr_idx = 0
    en_idx = -1
    for curr_l in all_lines:
        if curr_l.startswith("[+] Provided Function Name"):
            st_idx = curr_idx + 1
            break
        curr_idx += 1

    if st_idx != -1:
        en_idx = st_idx
        while en_idx < len(all_lines):
            if all_lines[en_idx].startswith("Provided Function Name"):
                break
            en_idx += 1

    if st_idx != -1 and en_idx != -1 and en_idx < len(all_lines):
        target_lines = all_lines[st_idx:en_idx]
        target_dev_type = None
        target_dev_name = None
        for curr_l in target_lines:
            if curr_l.startswith('[+] Device Type:'):
                target_dev_type = curr_l.split('[+] Device Type:')[-1].strip()
            if curr_l.startswith('[+] V4L2 Device:'):
                tmp_dev_name = curr_l.split('[+] V4L2 Device:')[-1].strip()
                if target_dev_name is not None:
                    target_dev_name += ", dev_name:" + tmp_dev_name
                else:
                    target_dev_name = tmp_dev_name
            if curr_l.startswith('[+] Look into:'):
                tmp_dev_name = curr_l.split('[+] Look into:')[-1].strip()
                if target_dev_name is not None:
                    target_dev_name += ", dynamic_info:" + tmp_dev_name
                else:
                    target_dev_name = tmp_dev_name
            if curr_l.startswith('[+] Device Name:'):
                target_dev_name = curr_l.split('[+] Device Name:')[-1].strip()
        if target_dev_type is None:
            log_warning("Unable to find device type.")
        else:
            ep_object.dev_type = target_dev_type
        if target_dev_name is None:
            log_warning("Unable to find device name.")
        else:
            ep_object.dev_name = target_dev_name
        first_split = all_lines[0:st_idx]
        second_split = all_lines[en_idx:]
        all_lines = first_split + second_split

    return all_lines


def parse_type(curr_idx, all_lines):
    to_ret = None
    if all_lines[curr_idx].startswith('STARTTYPE'):
        to_ret = TypeLines()
        curr_idx += 1
        while not all_lines[curr_idx].startswith('ENDTYPE'):
            to_ret.add_type_line(all_lines[curr_idx])
            curr_idx += 1
        if all_lines[curr_idx].startswith('ENDTYPE'):
            curr_idx += 1
    return curr_idx, to_ret


def get_cmd_val(curr_cmd_line):
    if 'BR' in curr_cmd_line:
        cmd_val = curr_cmd_line.split(',')[1].split(":")[0]
    else:
        cmd_val = curr_cmd_line.split(':')[1]
    return cmd_val


def parse_cmd_start(curr_idx, all_lines):
    to_ret = None
    curr_cmd_line = all_lines[curr_idx]
    if 'Cmd' in curr_cmd_line and curr_cmd_line.endswith('START'):
        cmd_val = get_cmd_val(curr_cmd_line)
        to_ret = IoctlCmd(cmd_val)
        curr_idx += 1
    return curr_idx, to_ret


def parse_cmd_end(curr_idx, all_lines, curr_cmd):
    curr_cmd_line = all_lines[curr_idx]
    if 'Cmd' in curr_cmd_line and curr_cmd_line.endswith('END'):
        cmd_val = get_cmd_val(curr_cmd_line)
        assert curr_cmd.cmd_val == cmd_val
        curr_idx += 1
    return curr_idx, curr_cmd


def parse_field_constraints(curr_idx, all_lines):
    curr_line = all_lines[curr_idx]
    all_constraints = {}
    if curr_line.startswith("field:"):
        fld_id = curr_line.split("field:")[-1].strip()
        possible_values = []
        curr_idx += 1
        while curr_idx < len(all_lines):
            curr_line = all_lines[curr_idx]
            if curr_line.startswith("POSSIBLE VALUES"):
                curr_idx += 1
                pass
            elif 'VALUE' in curr_line or 'Value' in curr_line:
                target_val = curr_line.split(":")[-1]
                possible_values.append(target_val)
                curr_idx += 1
            else:
                break
        all_constraints[fld_id] = possible_values
    return curr_idx, all_constraints


def parse_common_constraints(curr_idx, all_lines):
    curr_line = all_lines[curr_idx]
    all_constraints = {}
    if curr_line.startswith("POSSIBLE VALUES"):
        fld_id = "0"
        possible_values = []
        curr_idx += 1
        while curr_idx < len(all_lines):
            curr_line = all_lines[curr_idx]
            if 'VALUE' in curr_line:
                target_val = curr_line.split(":")[-1]
                possible_values.append(target_val)
                curr_idx += 1
            else:
                break
        all_constraints[fld_id] = possible_values
    return curr_idx, all_constraints


def adjust_type_for_obj(curr_obj, parent_objs):
    if len(curr_obj.possible_types) == 0:
        curr_pa = len(parent_objs) - 1
        while curr_pa >= 0:
            curr_pobj = parent_objs[curr_pa]
            if len(curr_pobj.possible_types) > 0:
                curr_obj.possible_types.extend(curr_pobj.possible_types)
                break
            curr_pa -= 1


def usage():
    log_error("Invalid Usage.")
    log_info("This script converts the crazy output of Interface Recovery pass into nice json files.")
    log_info("Use this to get the interface of the driver in a clean and consistent format.")
    log_error("Run: python ", __file__, " <ioctl_finder_out_dir> <output_json_dir>")
    sys.exit(-1)


def process_output_file((ioctl_finder_out, output_json)):
    # read all lines
    ret_val = True
    log_info("Trying to process file:", ioctl_finder_out)
    fp = open(ioctl_finder_out, "r")
    all_lines = map(lambda x: x.strip(), fp.readlines())
    fp.close()
    try:
        # may lead to an exception, ignore
        ep_name = os.path.basename(ioctl_finder_out)[:-4]
        ep_obj = EntryPointObject(ep_name)
        all_lines = parse_device_info(all_lines, ep_obj)

        curr_idx = 0
        cmd_stack = []
        global_types = []
        while curr_idx < len(all_lines):
            curr_line = all_lines[curr_idx]
            if curr_line.endswith('START'):
                curr_idx_new, new_cmd = parse_cmd_start(curr_idx, all_lines)
                cmd_stack.append(new_cmd)
                ep_obj.add_possible_cmd(new_cmd)
                assert curr_idx_new > curr_idx
                curr_idx = curr_idx_new
            elif curr_line.endswith('END'):
                curr_idx_new, _ = parse_cmd_end(curr_idx, all_lines, cmd_stack[-1])
                # adjust the types here
                adjust_type_for_obj(cmd_stack[-1], cmd_stack[0:-1])
                # pop current cmd off command stack.
                cmd_stack = cmd_stack[:-1]
                assert curr_idx_new > curr_idx
                curr_idx = curr_idx_new
            elif curr_line.startswith('STARTTYPE'):
                curr_idx_new, target_type = parse_type(curr_idx, all_lines)
                if len(cmd_stack) == 0:
                    global_types.append(target_type)
                else:
                    cmd_stack[-1].add_possible_type(target_type)
                assert curr_idx_new > curr_idx
                curr_idx = curr_idx_new
            elif 'Preprocessed file:' in curr_line:
                all_pre_files = curr_line.split('Preprocessed file:')
                if len(all_pre_files) > 0:
                    curr_p_file = all_pre_files[-1].strip()
                    if len(curr_p_file) > 0:
                        ep_obj.all_preprocessed_files.append(curr_p_file)
                curr_idx += 1
            elif curr_line.startswith('field:'):
                curr_idx_new, target_constrains = parse_field_constraints(curr_idx, all_lines)
                assert curr_idx_new > curr_idx
                curr_idx = curr_idx_new
                if len(target_constrains) > 0:
                    global_types[-1].constraints.update(target_constrains)
            elif curr_line.startswith('POSSIBLE VALUE'):
                curr_idx_new, target_constrains = parse_common_constraints(curr_idx, all_lines)
                assert curr_idx_new > curr_idx
                curr_idx = curr_idx_new
                if len(target_constrains) > 0:
                    global_types[-1].constraints.update(target_constrains)
            else:
                curr_idx += 1

        # adjust global commands
        for curr_cmd in ep_obj.possible_cmds:
            if len(curr_cmd.possible_types) == 0:
                curr_cmd.possible_types.extend(global_types)

        # adjust constraints
        for curr_glob_type in global_types:
            if len(curr_glob_type.constraints) > 0:
                processed_types = list()
                processed_types.append(curr_glob_type)
                for curr_cmd in ep_obj.possible_cmds:
                    for curr_type in curr_cmd.possible_types:
                        if curr_glob_type.is_same(curr_type):
                            if curr_type not in processed_types:
                                processed_types.append(curr_type)
                                curr_type.constraints.update(curr_glob_type.constraints)

        fp = open(output_json, "w")
        fp.write(ep_obj.get_as_json())
        fp.close()
    except Exception as e:
        ret_val = False
        log_error("Error occurred while trying to process file:", ioctl_finder_out)

    if ret_val:
        log_success("Successfully processed the file:", ioctl_finder_out)
    return ioctl_finder_out, ret_val


def get_all_files(curr_dir, curr_out_dir):
    to_process = []
    for curr_f in os.listdir(curr_dir):
        if curr_f.endswith('.txt'):
            ep_name = curr_f[:-4]
            if not os.path.isdir(curr_out_dir):
                os.makedirs(curr_out_dir)
            output_json_file = os.path.join(curr_out_dir, ep_name + ".json")
            curr_f_path = os.path.join(curr_dir, curr_f)
            to_process.append((curr_f_path, output_json_file))
        elif os.path.isdir(os.path.join(curr_dir, curr_f)):
            dst_out_dir = os.path.join(curr_out_dir, curr_f)
            child_proc_files = get_all_files(os.path.join(curr_dir, curr_f), dst_out_dir)
            to_process.extend(child_proc_files)

    return to_process


def main():
    if len(sys.argv) < 3:
        usage()

    ioctl_finder_out_dir = sys.argv[1]
    output_json_dir = sys.argv[2]

    if not os.path.exists(ioctl_finder_out_dir) and os.path.isdir(ioctl_finder_out_dir):
        log_error("Provided Ioctl finder out folder:", ioctl_finder_out_dir, "does not exist.")
        sys.exit(-2)

    if not os.path.exists(output_json_dir):
        os.makedirs(output_json_dir)

    to_process = get_all_files(ioctl_finder_out_dir, output_json_dir)

    log_info("Trying to process:", len(to_process), "ioctl out files in multiprocessing file.")

    p = Pool(cpu_count())
    all_output_res = p.map(process_output_file, to_process)
    log_success("Finished processing all ioctl out files.")

    all_okay = True
    for curr_f, curr_res in all_output_res:
        if not curr_res:
            all_okay = False
            log_error("Error occurred while trying to process:", curr_f)

    log_success("All output files are in:", output_json_dir)
    if all_okay:
        log_success("ALL OKAY!!")


if __name__ == "__main__":
    main()
