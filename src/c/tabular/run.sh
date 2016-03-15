
./ccscan $1 | python3 -c "import sys;import json;print(json.dumps(json.load(sys.stdin),indent=4))"
