#ifndef _Included_jniNative
#define _Included_jniNative

#include <../jni.h>
#include <mygl/OGLKview.h>
/// TODO
#define JNIREG_CLASS "com/automatic/kline/Native"

#ifdef __cplusplus
extern "C" {
#endif
	/*
	* Class:     Java_Native
	* Method:    getNative
	* Signature: ()V
	*/
	JNIEXPORT jstring JNICALL Java_Native_Func(JNIEnv *, jclass);


#ifdef __cplusplus
}
#endif

JNIEXPORT jstring JNICALL Java_Native_Func(JNIEnv *env, jclass clazz)
{
	FILE *f;
	f = fopen("hello.txt", "a+, ccs=UTF-8");
	wchar_t *t = (wchar_t*)"嘿嘿xix你好啊！";
	fwrite(t, sizeof(wchar_t), 15, f);
	fclose(f);
	return env->NewStringUTF("hello jni.");
}

// Java & JNI
static JNINativeMethod method_table[] = {
	{ "getNative", "()Ljava/lang/String;", (void*)Java_Native_Func },//绑定
};

// register native method to java
static int registerNativeMethods(JNIEnv* env, const char* className,
	JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;
	clazz = env->FindClass(className);
	if (clazz == NULL) {
		return JNI_FALSE;
	}
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

int register_ndk_load(JNIEnv *env)
{
	return registerNativeMethods(env, JNIREG_CLASS,
		method_table, SZ_ELEM(method_table));
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;
	if (vm->GetEnv((void**)&env, JNI_VERSION_1_8) != JNI_OK) {
		return result;
	}
	register_ndk_load(env);
	return JNI_VERSION_1_8;
}

#endif // _Included_jniNative
