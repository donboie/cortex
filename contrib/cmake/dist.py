import subprocess
p = subprocess.Popen(['/usr/bin/lsb_release', '-a'],  stderr=subprocess.PIPE, stdout=subprocess.PIPE)
stdout, stderr = p.communicate()
lines = stdout.split('\n')

info = {}
for line in lines:
	parts = line.split(':')
	if len(parts) > 1:
		info[parts[0]] = parts[1].strip()

result = "unknown"
if "Distributor ID" in info and "cent" in info["Distributor ID"].lower():
	result = "cent"

if "Release" in info:
	result += info["Release"].split('.')[0]

print result

