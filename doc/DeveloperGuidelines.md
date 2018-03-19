# Developer Guidelines

### Unit Tests

### Commit Messages

# Python Specific Guidelines

### Python Code Style

### Function & Variable Names


# C++ Specific Guidelines

### Smart Pointers vs Raw Pointers

### Code Style

### **get** & **set** Functions

### Include Order

- In a Foo.cpp file, the equivalent Foo.h header is the first include. Rationale : This proves that Foo.h is a standalone header, not relying on other stuff being included first.
- Headers are then included with the highest level projects first, down to system headers last. Rationale :
    - Some high level headers need to work around problems with lower-level headers, and to do so they need to come first. For instance, the problems described in https://github.com/ImageEngine/cortex/commit/8e2d356f570a28a87f7628d14fbc4bc7f4f9ed48#diff-472bb092433c363608dd9b24fdb806ffR48.
    - Foo.h is the highest level header, and is first anyway.
- Includes from within the same project (i.e. all IECore headers) are ordered alphabetically within their section. Rationale :
    - Makes it harder to accidentally include the same header twice.
- If it must be included, boost/python.hpp comes first. It has always needed to be first so it can work around weird problems created by the python headers - it's a good example of the "high level header needs to fix problems in low level header" thing.
- Finally, we make the odd [inexplicable change](8a8add6) required by Clang. Rationale : I am a mortal, and Clang is a god.

**There's a new script in the contrib directory that can be used to automate this ordering, or check for future violations.**
