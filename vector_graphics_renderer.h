/*************************************************************************/
/*  vg_renderer.h                                                        */
/*************************************************************************/

#ifndef VG_RENDERER_H
#define VG_RENDERER_H

#include "core/resource.h"
#include "scene/2d/mesh_instance_2d.h"

class VGPath;

class VGRenderer : public Resource {
    GDCLASS(VGRenderer, Resource);

protected:
    static void clear_mesh(Ref<ArrayMesh> &p_mesh);

public:
    virtual bool prefer_sprite() const {
        return false;
    }

    virtual void render_mesh(Ref<ArrayMesh> &p_mesh, Ref<Material> &r_material, Ref<Texture> &r_texture, VGPath *p_path, bool p_hq, bool p_spatial) {
    }

    virtual Ref<ImageTexture> render_texture(VGPath *p_path, bool p_hq) {
        return Ref<ImageTexture>();
    }

    virtual bool is_dirty_on_transform_change() const = 0;
};

#endif // VG_RENDERER_H
