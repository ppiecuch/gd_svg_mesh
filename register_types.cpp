/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"
#include "image_loader_svg.h"
#include "vector_graphics_path.h"
#include "vector_graphics_paint.h"
#include "vector_graphics_color.h"
#include "vector_graphics_gradient.h"
#include "vector_graphics_linear_gradient.h"
#include "vector_graphics_radial_gradient.h"
#include "vector_graphics_renderer.h"
#include "vector_graphics_texture_renderer.h"
#include "vector_graphics_adaptive_renderer.h"

#ifdef TOOLS_ENABLED
#include "vector_graphics_editor_plugin.h"

static void editor_init_callback() {
	EditorNode *editor = EditorNode::get_singleton();
	editor->add_editor_plugin(memnew(VGEditorPlugin(editor)));
}
#endif // TOOLS_ENABLED

void register_gd_svg_mesh_types() {
	ClassDB::register_class<VGPath>();
	ClassDB::register_virtual_class<VGPaint>();
	ClassDB::register_class<VGColor>();
	ClassDB::register_class<VGGradient>();
	ClassDB::register_class<VGLinearGradient>();
	ClassDB::register_class<VGRadialGradient>();

	ClassDB::register_virtual_class<VGRenderer>();
	ClassDB::register_class<VGSpriteRenderer>();
	ClassDB::register_class<VGMeshRenderer>();

	Ref<ResourceImporterSVGSpatial> svg_spatial_loader;
	svg_spatial_loader.instance();
	ResourceFormatImporter::get_singleton()->add_importer(svg_spatial_loader);

	Ref<ResourceImporterSVGNode2D> svg_node_2d_loader;
	svg_node_2d_loader.instance();
	ResourceFormatImporter::get_singleton()->add_importer(svg_node_2d_loader);

	Ref<ResourceImporterSVGVGPath> svg_vg_path_loader;
	svg_vg_path_loader.instance();
	ResourceFormatImporter::get_singleton()->add_importer(svg_vg_path_loader);

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(editor_init_callback);
#endif
}

void unregister_gd_svg_mesh_types() {
}
