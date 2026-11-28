#include "verification.hpp"
#include <cassert>
#include <dlfcn.h>
int main(int argc, char** argv) {
 assert(argc==2);
 assert(aura::verifyLoadedGame().status==aura::VerificationStatus::MissingLibrary);
 void* handle=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL); assert(handle);
 assert(aura::verifyLoadedGame().status==aura::VerificationStatus::Mismatch);
 dlclose(handle);
 assert(aura::verifyLoadedGame().status==aura::VerificationStatus::MissingLibrary);
}
