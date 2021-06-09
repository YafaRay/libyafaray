/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef YAFARAY_C_API_H
#define YAFARAY_C_API_H

#include "yafaray_c_api_export.h"
#include "yafaray_conf.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct yafaray_Interface yafaray_Interface_t;
	typedef struct yafaray_Image yafaray_Image_t;

	typedef enum { YAFARAY_INTERFACE_FOR_RENDERING, YAFARAY_INTERFACE_EXPORT_XML } yafaray_Interface_Type_t;
	typedef enum { YAFARAY_BOOL_FALSE = 0, YAFARAY_BOOL_TRUE = 1 } yafaray_bool_t;

	YAFARAY_C_API_EXPORT yafaray_Interface_t *yafaray_createInterface(yafaray_Interface_Type_t interface_type, const char *exported_file_path, const yafaray_LoggerCallback_t logger_callback, void *callback_user_data, yafaray_DisplayConsole_t display_console);
	YAFARAY_C_API_EXPORT void yafaray_destroyInterface(yafaray_Interface_t *interface);

	YAFARAY_C_API_EXPORT void yafaray_createScene(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_startGeometry(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_endGeometry(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT unsigned int yafaray_getNextFreeId(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_endObject(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT int yafaray_addVertex(yafaray_Interface_t *interface, double x, double y, double z);
	YAFARAY_C_API_EXPORT int yafaray_addVertexWithOrco(yafaray_Interface_t *interface, double x, double y, double z, double ox, double oy, double oz);
	YAFARAY_C_API_EXPORT void yafaray_addNormal(yafaray_Interface_t *interface, double nx, double ny, double nz);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_addTriangle(yafaray_Interface_t *interface, int a, int b, int c);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_addTriangleWithUv(yafaray_Interface_t *interface, int a, int b, int c, int uv_a, int uv_b, int uv_c);
	YAFARAY_C_API_EXPORT int yafaray_addUv(yafaray_Interface_t *interface, float u, float v);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_smoothMesh(yafaray_Interface_t *interface, const char *name, double angle);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_addInstance(yafaray_Interface_t *interface, const char *base_object_name, const float obj_to_world[4][4]);
	YAFARAY_C_API_EXPORT void yafaray_paramsSetVector(yafaray_Interface_t *interface, const char *name, double x, double y, double z);
	YAFARAY_C_API_EXPORT void yafaray_paramsSetString(yafaray_Interface_t *interface, const char *name, const char *s);
	YAFARAY_C_API_EXPORT void yafaray_paramsSetBool(yafaray_Interface_t *interface, const char *name, yafaray_bool_t b);
	YAFARAY_C_API_EXPORT void yafaray_paramsSetInt(yafaray_Interface_t *interface, const char *name, int i);
	YAFARAY_C_API_EXPORT void yafaray_paramsSetFloat(yafaray_Interface_t *interface, const char *name, double f);
	YAFARAY_C_API_EXPORT void yafaray_paramsSetColor(yafaray_Interface_t *interface, const char *name, float r, float g, float b, float a);
	YAFARAY_C_API_EXPORT void yafaray_paramsSetMatrix(yafaray_Interface_t *interface, const char *name, const float *m, yafaray_bool_t transpose);
	YAFARAY_C_API_EXPORT void yafaray_paramsClearAll(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT void yafaray_paramsPushList(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT void yafaray_paramsEndList(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT void yafaray_setCurrentMaterial(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createObject(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createLight(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createTexture(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createTextureFromImage(yafaray_Interface_t *interface, const char *name, const yafaray_Image_t *image);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createMaterial(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createCamera(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createBackground(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createIntegrator(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createVolumeRegion(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createRenderView(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_createOutput(yafaray_Interface_t *interface, const char *name, yafaray_bool_t auto_delete, yafaray_OutputPutpixelCallback_t output_putpixel_callback, yafaray_OutputFlushAreaCallback_t output_flush_area_callback, yafaray_OutputFlushCallback_t output_flush_callback, void *callback_user_data);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_removeOutput(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT void yafaray_clearOutputs(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT void yafaray_clearAll(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT void yafaray_render(yafaray_Interface_t *interface, yafaray_ProgressBarCallback_t monitor_callback, void *callback_user_data, yafaray_DisplayConsole_t progress_bar_display_console);
	YAFARAY_C_API_EXPORT void yafaray_defineLayer(yafaray_Interface_t *interface, const char *layer_type_name, const char *exported_image_type_name, const char *exported_image_name, const char *image_type_name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_setupLayersParameters(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT void yafaray_cancel(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_setInteractive(yafaray_Interface_t *interface, yafaray_bool_t interactive);
	YAFARAY_C_API_EXPORT void yafaray_enablePrintDateTime(yafaray_Interface_t *interface, yafaray_bool_t value);
	YAFARAY_C_API_EXPORT void yafaray_setConsoleVerbosityLevel(yafaray_Interface_t *interface, yafaray_LogLevel_t log_level);
	YAFARAY_C_API_EXPORT void yafaray_setLogVerbosityLevel(yafaray_Interface_t *interface, yafaray_LogLevel_t log_level);
	YAFARAY_C_API_EXPORT void yafaray_getVersion(yafaray_Interface_t *interface, char *dest_string, size_t dest_string_size);
	YAFARAY_C_API_EXPORT void yafaray_printDebug(yafaray_Interface_t *interface, const char *msg);
	YAFARAY_C_API_EXPORT void yafaray_printVerbose(yafaray_Interface_t *interface, const char *msg);
	YAFARAY_C_API_EXPORT void yafaray_printInfo(yafaray_Interface_t *interface, const char *msg);
	YAFARAY_C_API_EXPORT void yafaray_printParams(yafaray_Interface_t *interface, const char *msg);
	YAFARAY_C_API_EXPORT void yafaray_printWarning(yafaray_Interface_t *interface, const char *msg);
	YAFARAY_C_API_EXPORT void yafaray_printError(yafaray_Interface_t *interface, const char *msg);
	YAFARAY_C_API_EXPORT void yafaray_cancelRendering(yafaray_Interface_t *interface);
	YAFARAY_C_API_EXPORT void yafaray_setInputColorSpace(const char *color_space_string, float gamma_val);
	YAFARAY_C_API_EXPORT yafaray_Image_t *yafaray_createImage(yafaray_Interface_t *interface, const char *name);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_setImageColor(yafaray_Image_t *image, int x, int y, float red, float green, float blue, float alpha);
	YAFARAY_C_API_EXPORT yafaray_bool_t yafaray_getImageColor(const yafaray_Image_t *image, int x, int y, float *red, float *green, float *blue, float *alpha);
	YAFARAY_C_API_EXPORT void yafaray_setConsoleLogColorsEnabled(yafaray_Interface_t *interface, yafaray_bool_t colors_enabled);

#ifdef __cplusplus
}
#endif

#endif /* YAFARAY_C_API_H */
