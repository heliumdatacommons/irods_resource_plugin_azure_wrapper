# irods_resource_plugin_azure_wrapper
This directory contains a wrapper that mediates between the irods azure resource plugin on one side and
the calls to the libazure storage library on the other side.  The purpose of this is to eliminate link
depencies on the c++ stdlib, which allows us to link our resource driver (built with clang) with the rest of the environment (build with Gnu)
