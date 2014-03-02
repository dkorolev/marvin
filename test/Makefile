CPPFLAGS=-DMARVIN_NO_GFLAGS

CPPFLAGS+= -DBOOST_NO_RVALUE_REFERENCES  # Part 1/2 of the patch to have this test build. -- D.K.

CPP=g++ ${CPPFLAGS}
CPP11=${CPP} -std=c++11

all: binary

binary: code.cc gen-cpp/headers.h
	${CPP11} -o $@ $< gen-cpp/*.o -lthrift -lssl

# TODO(dkorolev): Add ../*.thrift when running a proper test, after Thrift + Boost + C++11 issue is resolved. -- D.K.
gen-cpp/headers.h: *.thrift
	for i in *.thrift ; do thrift --gen cpp -r $$i ; done
	(cd gen-cpp; for i in `ls *.cpp | grep -v skeleton` ; do ${CPP} -c $$i ; done)
	ls gen-cpp/*.h | grep -v $@ | xargs cat >$@

clean:
	rm -rf binary gen-cpp/
