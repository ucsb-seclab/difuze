
class Mapping(object):
	def __init__(self, blob_id, src_id, src_idx, dst_idx, offset):
		self.src_id = src_id
		self.blob_id = blob_id
		self.src_idx = src_idx
		self.dst_idx = dst_idx
		self.offset = offset

	def show(self):
		print "Src idx:", self.src_idx
		print "Dst idx:", self.dst_idx
		print "Offset:", self.offset
		print "Src id: " + self.src_id
		print "Dst id: " + self.blob_id
