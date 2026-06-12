from pathlib import Path
import shutil
import os
import stat



Import("env")

text = env.GetProjectOption("Firmware-Variant")


print("\nStarting precompiler script")
print("------------------------------------------------------------")
print("Folder name from platformio.ini -> Firmware-Variant:", text)



SOURCEFolder = text

SOURCE = Path("Conversation")/SOURCEFolder
DEST = Path("src/Adaptability")

REQUIRED_FILES = [
    "Binder.cpp",
    "UnPacker.cpp",
    "UnPacker.h",
]
# Validate folders
if not SOURCE.exists():
    raise RuntimeError(f"Source folder does not exist: {SOURCE}")

if not SOURCE.is_dir():
    raise RuntimeError(f"Source is not a directory: {SOURCE}")

if not DEST.exists():
    raise RuntimeError(f"Destination folder does not exist: {DEST}")

if not DEST.is_dir():
    raise RuntimeError(f"Destination is not a directory: {DEST}")

# Validate required files exist in source
missing = [
    filename
    for filename in REQUIRED_FILES
    if not (SOURCE / filename).is_file()
]

if missing:
    raise RuntimeError(
        f"Missing required files in {SOURCE}: {', '.join(missing)}"
    )

def remove_readonly(path):
    os.chmod(path, stat.S_IWRITE)

# Clear destination contents
for item in DEST.iterdir():
    if item.is_dir():
        shutil.rmtree(
            item,
            onerror=lambda func, path, excinfo: (
                print("removeing "+str(path)),
                remove_readonly(path),
                func(path)
            )
        )
    else:
        print("removeing "+ item.name)
        remove_readonly(item)
        item.unlink()

# Copy source contents
for item in SOURCE.iterdir():
    target = DEST / item.name

    if item.is_dir():
        shutil.copytree(item, target)
    else:
        shutil.copy2(item, target)

# Create an additional file

generated_file = DEST / (SOURCEFolder + ".TITLE")

generated_file.write_text(
    'Do not edit the content of this folder directly instead modify its parent in '+str(SOURCE)+'\n',
    encoding="utf-8"
)

# Make everything read-only
for file in DEST.rglob("*"):
    if file.is_file():
        file.chmod(0o444)



print("Ending precompiler script")
print("------------------------------------------------------------\n")