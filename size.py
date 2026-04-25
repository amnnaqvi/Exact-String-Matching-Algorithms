import os

file_path = r"data\pizza\english.txt"

size_bytes = os.path.getsize(file_path)
size_mb = size_bytes / (1024 * 1024)

print("Size in bytes:", size_bytes)
print("Size in MB:", size_mb)