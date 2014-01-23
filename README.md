# marvin

## Purpose

Marvin is a stub for real-time clients that:

* Consume recent and real-time user behavior logs (think tail -f).
* Keep track of user behavior and user engagement data on the fly (think a fixed set of simple data analysis queries).
* Augment log messages with real-time feature values and perhaps their labels (gather labeled data).
* Generate personalized recommendations (based on models trained on the data gathered).

Marvin is written in C++ and uses Thrift as an external interface.

The original design is to pipe in the output of ```tail -f``` or ```fetch --last_ms_and_follow``` from ```npm install overlog``` as the input and use ```npm install ariadne``` as the frontend.

## Motivation

![marvin](https://raw2.github.com/dkorolev/marvin/master/static/marvin.png)

User engagement. Don't talk to me about user engagement.

## Usage

C++:

```c++
#include <gflags/gflags.h>

#include "../marvin/main.h"

#include "gen-cpp/API.h"

class APIImpl : virtual public API::APIIf {
 public:
  void ariadne_status(API::Status& status) {
    // ... application-specific code ...
  }
  // ... more application-specific code ...
  void ariadne_keepalive(API::Void&, const API::Void&) {
    watchdog_reset();
  }
};

int main(int argc, char** argv) {
  if (!google::ParseCommandLineFlags(&argc, &argv, true)) {
    return -1; 
  }
  marvin_main<APIImpl, API::APIProcessor>();
}
```

Thrift:

```
namespace cpp API

struct Void {
}

struct Status {
  // ... application-specific data ...
}

// ... more application-specific data ...

// The service itself.
service VCR {
  Status ariadne_status(),
  Void ariadne_keepalive(1: Void _)
  // ... application-specific methods ...
}
```

Then run the binary under a thin frontend wrapper using ```npm install ariadne```:

```bash#!/bin/bash
#!/bin/bash
make || exit 1               # Generage thrift protos, npm install, etc.
(cd ../backend ; make) || exit 1  # Rebuild C++ code if necessary.
tail -l /some/log.txt | node wrapper.js --server_command ../backend/binary
```

## Running

Would require boost and thrift installed. For installation instructions, check out the README of this repository: https://github.com/dkorolev/sandbox_node_cpp_server

TODO(dkorolev): Put a README into this repository.
