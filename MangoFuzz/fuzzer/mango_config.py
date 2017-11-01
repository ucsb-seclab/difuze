import os
from mango_parser import MangoParser

class MangoConfig(object):
    """
        This is the configuration object for each mango file.
        Given a mango file, this class handles all the
        steps needed for the mango file to juice up.
    """
    def __init__(self, target_engine, target_blender_factory, target_mango_file):
        """
            Create mango config for the provided mango file.
        :param target_engine: The engine object that needs to be used by the config.
        :param target_blender_factory: Blender factory object that needs to be used
                                       for the the process.
        :param target_mango_file: target mango file to process.
        """
        assert(os.path.exists(target_mango_file),
               "Provided mango file:" + str(target_mango_file) + " does not exists.")
        self.target_mango_file = target_mango_file
        self.target_engine = target_engine
        self.target_blender_factory = target_blender_factory

    def process_mango(self):
        """
            Process the provided mango and setup all the structures.
        :return: True if everything goes right else false
        """
        to_ret = False
        curr_parser = MangoParser()
        all_mango_types = curr_parser.Parse(self.target_mango_file, self.target_engine, self.target_blender_factory)
        if len(all_mango_types) > 0:
            to_ret = True
        return to_ret

    def jucify(self):
        """
            Jucify using the provided mango config.
        :return: True if every thing goes fine else false
        """
        to_ret = False
        # TODO: finish this
        return to_ret
