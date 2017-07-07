#include "GLSLCanvas.h"

#include "GLBase.h"
#include "LoadShaders.h"

#include "SystemUtil.h"

#include <iostream>



namespace {
	
	// common GL resources
	CGLContextObj		renderContext = 0;
	NSOpenGLContext		*context = 0;
	std::string			resourcePath;
	
	GLuint frameBuffer = 0;
	GLuint vao = 0;
	GLuint quad = 0;
	GLuint outputFrameTexture = 0;
	
	// program
	GLuint program = 0;
	GLuint attribPositionLocation = 0;
	GLuint uniformResolutionLocation = 0;
	GLuint uniformTimeLocation = 0;
	GLuint uniformMouseLocation = 0;
	
	
	
	
	void InitProgram(std::string fragmentFilePath) {
		
		program = LoadShaders((resourcePath + "vertex-shader.vert").c_str(),
							  fragmentFilePath.c_str());
		
		attribPositionLocation		= glGetAttribLocation(program, "position");
		uniformResolutionLocation	= glGetUniformLocation(program, "u_resolution");
		uniformTimeLocation			= glGetUniformLocation(program, "u_time");
		uniformMouseLocation		= glGetUniformLocation(program, "u_mouse");
	}
	
	void CreateQuad(u_int16 w, u_int16 h) {
		
		// make and bind the VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		
		// make and bind the VBO
		GLfloat positions[] = {
			-1, -1, 0,
			+1, -1, 0,
			-1, +1, 0,
			+1, +1, 0
		};
		
		glGenBuffers(1, &quad);
		glBindBuffer(GL_ARRAY_BUFFER, quad);
		glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
		
		glBindVertexArray(0);
	}
	
	void RenderQuad() {
		glEnableVertexAttribArray(attribPositionLocation);
		glBindBuffer(GL_ARRAY_BUFFER, quad);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(0);
	}
	
	
	void SetPluginContext() {
		if (context) {
			[context makeCurrentContext];
		}
		AESDK_OpenGL::makeCurrentFlush(renderContext);
	}
	
	
	void RenderGL(GLfloat width, GLfloat height, GLfloat time, A_FloatPoint mouse) {
		
		glViewport(0, 0, width, height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(program);
		
		glUniform2f(uniformResolutionLocation, width, height);
		glUniform1f(uniformTimeLocation, time);
		glUniform2f(uniformMouseLocation, mouse.x, mouse.y);

		glBindVertexArray(vao);
		RenderQuad();
		glBindVertexArray(0);
		
		glUseProgram(0);
	}
	
	std::string GetResourcesPath(PF_InData *in_data) {
		//initialize and compile the shader objects
		A_UTF16Char pluginFolderPath[AEFX_MAX_PATH];
		PF_GET_PLATFORM_DATA(PF_PlatData_EXE_FILE_PATH_W, &pluginFolderPath);
		
		NSUInteger length = 0;
		A_UTF16Char* tmp = pluginFolderPath;
		while (*tmp++ != 0) {
			++length;
		}
		NSString* newStr = [[NSString alloc] initWithCharacters:pluginFolderPath length : length];
		std::string path([newStr UTF8String]);
		path += "/Contents/Resources/";
		
		return path;
	}
	
	void InitResources(u_int16 width, u_int16 height) {
		
		if (frameBuffer == 0) {
			// create the color buffer to render to
			glGenFramebuffers(1, &frameBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
			
			// The texture we're going to render to
			glGenTextures(1, &outputFrameTexture);
			
			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, outputFrameTexture);
			
			// Give an empty image to OpenGL ( the last "0" )
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			
			// Poor filtering. Needed !
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		
		if (vao == 0) {
			CreateQuad(width, height);
			
		}
		
	}
};

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
PopDialog (
   PF_InData		*in_data,
   PF_OutData		*out_data,
   PF_ParamDef		*params[],
   PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
	
	std::vector<std::string> fileTypes;
	
	fileTypes.push_back("frag");
	fileTypes.push_back("glsl");
	fileTypes.push_back("fs");
	
	std::string path = AESDK_SystemUtil::openFileDialog(fileTypes);
	
	if (!path.empty()) {
		std::cout << "path:" << path << std::endl;
		InitProgram(path);
		
		FilterSeqData *info = *(FilterSeqData**)out_data->sequence_data;
		strncpy(info->fragPath, path.c_str(), FRAGPATH_MAX_LEN);
		
	} else {
		std::cout << "path: Not Changed" << std::endl;
	}
	
	out_data->out_flags |= PF_OutFlag_SEND_DO_DIALOG;
	
	return err;
}

//---------------------------------------------------------------------------
static PF_Err
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_NON_PARAM_VARY | PF_OutFlag_I_DO_DIALOG;
	
	PF_Err err = PF_Err_NONE;
	
	try {
		
		// always restore back AE's own OGL context
		AESDK_OpenGL::SaveRestoreOGLContext oSavedContext;
		SetPluginContext();
		
		context = AESDK_OpenGL::createNSContext(nullptr, renderContext);
		[context makeCurrentContext];
		
		
		std::cout << std::endl
			<< "OpenGL Version:			" << glGetString(GL_VERSION) << std::endl
			<< "OpenGL Vendor:			" << glGetString(GL_VENDOR) << std::endl
			<< "OpenGL Renderer:		" << glGetString(GL_RENDERER) << std::endl
			<< "OpenGL GLSL Versions:	" << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
		
		resourcePath = GetResourcesPath(in_data);
		
		InitProgram(resourcePath + "fragment-shader.frag");
		
		
	} catch(PF_Err& thrown_err) {
		err = thrown_err;
	} catch (...) {
		err = PF_Err_OUT_OF_MEMORY;
	}
	
	return err;
}

//---------------------------------------------------------------------------
static PF_Err
GlobalSetdown (
			   PF_InData		*in_data,
			   PF_OutData		*out_data,
			   PF_ParamDef		*params[],
			   PF_LayerDef		*output )
{
	PF_Err			err			=	PF_Err_NONE;
	
	try
	{
		// always restore back AE's own OGL context
		AESDK_OpenGL::SaveRestoreOGLContext oSavedContext;
		
		if (outputFrameTexture)
			glDeleteTextures(1, &outputFrameTexture);
		
		if (program)
			glDeleteProgram(program);
		
		if (frameBuffer)
			glDeleteFramebuffers(1, &frameBuffer);
		
		if (vao) {
			glDeleteBuffers(1, &quad);
			glDeleteVertexArrays(1, &vao);
		}
		
		if (in_data->sequence_data) {
			PF_DISPOSE_HANDLE(in_data->sequence_data);
			out_data->sequence_data = NULL;
		}
		
	} catch(PF_Err& thrown_err) {
		err = thrown_err;
	} catch (...) {
		err = PF_Err_OUT_OF_MEMORY;
	}
	
	return err;
}

//---------------------------------------------------------------------------
static PF_Err
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	
	PF_ParamDef		def;
	
	AEFX_CLR_STRUCT(def);
	
	// TODO: set default mouse position to center of layer
	PF_ADD_POINT(STR(StrID_Mouse_Param_Name),
				 (A_long)(in_data->width / 2.0f),
				 (A_long)(in_data->height / 2.0f),
				 RESTRICT_BOUNDS,
				 MOUSE_DISK_ID);
	
	out_data->num_params = FILTER_NUM_PARAMS;

	return err;
}

//---------------------------------------------------------------------------
static PF_Err
SequenceSetup (
			   PF_InData		*in_data,
			   PF_OutData		*out_data,
			   PF_ParamDef		*params[],
			   PF_LayerDef		*output )
{
	FilterSeqData		*info;
	
	if (out_data->sequence_data){
		PF_DISPOSE_HANDLE(out_data->sequence_data);
	}
	out_data->sequence_data = PF_NEW_HANDLE(sizeof(FilterSeqData));
	if (!out_data->sequence_data) {
		return PF_Err_INTERNAL_STRUCT_DAMAGED;
	}
	
	// generate base table
	info = *(FilterSeqData**)out_data->sequence_data;
	strcpy(info->fragPath, "");
	
	
	std::cout << "SsequenceSetup Called" << std::endl;
	
	return PF_Err_NONE;
}

//---------------------------------------------------------------------------
static PF_Err
SequenceSetdown (
	 PF_InData		*in_data,
	 PF_OutData		*out_data,
	 PF_ParamDef	*params[],
	 PF_LayerDef	*output )
{
	
	std::cout << "SequenceSetdown Called" << std::endl;
	
	if (in_data->sequence_data) {
		PF_DISPOSE_HANDLE(in_data->sequence_data);
		out_data->sequence_data = NULL;
	}
	return PF_Err_NONE;
}

//---------------------------------------------------------------------------
static PF_Err
SequenceFlatten(
	PF_InData		*in_data,
	PF_OutData		*out_data)
{
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	std::cout << "SequenceFlatten Called" << std::endl;
	
	// Make a flat copy of whatever is in the unflat seq data handed to us.
	
	if (in_data->sequence_data) {
		FilterSeqData* unflatSeqDataP = reinterpret_cast<FilterSeqData*>(DH(in_data->sequence_data));
		
		if (unflatSeqDataP){
			PF_Handle flatSeqDataH = suites.HandleSuite1()->host_new_handle(sizeof(FilterSeqData));
			
			if (flatSeqDataH){
				FilterSeqData*	flat_seq_dataP = reinterpret_cast<FilterSeqData*>(suites.HandleSuite1()->host_lock_handle(flatSeqDataH));
				
				if (flat_seq_dataP){
					AEFX_CLR_STRUCT(*flat_seq_dataP);

#ifdef AE_OS_WIN
					strncpy_s(flat_seq_dataP->fragPath, unflatSeqDataP->fragPath, FRAGPATH_MAX_LEN);
#else
					strncpy(flat_seq_dataP->fragPath, unflatSeqDataP->fragPath, FRAGPATH_MAX_LEN);
#endif
					
					// In SequenceSetdown we toss out the unflat data
					//delete unflat_seq_dataP->fragPath;
					suites.HandleSuite1()->host_dispose_handle(in_data->sequence_data);
					
					out_data->sequence_data = flatSeqDataH;
					suites.HandleSuite1()->host_unlock_handle(flatSeqDataH);
				}
			} else {
				err = PF_Err_INTERNAL_STRUCT_DAMAGED;
			}
		}
	} else {
		err = PF_Err_INTERNAL_STRUCT_DAMAGED;
	}
	return err;
}

//---------------------------------------------------------------------------
static PF_Err
GetFlattenedSequenceData(
	 PF_InData		*in_data,
	 PF_OutData		*out_data)
{
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	std::cout << "GetFlattenedSequenceData Called" << std::endl;
	
	// Make a flat copy of whatever is in the unflat seq data handed to us.
	
	if (in_data->sequence_data){
		FilterSeqData* unflat_seq_dataP = reinterpret_cast<FilterSeqData*>(DH(in_data->sequence_data));
		
		if (unflat_seq_dataP){
			PF_Handle flat_seq_dataH = suites.HandleSuite1()->host_new_handle(sizeof(FilterSeqData));
			
			if (flat_seq_dataH){
				FilterSeqData*	flat_seq_dataP = reinterpret_cast<FilterSeqData*>(suites.HandleSuite1()->host_lock_handle(flat_seq_dataH));
				
				if (flat_seq_dataP){
					AEFX_CLR_STRUCT(*flat_seq_dataP);
					
#ifdef AE_OS_WIN
					strncpy_s(flat_seq_dataP->fragPath, unflat_seq_dataP->fragPath, FRAGPATH_MAX_LEN);
#else
					strncpy(flat_seq_dataP->fragPath, unflat_seq_dataP->fragPath, FRAGPATH_MAX_LEN);
#endif
					
					// The whole point of this function is that we don't dispose of the unflat data!
					// delete [] unflat_seq_dataP->stringP;
					//suites.HandleSuite1()->host_dispose_handle(in_data->sequence_data);
					
					out_data->sequence_data = flat_seq_dataH;
					suites.HandleSuite1()->host_unlock_handle(flat_seq_dataH);
				}
			} else {
				err = PF_Err_INTERNAL_STRUCT_DAMAGED;
			}
		}
	} else {
		err = PF_Err_INTERNAL_STRUCT_DAMAGED;
	}
	
	return err;
}

//---------------------------------------------------------------------------
static PF_Err
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err			= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);
	
	FilterSeqData *seqData = reinterpret_cast<FilterSeqData*>(DH(in_data->sequence_data));
	
	std::cout << "Render Called: " << seqData->fragPath << std::endl;

	/*	Put interesting code here. */
	try {
		// always restore back AE's own OGL context
		AESDK_OpenGL::SaveRestoreOGLContext oSavedContext;
		SetPluginContext();
		
		u_int16 width = in_data->width, height = in_data->height;
		
		InitResources(width, height);
		
		// MakeReadyToRender
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputFrameTexture, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		
		// RenderGL
		float time = (GLfloat)in_data->current_time / (GLfloat)in_data->time_scale;
		A_FloatPoint	mouse = {
			FIX_2_FLOAT(params[FILTER_MOUSE]->u.td.x_value),
			FIX_2_FLOAT(params[FILTER_MOUSE]->u.td.y_value)
		};
		RenderGL(width, height, time, mouse);
		
		// DownlodTexture
		size_t pixSize = sizeof(PF_Pixel8);
		
		PF_Handle bufferH = NULL;
		bufferH = suites.HandleSuite1()->host_new_handle(width * height * pixSize);
		void *bufferP = suites.HandleSuite1()->host_lock_handle(bufferH);
		
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bufferP);
		
		PF_Pixel8 *pixels = reinterpret_cast<PF_Pixel8*>(bufferP);
		
		for (int y = 0; y < output->height; ++y) {
			PF_Pixel8 *pixelDataStart = NULL;
			PF_GET_PIXEL_DATA8(output, NULL, &pixelDataStart);
			::memcpy(pixelDataStart + (y * output->rowbytes / pixSize),
					 pixels + y * width,
					 width * pixSize);
		}

		suites.HandleSuite1()->host_unlock_handle(bufferH);
		suites.HandleSuite1()->host_dispose_handle(bufferH);
		
		// end of DownloadTexture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		
	} catch (PF_Err& thrown_err) {
		err = thrown_err;
	} catch (...) {
		err = PF_Err_OUT_OF_MEMORY;
	}

	return err;
}


DllExport	
PF_Err 
EntryPointFunc (
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
			
			case PF_Cmd_DO_DIALOG:
				
				err = PopDialog(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_GLOBAL_SETDOWN:
				
				err = GlobalSetdown(in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
			
			case PF_Cmd_SEQUENCE_SETUP:
				
				err = SequenceSetup(in_data,
									out_data,
									params,
									output);
				break;
			
			case PF_Cmd_SEQUENCE_SETDOWN:
				
				err = SequenceSetdown(in_data,
									  out_data,
									  params,
									  output);
				break;
			
			case PF_Cmd_SEQUENCE_FLATTEN:
				
				err = SequenceFlatten(in_data, out_data);
				break;
			
			case PF_Cmd_GET_FLATTENED_SEQUENCE_DATA:
				
				GetFlattenedSequenceData(in_data,out_data);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

