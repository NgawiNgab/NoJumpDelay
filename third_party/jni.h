#pragma once
// Minimal jni.h subset for LeviLauncher native mods that never touch JNI
// (ApollonKillAura does NOT export JNI_OnLoad and holds JavaVM pointers opaquely).
#ifdef __cplusplus
extern "C" {
#endif

#define JNI_VERSION_1_1 0x00010001
#define JNI_VERSION_1_2 0x00010002
#define JNI_VERSION_1_4 0x00010004
#define JNI_VERSION_1_6 0x00010006
#define JNI_OK 0
#define JNI_ERR (-1)

struct JNIInvokeInterface;
struct JavaVM {
  const struct JNIInvokeInterface *functions;
};

struct JNINativeInterface;
struct JNIEnv {
  const struct JNINativeInterface *functions;
};

#ifdef __cplusplus
}
#endif
