#!/usr/bin/env python

import sys
import os
import platform

all_packages = [
    {"lib": "Poco", "version": "1.9.1OpenSSL", "path": "1602/stable", "shared": False}
]

print(sys.argv)
features = []
for parameter in sys.argv:
    if ("with-" in parameter):
        params = parameter.split("-")
        print("conanfile for feature: %s" % params[-1])
        features.append(params[-1])
    else:
        pass
features.append("default")

curSystem = platform.system()
print("current system: ", curSystem)

conanfile = "[requires]\r\n"
for item in all_packages:
    version = None

    if(item["lib"] == "live555") :
        if(curSystem != "Linux" and curSystem != "Windows") :
            continue

    if (not isinstance(item["version"], dict)):
        version = item["version"]
    else:
        for feature in features:
            if (feature in item["version"]):
                version = item["version"][feature]
                break
    
    if (version != None):    
        conanfile = conanfile + ("%s/%s@%s\r\n" % (item["lib"], version, item["path"]))

conanfile = conanfile + "\r\n[generators]\r\ncmake_multi\r\n\r\n[options]\r\n"
for item in all_packages:
    if ("shared" in item):
        conanfile = conanfile + ("%s:shared=%s\r\n" % (item["lib"], item["shared"]));

filepath = os.path.dirname(os.path.abspath(__file__))
cananfile_handle = open(os.path.join(filepath, "conanfile.txt"), "w")
cananfile_handle.write(conanfile)
cananfile_handle.close()
