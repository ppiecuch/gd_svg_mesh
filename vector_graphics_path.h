/*************************************************************************/
/*  vg_path.h 			                                                 */
/*************************************************************************/

#ifndef VG_PATH_H
#define VG_PATH_H

#include "utils.h"
#include "scene/2d/mesh_instance_2d.h"
#include "vector_graphics_paint.h"
#include "vector_graphics_renderer.h"

class VGPath : public Node2D {
	GDCLASS(VGPath, Node2D);

	Transform2D vg_transform;
	tove::PathRef tove_path;
	Ref<ArrayMesh> mesh;
	Ref<Texture> texture;

	mutable tove::GraphicsRef subtree_graphics;
	bool dirty;

	Ref<VGPaint> fill_color;
	Ref<VGPaint> line_color;
	Ref<VGRenderer> renderer;

	static void set_inherited_dirty(Node *p_node);
	static void compose_graphics(const tove::GraphicsRef &p_tove_graphics,
	const Transform2D &p_transform, const Node *p_node);
	static void _transform_changed(Node *p_node);

	bool inherits_renderer() const;

	tove::GraphicsRef create_tove_graphics() const;
	void add_tove_path(const tove::GraphicsRef &p_tove_graphics) const;
	void update_mesh_representation();

	void update_tove_fill_color();
	void update_tove_line_color();
	void create_fill_color();
	void create_line_color();

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	void _renderer_changed();
	void _bubble_change();

	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual Rect2 _edit_get_rect() const;
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;
	virtual void _edit_set_position(const Point2 &p_position);
	virtual void _edit_set_scale(const Size2 &p_scale);

	virtual void _changed_callback(Object *p_changed, const char *p_prop);

	VGPath *get_root_path();
	Ref<VGRenderer> get_inherited_renderer() const;

 	Ref<VGRenderer> get_renderer();
	void set_renderer(const Ref<VGRenderer> &p_renderer);

	Ref<VGPaint> get_fill_color() const;
	void set_fill_color(const Ref<VGPaint> &p_paint);

	Ref<VGPaint> get_line_color() const;
	void set_line_color(const Ref<VGPaint> &p_paint);

	float get_line_width() const;
	void set_line_width(const float p_line_width);

	void set_points(int p_subpath, Array p_points);
	void insert_curve(int p_subpath, float p_t);
	void remove_curve(int p_subpath, int p_curve);
	bool is_inside(const Point2 &p_point) const;
	VGPath *find_clicked_child(const Point2 &p_point);

	bool is_empty() const;
	int get_num_subpaths() const;
	tove::SubpathRef get_subpath(int p_subpath) const;
	tove::PathRef get_tove_path() const;
    tove::GraphicsRef get_subtree_graphics() const;

	void set_dirty(bool p_children = false);
	void set_tove_path(tove::PathRef p_path);
	void recenter();

	Node2D *create_mesh_node();

	static VGPath *create_from_svg(Ref<Resource> p_resource);
	void import_svg(const String &p_path);

	VGPath();
	VGPath(tove::PathRef p_path);
	virtual ~VGPath();
};

#endif // VG_PATH_H
