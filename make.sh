#!/bin/sh
#For all Linux users out there. I don't think anything uses the Asar library on Linux, but it does no harm.
g++ -std=c++98 -Wall -Werror -Wno-unused-result -Wno-uninitialized *.cpp -oasar -g -fno-rtti -DINTERFACE_CLI -Dstricmp=strcasecmp -Dlinux -DRELEASE
g++ -std=c++98 -fPIC -Wall -Werror -Wno-unused-result -Wno-uninitialized *.cpp -olibasar.so -g -fno-rtti -DINTERFACE_LIB -Dstricmp=strcasecmp -Dlinux -DRELEASE -shared
