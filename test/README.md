Issue:

I am observing the following three pieces not play well together:
* Thrift 0.9.1
* Boost 104601
* C++11 (g++ 4.7.3 on Ubuntu 12.04)

Steps to reproduce:

(git clone git@github.com:dkorolev/felicity.git; git clone git@github.com:dkorolev/marvin.git; cd marvin/test; make)

The code should build, while it does not.

Error:

boost::shared_ptr<T> can't be constructed from a const reference to another boost::shared_ptr<T>.

Error message:

Gist:

...
In file included from /usr/local/include/thrift/protocol/TBinaryProtocol.h:23:0,
                 from ../main.h:29,
                 from test.cc:1:
/usr/local/include/thrift/protocol/TProtocol.h: In member function ‘boost::shared_ptr<apache::thrift::transport::TTransport> apache::thrift::protocol::TProtocol::getTransport()’:
/usr/local/include/thrift/protocol/TProtocol.h:651:12: error: use of deleted function ‘boost::shared_ptr<apache::thrift::transport::TTransport>::shared_ptr(const boost::shared_ptr<apache::thrift::transport::TTransport>&)’
In file included from /usr/include/boost/shared_ptr.hpp:17:0,
                 from /usr/local/include/thrift/transport/TTransport.h:24,
                 from /usr/local/include/thrift/protocol/TProtocol.h:23,
                 from /usr/local/include/thrift/protocol/TBinaryProtocol.h:23,
...

See FULL_ERROR_LOG.txt for more details.

Workaround:

Before looking into Thrift code, the minimum workaround I found implies a code change in Boost and an extra C++ preprocessor flag.

1) Comment out BOOST_HAS_RVALUE_REFS in boost/config/compiler/gcc.hpp

$ diff /usr/include/boost/config/compiler/gcc.hpp ~/original_boost_config_compiler_gcc.hpp
160,161c160,161
< #  warning "BOOST_HAS_RVALUE_REFS is commented out by Dima for Thrift."
< // #  define BOOST_HAS_RVALUE_REFS
---
> #  define BOOST_HAS_RVALUE_REFS

2) Explicitly #define BOOST_NO_RVALUE_REFERENCES.

CPPFLAGS+= -DBOOST_NO_RVALUE_REFERENCES

Enviroment:

$ cat /etc/lsb-release 
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=12.04
DISTRIB_CODENAME=precise

$ uname -a
Linux dima-i9 3.8.0-36-generic #52~precise1-Ubuntu SMP Mon Feb 3 21:54:46 UTC 2014 x86_64 x86_64 x86_64 GNU/Linux

$ g++ --version
g++ (Ubuntu/Linaro 4.7.3-2ubuntu1~12.04) 4.7.3
Copyright (C) 2012 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

$ grep -R "#define BOOST_VERSION" /usr/include/boost/
/usr/include/boost/version.hpp:#define BOOST_VERSION_HPP
/usr/include/boost/version.hpp:#define BOOST_VERSION 104601

$ grep -R VERSION  /usr/local/include/thrift/config.h
#define PACKAGE_VERSION "0.9.1"
#define VERSION "0.9.1"
