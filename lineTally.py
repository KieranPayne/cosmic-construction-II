#quick script to find the number of lines of code in the src folder
import os
directory = os.getcwd() + "\\src"
total_lines = 0
paths = []
lines = []
def linesPerFile():
	#sort the files from largest to smallest
	for i in range(len(lines)):
		for j in range(1,len(lines) - i):
			if lines[j-1] < lines[j]:
				lines[j],lines[j-1] = lines[j-1],lines[j]
				paths[j],paths[j-1] = paths[j-1],paths[j]
	for i in range(len(paths)):
		print(paths[i],"has",lines[i],"lines (" + str(round(lines[i]/total_lines * 100,1))+"%)")
for root, dirs, files in os.walk(directory):
	#ignore any of the code i didnt write, not trying to steal credit
	if "Platform" in root or "Utility" in root or "box2d" in root or "imgui" in root:
		continue
	for file in files:
		file_path = os.path.join(root,file)
		if "json" in str(file_path) or "PerlinNoise" in str(file_path):
			continue
		with open(file_path,'r', encoding = "utf-8", errors="ignore") as f:
			total = len(f.readlines())
			paths.append(file_path[len(directory)+1:])
			lines.append(total)
			total_lines += total
linesPerFile()

print("Directory contains",total_lines, "lines of code across",len(paths),"files")
