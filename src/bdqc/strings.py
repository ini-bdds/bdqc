
def common_prefix( *args ):
	if len(args) < 1:
		return 0
	head = args[0]
	assert isinstance(head,str)
	N = len(head)
	i = 0
	while i < N:
		for s in args[1:]:
			assert isinstance(s,str)
			if len(s) <= i or s[i] != head[i]:
				return i
		i += 1
	return i

# Unit test
if __name__ == "__main__":
	import sys
	print( sys.argv[1][:common_prefix( *sys.argv[1:] )] )
