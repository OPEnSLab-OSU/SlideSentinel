import os
import shutil
import tempfile
Import("env")

def package_release(source, target, env):
    BIN_PATH = os.path.join(env.subst("$PROJECT_DIR"), "binaries")
    SOURCE_PATH = env.subst("$BUILD_DIR")
    PROJECT_NAME = env.subst("$PROGNAME")
    ARCHIVE_PATH = os.path.join(env.subst("$PROJECT_DIR"), "SSRover")
    with tempfile.TemporaryDirectory() as dir_name: 
        shutil.copyfile(os.path.join(SOURCE_PATH, f'{PROJECT_NAME}.elf'), os.path.join(dir_name,  f'{PROJECT_NAME}.elf'))
        shutil.copyfile(os.path.join(SOURCE_PATH, f'{PROJECT_NAME}.bin'), os.path.join(dir_name,  f'{PROJECT_NAME}.bin'))
        shutil.make_archive(ARCHIVE_PATH, 'zip', dir_name)

env.Replace(UPLOADCMD=package_release)