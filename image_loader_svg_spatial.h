#ifndef VG_IMAGE_LOADER_SVG_SPATIAL_H
#define VG_IMAGE_LOADER_SVG_SPATIAL_H

#include "core/io/resource_importer.h"
#include "core/io/resource_saver.h"
#include "core/os/file_access.h"
#include "editor/editor_node.h"
#include "editor/editor_file_system.h"
#include "scene/3d/mesh_instance.h"
#include "scene/resources/mesh_data_tool.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/surface_tool.h"
#include "scene/resources/texture.h"
#include "common/gd_core.h"
#include "vector_graphics_adaptive_renderer.h"
#include "vector_graphics_path.h"

class ResourceImporterSVGSpatial : public ResourceImporter {
	GDCLASS(ResourceImporterSVGSpatial, ResourceImporter);

public:
	virtual String get_importer_name() const;
	virtual String get_visible_name() const;
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual String get_save_extension() const;
	virtual String get_resource_type() const;

	virtual int get_preset_count() const;
	virtual String get_preset_name(int p_idx) const;

	virtual void get_import_options(List<ImportOption> *r_options, int p_preset = 0) const;
	virtual bool get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const;
	virtual Error import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files = NULL, Variant *r_metadata = NULL);

	static Point2 compute_center(const tove::PathRef &p_path) {
		const float *bounds = p_path->getBounds();
		return Point2((bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2);
	}

	ResourceImporterSVGSpatial();
	~ResourceImporterSVGSpatial();
};

String ResourceImporterSVGSpatial::get_importer_name() const {
	return "svgspatial";
}

String ResourceImporterSVGSpatial::get_visible_name() const {
	return "SVGSpatial";
}

void ResourceImporterSVGSpatial::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("svg");
	p_extensions->push_back("svgz");
}

String ResourceImporterSVGSpatial::get_save_extension() const {
	return "scn";
}

String ResourceImporterSVGSpatial::get_resource_type() const {
	return "PackedScene";
}

bool ResourceImporterSVGSpatial::get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const {
	return true;
}

int ResourceImporterSVGSpatial::get_preset_count() const {
	return 0;
}

String ResourceImporterSVGSpatial::get_preset_name(int p_idx) const {
	return String();
}

void ResourceImporterSVGSpatial::get_import_options(List<ImportOption> *r_options, int p_preset) const { }

Error ResourceImporterSVGSpatial::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	Spatial *root = memnew(Spatial);

	String units = "px";
	const float dpi = 96.0;

	Vector<uint8_t> buf = FileAccess::get_file_as_array(p_source_file);

	if (!buf.size()) {
		return FAILED;
	}

	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());

	tove::GraphicsRef tove_graphics = tove::Graphics::createFromSVG(str.utf8().ptr(), units.utf8().ptr(), dpi);
	{
		const float *bounds = tove_graphics->getBounds();
		const float s = 256.0 / MAX(bounds[2] - bounds[0], bounds[3] - bounds[1]);
		if (s > 1) {
			tove::nsvg::Transform transform(s, 0, 0, 0, s, 0);
			transform.setWantsScaleLineWidth(true);
			tove_graphics->set(tove_graphics, transform);
		}
	}
	const int32_t n = tove_graphics->getNumPaths();
	print_verbose(vformat("[SVG] Processing %d paths ...", n));
	EditorProgress progress("import", TTR("Importing Vector Graphics"), n + 2);
	Ref<VGMeshRenderer> renderer = newref(VGMeshRenderer);
	renderer->set_quality(0.4);
	VGPath *root_path = memnew(VGPath(tove::tove_make_shared<tove::Path>()));
	root->add_child(root_path);
	root_path->set_owner(root);
	root_path->set_renderer(renderer);
	Ref<SurfaceTool> st = newref(SurfaceTool);
	bool is_merged = true;
	if (is_merged) {
		Ref<ArrayMesh> combined_mesh;
		combined_mesh.instance();
		Ref<SurfaceTool> st = newref(SurfaceTool);
		for (int i = 0; i < n; i++) {
			progress.step(TTR("Importing and Merge Paths..."), i);
			tove::PathRef tove_path = tove_graphics->getPath(i);
			Point2 center = compute_center(tove_path);
			tove_path->set(tove_path, tove::nsvg::Transform(1, 0, -center.x, 0, 1, -center.y));
			VGPath *path = memnew(VGPath(tove_path));
			path->set_position(center);
			root_path->add_child(path);
			path->set_owner(root);
			Ref<ArrayMesh> mesh = newref(ArrayMesh);
			Ref<Texture> texture;
			Ref<Material> renderer_material;
			renderer->render_mesh(mesh, renderer_material, texture, path, true, true);
			Transform xform;
			const real_t gap = i * CMP_POINT_IN_PLANE_EPSILON * 16.0;
			xform.origin = Vector3(center.x * 0.001, center.y * -0.001, gap);
			st->append_from(mesh, 0, xform);
		}
		progress.step(TTR("Finalizing..."), n);
		combined_mesh = st->commit();
		MeshInstance *mesh_inst = memnew(MeshInstance);
		memdelete(root_path);
		Ref<SpatialMaterial> mat = newref(SpatialMaterial);
		mat->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		mat->set_cull_mode(SpatialMaterial::CULL_DISABLED);
		combined_mesh->surface_set_material(0, mat);
		mesh_inst->set_mesh(combined_mesh);
		mesh_inst->set_name(String("Path"));
		Vector3 translate = -combined_mesh->get_aabb().get_size() / 2;
		translate.y = -translate.y;
		mesh_inst->translate_object_local(translate);
		root->add_child(mesh_inst);
		mesh_inst->set_owner(root);
	} else {
		Spatial *spatial = memnew(Spatial);
		spatial->set_name(root_path->get_name());
		root->add_child(spatial);
		spatial->set_owner(root);
		AABB bounds;
		for (int mesh_i = 0; mesh_i < n; mesh_i++) {
			progress.step(TTR("Importing Paths..."), mesh_i);
			tove::PathRef tove_path = tove_graphics->getPath(mesh_i);
			Point2 center = compute_center(tove_path);
			tove_path->set(tove_path, tove::nsvg::Transform(1, 0, -center.x, 0, 1, -center.y));
			VGPath *path = memnew(VGPath(tove_path));
			path->set_position(center);
			root_path->add_child(path);
			path->set_owner(root);
			Ref<ArrayMesh> mesh = newref(ArrayMesh);
			Ref<Texture> texture;
			Ref<Material> renderer_material;
			renderer->render_mesh(mesh, renderer_material, texture, path, true, true);
			Transform xform;
			const real_t gap = mesh_i * CMP_POINT_IN_PLANE_EPSILON * 16.0;
			MeshInstance *mesh_inst = memnew(MeshInstance);
			mesh_inst->translate(Vector3(center.x * 0.001, -center.y * 0.001, gap));
			if (renderer_material.is_null()) {
				Ref<SpatialMaterial> mat = newref(SpatialMaterial);
				mat->set_texture(SpatialMaterial::TEXTURE_ALBEDO, texture);
				mat->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
				mat->set_cull_mode(SpatialMaterial::CULL_DISABLED);
				mesh->surface_set_material(0, mat);
			} else {
				mesh->surface_set_material(0, renderer_material);
			}
			mesh_inst->set_mesh(mesh);
			bounds = bounds.merge(mesh_inst->get_aabb());
			String name = tove_path->getName();
			if (!name.empty()) {
				mesh_inst->set_name(name);
			}
			spatial->add_child(mesh_inst);
			mesh_inst->set_owner(root);
		}
		progress.step(TTR("Finalizing..."), n);
		Vector3 translate = bounds.get_size();
		translate.x = -translate.x;
		translate.x += translate.x / 2.0;
		translate.y += translate.y;
		spatial->translate(translate);
		memdelete(root_path);
	}
	progress.step(TTR("Saving..."), n + 1);
	Ref<PackedScene> vg_scene = newref(PackedScene);
	vg_scene->pack(root);
	String save_path = p_save_path + ".scn";
	r_gen_files->push_back(save_path);
	return ResourceSaver::save(save_path, vg_scene);
}

ResourceImporterSVGSpatial::ResourceImporterSVGSpatial() { }

ResourceImporterSVGSpatial::~ResourceImporterSVGSpatial() { }

#endif // VG_IMAGE_LOADER_SVG_SPATIAL_H
