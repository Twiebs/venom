_(PFNGLCLEARPROC, glClear)
_(PFNGLCLEARBUFFERFVPROC, glClearBufferfv)
_(PFNGLCLEARCOLORPROC, glClearColor)
_(PFNGLGETINTEGERVPROC, glGetIntegerv)

_(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)
_(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)

_(PFNGLGENBUFFERSPROC, glGenBuffers)
_(PFNGLBUFFERDATAPROC, glBufferData)
_(PFNGLBUFFERSUBDATAPROC, glBufferSubData);
_(PFNGLBINDBUFFERPROC, glBindBuffer)

_(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers)
_(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer)
_(PFNGLFRAMEBUFFERTEXTUREPROC, glFramebufferTexture)
_(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D)
_(PFNGLFRAMEBUFFERTEXTURELAYERPROC, glFramebufferTextureLayer)
_(PFNGLDRAWBUFFERPROC, glDrawBuffer)
_(PFNGLDRAWBUFFERSPROC, glDrawBuffers)
_(PFNGLREADBUFFERPROC, glReadBuffer)

_(PFNGLGENTEXTURESPROC, glGenTextures)
_(PFNGLTEXSTORAGE2DPROC, glTexStorage2D)
_(PFNGLTEXSTORAGE3DPROC, glTexStorage3D)
_(PFNGLTEXIMAGE2DPROC, glTexImage2D)
_(PFNGLTEXIMAGE3DPROC, glTexImage3D)
_(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D)
_(PFNGLTEXSUBIMAGE3DPROC, glTexSubImage3D)

_(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap)
_(PFNGLTEXPARAMETERIPROC, glTexParameteri)
_(PFNGLBINDTEXTUREPROC, glBindTexture)

_(PFNGLACTIVETEXTUREPROC, glActiveTexture)

_(PFNGLMAPBUFFERPROC, glMapBuffer)
_(PFNGLUNMAPBUFFERPROC, glUnmapBuffer)

_(PFNGLCREATESHADERPROC, glCreateShader)
_(PFNGLSHADERSOURCEPROC, glShaderSource)
_(PFNGLCREATEPROGRAMPROC, glCreateProgram)
_(PFNGLGETSHADERIVPROC, glGetShaderiv)
_(PFNGLDETACHSHADERPROC, glDetachShader)

_(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)
_(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)
_(PFNGLDELETETEXTURESPROC, glDeleteTextures)
_(PFNGLDELETEPROGRAMPROC, glDeleteProgram)

_(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)
_(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)
_(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor)
_(PFNGLUSEPROGRAMPROC, glUseProgram)

_(PFNGLPROGRAMUNIFORM1IPROC, glProgramUniform1i)
_(PFNGLPROGRAMUNIFORMMATRIX4FVPROC, glProgramUniformMatrix4fv)

_(PFNGLATTACHSHADERPROC, glAttachShader)
_(PFNGLCOMPILESHADERPROC, glCompileShader)
_(PFNGLLINKPROGRAMPROC, glLinkProgram)
_(PFNGLGETPROGRAMIVPROC, glGetProgramiv)

_(PFNGLDELETESHADERPROC, glDeleteShader)

_(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)

_(PFNGLENABLEPROC, glEnable)
_(PFNGLDISABLEPROC, glDisable)




_(PFNGLBLENDFUNCPROC, glBlendFunc)
_(PFNGLDEPTHFUNCPROC, glDepthFunc)
_(PFNGLDEPTHMASKPROC, glDepthMask)

//Set Uniforms
_(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)
_(PFNGLUNIFORM1IPROC, glUniform1i)
_(PFNGLUNIFORM1FPROC, glUniform1f)
_(PFNGLUNIFORM1FVPROC, glUniform1fv)
_(PFNGLUNIFORM3FPROC, glUniform3f)
_(PFNGLUNIFORM3FVPROC, glUniform3fv)

_(PFNGLPOLYGONMODEPROC, glPolygonMode)
_(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)
_(PFNGLSTENCILFUNCPROC, glStencilFunc)
_(PFNGLSTENCILOPPROC, glStencilOp)
_(PFNGLSTENCILMASKPROC, glStencilMask)
_(PFNGLBLENDEQUATIONPROC, glBlendEquation)

_(PFNGLVIEWPORTPROC, glViewport)
_(PFNGLCULLFACEPROC, glCullFace)

_(PFNGLDRAWARRAYSPROC, glDrawArrays)
_(PFNGLDRAWELEMENTSPROC, glDrawElements)
_(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced)

#ifndef VENOM_RELEASE
_(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback)
_(PFNGLDEBUGMESSAGECONTROLPROC, glDebugMessageControl)

_(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)
_(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)
#endif
