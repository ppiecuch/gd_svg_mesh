/*************************************************************************/
/*  resource_format_loader_svg.h 	                                     */
/*************************************************************************/

// disabled, as it conflicts with the current SVG bitmap importer currently.
// you can still assign imported SVGs to a VectorGraphics resource though.
#define GDTOVE_SVG_RFL 0

#if GDTOVE_SVG_RFL

#ifndef VG_RESOURCE_FORMAT_LOADER_SVG_H
#define VG_RESOURCE_FORMAT_LOADER_SVG_H

#include "io/resource_loader.h"

class ResourceFormatLoaderSVG : public ResourceFormatLoader {
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_no_subresource_cache = false);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

#endif // VG_RESOURCE_FORMAT_LOADER_SVG_H

#endif // GDTOVE_SVG_RFL
