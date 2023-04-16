#ifndef VG_IMAGE_LOADER_SVG_H
#define VG_IMAGE_LOADER_SVG_H

#include "core/io/resource_importer.h"
#include "core/io/resource_saver.h"
#include "core/os/file_access.h"
#include "common/gd_core.h"
#include "editor/editor_file_system.h"
#include "editor/editor_node.h"
#include "editor/import/resource_importer_scene.h"
#include "scene/3d/mesh_instance.h"
#include "scene/resources/mesh_data_tool.h"
#include "scene/resources/surface_tool.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/texture.h"
#include "vector_graphics_adaptive_renderer.h"
#include "vector_graphics_path.h"

/// ResourceImporterSVGNode2D

class ResourceImporterSVGNode2D : public ResourceImporter {
	GDCLASS(ResourceImporterSVGNode2D, ResourceImporter);

public:
	virtual String get_importer_name() const G_OVERRIDE { return "svgnode2d"; }
	virtual String get_visible_name() const G_OVERRIDE { return "SVGNode2D"; }
	virtual void get_recognized_extensions(List<String> *p_extensions) const G_OVERRIDE {
		p_extensions->push_back("vg-svg");
		p_extensions->push_back("vg-svgz");
		p_extensions->push_back("vg.svg");
		p_extensions->push_back("vg.svgz");
	}
	virtual String get_save_extension() const G_OVERRIDE { return "scn"; }
	virtual String get_resource_type() const G_OVERRIDE { return "PackedScene"; }

	virtual int get_preset_count() const G_OVERRIDE { return 0; }
	virtual String get_preset_name(int p_idx) const G_OVERRIDE { return String(); }

	virtual void get_import_options(List<ImportOption> *r_options, int p_preset = 0) const G_OVERRIDE { }
	virtual bool get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const G_OVERRIDE { return true; }
	virtual Error import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files = nullptr, Variant *r_metadata = nullptr) G_OVERRIDE;

	static Point2 compute_center(const tove::PathRef &p_path) {
		const float *bounds = p_path->getBounds();
		return Point2((bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2);
	}

	ResourceImporterSVGNode2D() { }
};

Error ResourceImporterSVGNode2D::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	String units = "px";
	const float dpi = 100;

	Vector<uint8_t> buf = FileAccess::get_file_as_array(p_source_file);

	if (!buf.size()) {
		return FAILED;
	}

	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());

	tove::GraphicsRef tove_graphics = tove::Graphics::createFromSVG(str.utf8().ptr(), units.utf8().ptr(), dpi);
	{
		const float *bounds = tove_graphics->getBounds();
		float s = 256.0 / MAX(bounds[2] - bounds[0], bounds[3] - bounds[1]);
		if (s > 1) {
			tove::nsvg::Transform transform(s, 0, 0, 0, s, 0);
			transform.setWantsScaleLineWidth(true);
			tove_graphics->set(tove_graphics, transform);
		}
	}
	int32_t n = tove_graphics->getNumPaths();
	print_verbose(vformat("[SVG] Processing %d paths ...", n));
	EditorProgress progress("import", TTR("Importing Vector Graphics"), n + 2);
	Ref<VGMeshRenderer> renderer = newref(VGMeshRenderer);
	Node2D *root = memnew(Node2D);
	VGPath *root_path = memnew(VGPath(tove::tove_make_shared<tove::Path>()));
	root->add_child(root_path);
	root_path->set_owner(root);
	root_path->set_renderer(renderer);
	for (int i = 0; i < n; i++) {
		progress.step(TTR("Importing Paths..."), i);
		tove::PathRef tove_path = tove_graphics->getPath(i);
		Point2 center = compute_center(tove_path);
		tove_path->set(tove_path, tove::nsvg::Transform(1, 0, -center.x, 0, 1, -center.y));
		VGPath *path = memnew(VGPath(tove_path));
		path->set_position(center);
		std::string name = tove_path->getName();
		if (name.empty()) {
			name = "Path";
		}

		root_path->add_child(path);
		path->set_owner(root);

		MeshInstance2D *mesh_inst = memnew(MeshInstance2D);
		Ref<ArrayMesh> mesh;
		mesh.instance();
		Ref<Texture> texture;
		Ref<Material> renderer_material;
		const Rect2 area = renderer->render_mesh(mesh, renderer_material, texture, path, true, false);
		if (area.is_equal_approx(Rect2())) {
			continue;
		}
		mesh_inst->set_mesh(mesh);
		mesh_inst->set_material(renderer_material);
		mesh_inst->set_texture(texture);
		Transform2D path_xform = path->get_transform();
		mesh_inst->set_transform(path_xform);
		mesh_inst->set_name(String(name.c_str()));
		mesh_inst->set_z_index(i);
		root->add_child(mesh_inst);
		mesh_inst->set_owner(root);
	}
	progress.step(TTR("Saving..."), n);
	Ref<PackedScene> vg_scene = newref(PackedScene);
	vg_scene->pack(root);
	String save_path = p_save_path + ".scn";
	r_gen_files->push_back(save_path);
	return ResourceSaver::save(save_path, vg_scene);
}

/// ResourceImporterSVGSpatial

class ResourceImporterSVGSpatial : public ResourceImporter {
	GDCLASS(ResourceImporterSVGSpatial, ResourceImporter);

public:
	virtual String get_importer_name() const { return "svgspatial"; }
	virtual String get_visible_name() const  { return "SVGSpatial"; }
	virtual void get_recognized_extensions(List<String> *p_extensions) const {
		p_extensions->push_back("vg-svg");
		p_extensions->push_back("vg-svgz");
		p_extensions->push_back("vg.svg");
		p_extensions->push_back("vg.svgz");
	}
	virtual String get_save_extension() const G_OVERRIDE { return "scn"; }
	virtual String get_resource_type() const G_OVERRIDE { return "PackedScene"; }

	virtual int get_preset_count() const G_OVERRIDE { return 0; }
	virtual String get_preset_name(int p_idx) const G_OVERRIDE { return String(); }

	virtual void get_import_options(List<ImportOption> *r_options, int p_preset = 0) const G_OVERRIDE { }
	virtual bool get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const G_OVERRIDE { return true; }
	virtual Error import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files = nullptr, Variant *r_metadata = nullptr) G_OVERRIDE;

	static Point2 compute_center(const tove::PathRef &p_path) {
		const float *bounds = p_path->getBounds();
		return Point2((bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2);
	}

	ResourceImporterSVGSpatial() { }
};

Error ResourceImporterSVGSpatial::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	String units = "px";
	const float dpi = 96;

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
	Spatial *root = memnew(Spatial);
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
			const Rect2 area = renderer->render_mesh(mesh, renderer_material, texture, path, true, true);
			if (area.is_equal_approx(Rect2())) {
				continue;
			}
			Transform xform;
			const real_t gap = i * CMP_POINT_IN_PLANE_EPSILON * 16.0;
			xform.origin = Vector3(center.x * 0.001, center.y * -0.001, gap);
			st->append_from(mesh, 0, xform);
		}
		progress.step(TTR("Finalizing..."), n);
		combined_mesh = st->commit();
		MeshInstance *mesh_inst = memnew(MeshInstance);
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
			const Rect2 area = renderer->render_mesh(mesh, renderer_material, texture, path, true, true);
			if (area.is_equal_approx(Rect2())) {
				continue;
			}
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
	}
	progress.step(TTR("Saving..."), n + 1);
	Ref<PackedScene> vg_scene = newref(PackedScene);
	vg_scene->pack(root);
	String save_path = p_save_path + ".scn";
	r_gen_files->push_back(save_path);
	return ResourceSaver::save(save_path, vg_scene);
}

/// ResourceImporterSVGVGPath

class ResourceImporterSVGVGPath : public ResourceImporter {
	GDCLASS(ResourceImporterSVGVGPath, ResourceImporter);

public:
	virtual String get_importer_name() const G_OVERRIDE { return "svgvgpath"; }
	virtual String get_visible_name() const G_OVERRIDE { return "SVGVGPath"; }
	virtual void get_recognized_extensions(List<String> *p_extensions) const G_OVERRIDE {
		p_extensions->push_back("vg-svg");
		p_extensions->push_back("vg-svgz");
		p_extensions->push_back("vg.svg");
		p_extensions->push_back("vg.svgz");
	}
	virtual String get_save_extension() const G_OVERRIDE { return "scn"; }
	virtual String get_resource_type() const G_OVERRIDE { return "PackedScene"; }

	virtual int get_preset_count() const G_OVERRIDE { return 0; }
	virtual String get_preset_name(int p_idx) const G_OVERRIDE { return String(); }

	virtual void get_import_options(List<ImportOption> *r_options, int p_preset = 0) const G_OVERRIDE { }
	virtual bool get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const G_OVERRIDE { return true; }
	virtual Error import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files = nullptr, Variant *r_metadata = nullptr) G_OVERRIDE;

	static Point2 compute_center(const tove::PathRef &p_path) {
		const float *bounds = p_path->getBounds();
		return Point2((bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2);
	}

	ResourceImporterSVGVGPath() { }
};

Error ResourceImporterSVGVGPath::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	String units = "px";
	const float dpi = 100;

	Vector<uint8_t> buf = FileAccess::get_file_as_array(p_source_file);

	if (!buf.size()) {
		return FAILED;
	}

	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());

	tove::GraphicsRef tove_graphics = tove::Graphics::createFromSVG(
			str.utf8().ptr(), units.utf8().ptr(), dpi);
	{
		const float *bounds = tove_graphics->getBounds();
		float s = 256.0 / MAX(bounds[2] - bounds[0], bounds[3] - bounds[1]);
		if (s > 1) {
			tove::nsvg::Transform transform(s, 0, 0, 0, s, 0);
			transform.setWantsScaleLineWidth(true);
			tove_graphics->set(tove_graphics, transform);
		}
	}
	int32_t n = tove_graphics->getNumPaths();
	print_verbose(vformat("[SVG] Processing %d paths ...", n));
	EditorProgress progress("import", TTR("Importing Vector Graphics"), n + 2);
	Ref<VGMeshRenderer> renderer = newref(VGMeshRenderer);
	Node2D *root = memnew(Node2D);
	VGPath *root_path = memnew(VGPath(tove::tove_make_shared<tove::Path>()));
	root->add_child(root_path);
	root_path->set_owner(root);
	root_path->set_renderer(renderer);
	for (int i = 0; i < n; i++) {
		progress.step(TTR("Importing Paths..."), i);
		tove::PathRef tove_path = tove_graphics->getPath(i);
		Point2 center = compute_center(tove_path);
		tove_path->set(tove_path, tove::nsvg::Transform(1, 0, -center.x, 0, 1, -center.y));
		VGPath *path = memnew(VGPath(tove_path));
		path->set_position(center);

		std::string name = tove_path->getName();
		if (name.empty()) {
			name = "Path";
		}
		path->set_name(String(name.c_str()));
		root_path->add_child(path);
		path->set_owner(root);
	}
	progress.step(TTR("Saving..."), n);
	Ref<PackedScene> vg_scene = newref(PackedScene);
	vg_scene->pack(root);
	String save_path = p_save_path + ".scn";
	r_gen_files->push_back(save_path);
	return ResourceSaver::save(save_path, vg_scene);
}

/// EditorSceneImporterSVG

class EditorSceneImporterSVG : public EditorSceneImporter {
	GDCLASS(EditorSceneImporterSVG, EditorSceneImporter);

	static Point2 compute_center(const tove::PathRef &p_path) {
		const float *bounds = p_path->getBounds();
		return Point2((bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2);
	}

public:
	virtual uint32_t get_import_flags() const G_OVERRIDE { return IMPORT_SCENE | IMPORT_ANIMATION; }
	virtual void get_extensions(List<String> *r_extensions) const G_OVERRIDE {
		r_extensions->push_back("vg-svg");
		r_extensions->push_back("vg-svgz");
		r_extensions->push_back("vg.svg");
		r_extensions->push_back("vg.svgz");
	}
	virtual Node *import_scene(const String &p_path, uint32_t p_flags, int p_bake_fps, uint32_t p_compress_flags, List<String> *r_missing_deps, Error *r_err = nullptr) G_OVERRIDE;

	EditorSceneImporterSVG() { }
};


Node *EditorSceneImporterSVG::import_scene(const String &p_path, uint32_t p_flags, int p_bake_fps, uint32_t p_compress_flags, List<String> *r_missing_deps, Error *r_err) {
	const String units = "px";
	const float dpi = 96;

	Vector<uint8_t> buf = FileAccess::get_file_as_array(p_path);
	if (!buf.size()) {
		return nullptr;
	}
	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());
	tove::GraphicsRef tove_graphics = tove::Graphics::createFromSVG(str.utf8().ptr(), units.utf8().ptr(), dpi);
	const float *tove_bounds = tove_graphics->getBounds();
	const float s = 256.0 / MAX(tove_bounds[2] - tove_bounds[0], tove_bounds[3] - tove_bounds[1]);
	if (s > 1) {
		tove::nsvg::Transform transform(s, 0, 0, 0, s, 0);
		transform.setWantsScaleLineWidth(true);
		tove_graphics->set(tove_graphics, transform);
	}
	int32_t n = tove_graphics->getNumPaths();
	Ref<VGMeshRenderer> renderer = newref(VGMeshRenderer);
	renderer->set_quality(0.4);
	VGPath *root_path = memnew(VGPath(tove::tove_make_shared<tove::Path>()));
	root_path->set_renderer(renderer);
	Node *root = memnew(Node);
	Ref<SurfaceTool> st = newref(SurfaceTool);
	for (int mesh_i = 0; mesh_i < n; mesh_i++) {
		tove::PathRef tove_path = tove_graphics->getPath(mesh_i);
		Point2 center = compute_center(tove_path);
		tove_path->set(tove_path, tove::nsvg::Transform(1, 0, -center.x, 0, 1, -center.y));
		VGPath *path = memnew(VGPath(tove_path));
		path->set_position(center);
		root_path->add_child(path, true);
		path->set_owner(root);
		Ref<ArrayMesh> mesh = newref(ArrayMesh);
		Ref<Texture> texture;
		Ref<Material> renderer_material;
		const Rect2 area = renderer->render_mesh(mesh, renderer_material, texture, path, true, true);
		if (area.is_equal_approx(Rect2())) {
			continue;
		}
		if (renderer_material.is_valid()) {
			mesh->surface_set_material(0, renderer_material);
		}
		Transform xform;
		real_t gap = mesh_i * CMP_POINT_IN_PLANE_EPSILON * 16;
		xform.origin = Vector3(center.x * 0.001, center.y * -0.001, gap);
		if (mesh.is_null()) {
			continue;
		}
		st->add_smooth_group(true);
		st->append_from(mesh, 0, xform);
	}
	String root_name = root_path->get_name();
	memdelete(root_path);
	Ref<ArrayMesh> combined_mesh = newref(ArrayMesh);
	Ref<SpatialMaterial> standard_material = newref(SpatialMaterial);
	standard_material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	standard_material->set_depth_draw_mode(SpatialMaterial::DEPTH_DRAW_ALWAYS);
	standard_material->set_flag(SpatialMaterial::FLAG_DISABLE_DEPTH_TEST, true);
	standard_material->set_cull_mode(SpatialMaterial::CULL_DISABLED);
	combined_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
	combined_mesh->surface_set_material(0, standard_material);
	if (combined_mesh.is_null()) {
		return nullptr;
	}
	Vector3 translate = combined_mesh->get_aabb().get_size();
	translate.x = -translate.x / 2;
	translate.y = translate.y / 2;
	Transform xform;
	xform.origin = translate;

	MeshInstance *mesh_instance = memnew(MeshInstance);
	mesh_instance->set_mesh(combined_mesh);
	root->add_child(mesh_instance, true);
	mesh_instance->set_owner(root);
	mesh_instance->set_transform(xform);
	mesh_instance->set_name(root_name);
	return root;
}

#endif // VG_IMAGE_LOADER_SVG_H
