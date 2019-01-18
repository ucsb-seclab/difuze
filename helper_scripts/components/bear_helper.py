import json
import collections
import os
from base_component import log_error

"""
Handles parsing json file produced by the Bear
"""

CompilationCommand = collections.namedtuple("CompilationCommand", ["curr_args", "work_dir", "src_file", "output_file"])
LinkerCommand = collections.namedtuple("LinkerCommand", ["curr_args", "work_dir", "input_files", "output_file"])


def parse_compile_json(json_file_path):
    """
        Parse the json file output of Bear
    :param json_file_path: Path to the output json file of Bear
    :return: pair of compilation and linker commands.
    """
    compile_commands = []
    linker_commands = []
    if os.path.exists(json_file_path):
        try:
            fp = open(json_file_path, "r")
            all_cont = fp.read()
            fp.close()
            json_obj = json.loads(all_cont)
            # it contains array of commands
            for curr_command in json_obj:
                curr_args = curr_command["arguments"]
                i = 0
                # convert each string in the argument
                # into a python friendly escaped string.
                while i < len(curr_args):
                    cura = curr_args[i]
                    if '="' in cura:
                        cn = cura.index('="')
                        curr_args[i] = cura[0:cn+1] + "'" + cura[cn+1:]
                        curr_args[i] = curr_args[i] + "'"
                    i += 1

                work_dir = curr_command["directory"]
                output_file = curr_command["output"][-1]
                if "loader" not in curr_command:
                    src_file = curr_command["file"]
                    if "drivers/" in src_file:
                        compile_commands.append(CompilationCommand(curr_args, work_dir, src_file, output_file))
                else:
                    input_files = curr_command["files"]
                    if "drivers/" in output_file:
                        linker_commands.append(LinkerCommand(curr_args, work_dir, input_files, output_file))
        except Exception as e:
            log_error("Error occurred while trying to parse provided json file", json_file_path, "error", e.message)
    else:
        log_error("Provided json file doesn't exist", json_file_path)
    return compile_commands, linker_commands
