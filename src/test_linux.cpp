#include <stdio.h>
#include <dlfcn.h>

int main()
{
	void *libgl = dlopen("libGL.so", RTLD_LAZY | RTLD_GLOBAL);
	void *test = dlsym(libgl, "glXQueryVersion");
		
	dlclose(libgl);
	return 0;
}
