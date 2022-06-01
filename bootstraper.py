import sys
import os
import subprocess
import errno
import re

class MaxMEBootstraper:
  target = None
  build_folder = None
  build = False
  src_dir = os.getcwd()

  static_libgcc = False

  build_component = 'full'

  if ('ANDROID_NDK_HOME' in os.environ):
    android_ndk = os.environ["ANDROID_NDK_HOME"]
  else:
    android_ndk = ""

  target_configs = {
    'osx':{'generator':'Xcode', 'options':['CMAKE_OSX_SYSROOT=macosx', 'CMAKE_OSX_ARCHITECTURES=x86_64', 'MACOS=ON', 'WITH_RECORD=ON']},
    'win':{'generator':'Visual Studio 15 2017', 'options':['WITH_RECORD=ON']},
    'win64':{'generator':'Visual Studio 15 2017 Win64', 'options':['WITH_RECORD=ON']},
    'win2015':{'generator':'Visual Studio 14 2015', 'options':['WITH_RECORD=ON']},
    'android':{'generator':'Unix Makefiles', 'options':['ANDROID_TOOLCHAIN=gcc', 'ANDROID_STL=gnustl_static', 'ANDROID_ABI=armeabi-v7a', 'ANDROID_NATIVE_API_LEVEL=21', 'CMAKE_TOOLCHAIN_FILE=%s'%(os.path.join(android_ndk, "build/cmake/android.toolchain.cmake"))]},
    'android64':{'generator':'Unix Makefiles', 'options':['ANDROID_TOOLCHAIN=gcc', 'ANDROID_STL=gnustl_static', 'ANDROID_ABI=arm64-v8a', 'ANDROID_NATIVE_API_LEVEL=21', 'CMAKE_TOOLCHAIN_FILE=%s'%(os.path.join(android_ndk, "build/cmake/android.toolchain.cmake"))]},
    'ios':{'generator':'Xcode', 'options':['CMAKE_OSX_SYSROOT=iphoneos', 'CMAKE_OSX_ARCHITECTURES=arm64', 'IOS=ON', 'CMAKE_TOOLCHAIN_FILE=%s'%(os.path.join(src_dir, "vendor/cmake/ios.cmake"))]},
    'unix':{'generator':'Unix Makefiles', 'options':['CMAKE_C_FLAGS=-fPIC', 'CMAKE_CXX_FLAGS=-fPIC', 'CMAKE_SHARED_LINKER_FLAGS="-Wl,-Bsymbolic"', 'MAXME_UNIX=ON']},
    'linux':{'generator':'Unix Makefiles', 'options':['CMAKE_C_FLAGS=-fPIC', 'CMAKE_CXX_FLAGS=-fPIC', 'CMAKE_SHARED_LINKER_FLAGS="-Wl,-Bsymbolic"', 'MAXME_LINUX=ON', 'WITH_KITCHENSINK=OFF']},
    'hisilicon':{'generator':'Unix Makefiles','options':['ANDROID_TOOLCHAIN=gcc', 'ANDROID_STL=gnustl_static', 'ANDROID_ABI=armeabi-v7a', 'ANDROID_NATIVE_API_LEVEL=19', 'SCREEN_STREAM_SDK=ON', 'CMAKE_TOOLCHAIN_FILE=%s'%(os.path.join(android_ndk, "build/cmake/android.toolchain.cmake"))]},
    'hisiv500':{'generator':'Unix Makefiles','options':['HISI_ARM=ON', 'HISI_TEST=ON', 'CMAKE_C_FLAGS=-fPIC', 'CMAKE_CXX_FLAGS=-fPIC', 'CMAKE_SHARED_LINKER_FLAGS="-Wl,-Bsymbolic,-Map=out.map"', 'UCLIBC=ON', 'CMAKE_TOOLCHAIN_FILE=%s'%(os.path.join(src_dir, "vendor/cmake/hisiv500.cmake"))]},
    'hisiv300':{'generator':'Unix Makefiles','options':['HISI_ARM=ON', 'HISI_TEST=ON','CMAKE_C_FLAGS=-fPIC', 'CMAKE_CXX_FLAGS=-fPIC', 'CMAKE_SHARED_LINKER_FLAGS="-Wl,-Bsymbolic,-Map=out.map"', 'UCLIBC=ON', 'CMAKE_TOOLCHAIN_FILE=%s'%(os.path.join(src_dir, "vendor/cmake/hisiv300.cmake"))]},
  }

  compile_config = {
    'osx' : { 
      'Debug' : ['-configuration Debug', '-target "ALL_BUILD"', 'DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"', '-UseModernBuildSystem=NO'], 
      'Release' : ['-configuration Release', '-target "ALL_BUILD"', 'DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"', '-UseModernBuildSystem=NO']
    },
    'ios' : {
      'Debug' : ['-configuration Debug', '-target "ALL_BUILD"', 'DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"', '-UseModernBuildSystem=NO'], 
      'Release' : ['-configuration Release', '-target "ALL_BUILD"', 'DEBUG_INFORMATION_FORMAT="dwarf-with-dsym"', '-UseModernBuildSystem=NO']
    }
  }

  sign_config = {
    'osx' : {
      'Debug' : ['CODE_SIGN_IDENTITY="Mac Developer: Jingzhi Wu (84R7NLM3AZ)"', 'PROVISIONING_PROFILE=""', 'PRODUCT_BUNDLE_IDENTIFIER="vip.maxhub.maxme"'],
      'Release' : ['CODE_SIGN_IDENTITY="3rd Party Mac Developer Application: Jingzhi Wu (SGQF43Z5BM)"', 'PROVISIONING_PROFILE=""', 'PRODUCT_BUNDLE_IDENTIFIER="vip.maxhub.maxme"']
    },
    'ios' : { 
      'Debug' : ['CODE_SIGN_IDENTITY="Developer ID Application: Guangzhou Shizhen Information Technology Co., Ltd. (2ZNMX3X6V3)"', 'PROVISIONING_PROFILE=""', 'PRODUCT_BUNDLE_IDENTIFIER="vip.maxhub.maxme"'],
      'Release' : ['CODE_SIGN_IDENTITY="iPhone Distribution: Guangzhou Shizhen Information Technology Co., Ltd. (2ZNMX3X6V3)"', 'PROVISIONING_PROFILE=""', 'PRODUCT_BUNDLE_IDENTIFIER="vip.maxhub.maxme"']
    }
  }


  def __init__(self, target, build_folder, build, debug):
    self.target = target 
    tempTarget=target
    if(target in ["win","win2015"]):
      tempTarget="win32"
    self.build_folder = os.path.join(build_folder, tempTarget)
    self.build = build
    self.debug = debug

  def mkdir_p(self):
    try:
        os.makedirs(self.build_folder)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(self.build_folder):
            pass
        else:
            raise('Can not create directory.')
  

  def remove_cache_p(self):
    try:
        makecachefile = self.build_folder + "/CMakeCache.txt"
        os.remove(makecachefile)
        print("remove cmake cache flie:" , makecachefile)
    except OSError as exc:  # Python >2.5
        pass

  def setEnv(self):
    if (self.target == 'hisiv500'):
      os.environ["CMAKE_TOOLCHAIN_FILE_NAME"] = "hisiv500.cmake" # set environment for thirdparty library when install with 'conan install --build'
    elif (self.target == 'hisiv300'):
      os.environ["CMAKE_TOOLCHAIN_FILE_NAME"] = "hisiv300.cmake"
      os.environ["CFLAGS"]="-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations -w -fpic"
      os.environ["CXXFLAGS"]="-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations -w -fpic"


  def run(self):

    os.system("git submodule update --init --recursive ")

    self.mkdir_p()
    self.remove_cache_p()
    os.chdir(self.build_folder)

    status = self.gen_project_files()
    if (0 != status):
      raise('Generate project files failed.')

    if (self.build):
      status = self.build_project()
      if (0 != status):
        raise('Build project failed.')

  def gen_project_files(self):
    cmd = 'cmake %s -G "%s"'%(self.src_dir, self.target_configs[self.target]['generator'])
    optprefix = '-D'
    
    options = self.target_configs[self.target]['options']
    options.append('BUILD_SDK=ON')
    
    if (self.debug):
      options.append('CMAKE_BUILD_TYPE=Debug')
    else:
      options.append('CMAKE_BUILD_TYPE=Release')


    for opt in options:
      cmd += ' %s%s'%(optprefix, opt)

    print('Use this cmake command to generate project files: %s'%cmd)
    return subprocess.call(cmd, shell=True)


  def build_project(self):
    build_type = 'Release'
    if (self.debug):
      build_type = 'Debug'

    cmd = 'cmake --build . --config ' + build_type

    if (self.runtime == 'electron') :
      cmd += ' --target maxme-electron'
    elif (self.runtime == 'nwjs') :
      cmd += ' --target maxme-nw'

    # multithread compilation
    if (self.compiler_cores > 0 and (self.target == 'win' or self.target == 'win64')):
        cmd += ' -- /maxcpucount:' + str(self.compiler_cores)

    print("calling build cmd: "+cmd)
    status = subprocess.call(cmd, shell=True)
    return status
    
    
