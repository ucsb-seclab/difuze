def get_all_includes(comp_args, dst_includes):
    i = 0
    while i < len(comp_args):
        curr_arg = comp_args[i].strip()
        if curr_arg == "-isystem":
            curr_arg1 = "-I" + comp_args[i+1].strip()
            if curr_arg1 not in dst_includes:
                dst_includes.append(curr_arg1)
        if curr_arg == "-include":
            curr_arg1 = comp_args[i+1].strip()
            if "dhd_sec_feature.h" not in curr_arg1:
                final_arg = curr_arg + " " + curr_arg1
                if final_arg not in dst_includes:
                    dst_includes.append(final_arg)
        if curr_arg[0:2] == "-I":
            if curr_arg not in dst_includes:
                if 'drivers' not in curr_arg and 'sound' not in curr_arg:
                    dst_includes.append(curr_arg)
        i += 1
