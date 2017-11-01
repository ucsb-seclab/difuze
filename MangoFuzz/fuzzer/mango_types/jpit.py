from data_guys import DataModel
from mapping import Mapping
from ..utils import rotten_peel


class JPit(object):
	def __init__(self, name):
		self.name = name
		self.main_struct = None
		self.data_models = []
		self.ioctl_id = None
		self.devname = None
		self.target_struct = None

	def __getitem__(self, idx):
		return self.data_models[idx]

	def __len__(self):
		return len(self.data_models)

	def findDataModel(self, dm_name):
		for dm in self.data_models:
			if dm.name == dm_name:
				return dm
		return None

	def addChild(self, dm):
		if isinstance(dm, DataModel):
			self.data_models.append(dm)
			dm.parent = self
		else:
			rotten_peel("Trying to add a non DataModel to a JPit")

	def run(self):
		assert self.target_struct is not None
		dm_to_gen = self.findDataModel(self.target_struct)
		if dm_to_gen is None:
			rotten_peel("Couldn't find DataModel: %s when trying to Run!", self.target_struct)
		data, blobs, data_mappings, blob_mappings =  dm_to_gen.generate()

		blob_arr = []
		id_idx = {}
		blob_arr.append(data)
		i = 1
		for b_id in blobs:
			blob = blobs[b_id]
			id_idx[b_id] = i
			blob_arr.append(blob)
			i += 1

		map_arr = []
		for offset in data_mappings:
			# main guy
			src_id = self.target_struct
			blob_id = data_mappings[offset]
			dst_idx = id_idx[blob_id]
			src_idx = 0
			entry = Mapping(blob_id, src_id, src_idx, dst_idx, offset)
			map_arr.append(entry)

		for blob_id in blob_mappings:
			for offset in blob_mappings[blob_id]:
				src_id = blob_id
				src_idx = id_idx[blob_id]
				dst_id = blob_mappings[blob_id][offset]
				dst_idx = id_idx[dst_id]
				entry = Mapping(dst_id, src_id, src_idx, dst_idx, offset)
				map_arr.append(entry)

		return blob_arr, map_arr
