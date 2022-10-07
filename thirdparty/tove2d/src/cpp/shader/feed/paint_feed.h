/*
* TÖVE - Animated vector graphics for LÖVE.
* https://github.com/poke1024/tove2d
*
* Copyright (c) 2018, Bernhard Liebl
*
* Distributed under the MIT license. See LICENSE file for details.
*
* All rights reserved.
*/

#ifndef __TOVE_SHADER_COLOR
#define __TOVE_SHADER_COLOR 1

#include "core/error_macros.h"

#include <functional>
#include "../gen.h"

BEGIN_TOVE_NAMESPACE

#define SPECIAL_CASE_2 true

class AbstractPaintFeed {
protected:
	TovePaintData &paintData;
	const float scale;
	PaintRef shader;

protected:
	static TovePaintType getStyle(const PaintRef &paint) {
		if (paint.get() == nullptr) {
			return PAINT_NONE;
		}
		const TovePaintType style = paint->getType();
		if (style < PAINT_LINEAR_GRADIENT) {
			return style;
		} else {
			NSVGgradient *g = paint->getNSVGgradient();
			ERR_FAIL_NULL_V(g, PAINT_NONE);

			if (g->nstops < 1) {
				return PAINT_NONE;
			} else {
				return style;
			}
		}
	}

	int determineNumColors(const PaintRef &paint) const {
		const TovePaintType style = (TovePaintType)paintData.style;
		if (style < PAINT_LINEAR_GRADIENT) {
			return style == PAINT_SOLID ? 1 : 0;
		} else {
			NSVGgradient *g = paint->getNSVGgradient();
			ERR_FAIL_NULL_V(g, 0);
			ERR_FAIL_COND_V(g->nstops < 1, 0);

			if (SPECIAL_CASE_2 && g->nstops == 2 &&
				g->stops[0].offset == 0 &&
				g->stops[1].offset == 1) {
				return 2;
			} else {
				return 256;
			}
		}
	}

	bool update(const PaintRef &paint, float opacity) {
		ToveGradientData &gradient = paintData.gradient;
		if (gradient.colorsTexture==nullptr) assert(false);
		ERR_FAIL_NULL_V(gradient.colorsTexture, false);

		const TovePaintType style = (TovePaintType)paintData.style;
		if (style < PAINT_LINEAR_GRADIENT) {
			if (style == PAINT_SOLID) {

				// always fill paintData.rgba, even if we don't
				// have a texture. this is used in gpux, for
				// handing over solid colors (see ColorSend:endInit).
				// (regression test: line in gpux blob).
				paint->getRGBA(paintData.rgba, opacity);

				uint8_t *texture = gradient.colorsTexture;
				if (texture) {
					const uint32_t color = nsvg::makeColor(
						paintData.rgba.r,
						paintData.rgba.g,
						paintData.rgba.b,
						paintData.rgba.a);

					*(uint32_t*)texture = color;
				}
			}

			return true;
		}

		uint8_t *texture = gradient.colorsTexture;
		const int textureRowBytes = gradient.colorsTextureRowBytes;

		NSVGgradient *g = paint->getNSVGgradient();
		ERR_FAIL_NULL_V(g, false);

		if (SPECIAL_CASE_2 && gradient.numColors == 2 &&
			gradient.colorsTextureHeight == gradient.numColors) {
			ERR_FAIL_COND_V(g->nstops != gradient.numColors, false);

			for (int i = 0; i < gradient.numColors; i++) {
				*(uint32_t*)texture = nsvg::applyOpacity(
					g->stops[i].color, opacity);
				texture += textureRowBytes;
			}
		} else {
			NSVGpaint nsvgPaint;
			nsvgPaint.type = paint->getType() == PAINT_LINEAR_GRADIENT ?
				NSVG_PAINT_LINEAR_GRADIENT : NSVG_PAINT_RADIAL_GRADIENT;
			nsvgPaint.gradient = g;

			ERR_FAIL_COND_V(gradient.colorsTextureHeight != 256, false);

			nsvg::CachedPaint cached(texture, textureRowBytes, 256);
			cached.init(nsvgPaint, opacity);
		}

		ERR_FAIL_NULL_V(gradient.matrix, false);
		paint->getGradientMatrix(gradient.matrix, scale);

		return true;
	}

public:
	AbstractPaintFeed(TovePaintData &data, float scale) :
		paintData(data), scale(scale) {

		paintData.style = PAINT_UNDEFINED;
	}
};

class PaintFeedBase : public AbstractPaintFeed, public Observer {
protected:
	const PathRef path;
	const ToveChangeFlags CHANGED_STYLE;
	bool changed;

	inline const PaintRef &getColor() const {
		static const PaintRef none;

		switch (CHANGED_STYLE) {
			case CHANGED_LINE_STYLE:
				return path->getLineWidth() >= 0.0f ?
					path->getLineColor() : none;
			case CHANGED_FILL_STYLE:
				return path->getFillColor();
			default:
				WARN_PRINT("Unknown style");
		}
		return none;
	}

public:
	PaintFeedBase(
		const PathRef &path,
		TovePaintData &data,
		float scale,
		ToveChangeFlags CHANGED_STYLE) :

		AbstractPaintFeed(data, scale),
		path(path),
		CHANGED_STYLE(CHANGED_STYLE) {

		path->addObserver(this);
		changed = false;

		std::memset(&paintData, 0, sizeof(paintData));

		const PaintRef &color = getColor();
		const TovePaintType style = getStyle(color);
		paintData.style = style;
		paintData.gradient.numColors = determineNumColors(color);

		if (style == PAINT_SHADER) {
			shader = std::static_pointer_cast<PaintShader>(color);
			paintData.shader = shader.get();
		}
	}

	virtual ~PaintFeedBase() {
		path->removeObserver(this);
	}

	virtual void observableChanged(Observable *observable, ToveChangeFlags what) {
		if (what & CHANGED_STYLE) {
			changed = true;
		}
	}

	inline int getNumColors() const {
		return paintData.gradient.numColors;
	}

	inline ToveChangeFlags beginUpdate() {
		if (changed) {
			changed = false;

			const PaintRef &color = getColor();
			const TovePaintType style = getStyle(color);

			if (paintData.style != style ||
				determineNumColors(color) != paintData.gradient.numColors) {
				return CHANGED_RECREATE;
			}

			return CHANGED_STYLE;
		} else {
			return 0;
		}
	}

	inline ToveChangeFlags endUpdate() {
		return update(getColor(), path->getOpacity()) ? CHANGED_STYLE : 0;
	}
};

class PaintFeed : public PaintFeedBase {
private:
	TovePaintData _data;
	const PaintIndex paintIndex;

public:
	PaintFeed(const PaintFeed &feed) :
		PaintFeedBase(feed.path, _data, feed.scale, feed.CHANGED_STYLE), paintIndex(feed.paintIndex) {
	}

	PaintFeed(const PathRef &path, float scale, ToveChangeFlags changed, const PaintIndex &paintIndex) :
		PaintFeedBase(path, _data, scale, changed), paintIndex(paintIndex) {
	}

	void bindPaintIndices(const ToveGradientData &data) {
		const int size = 3 * data.matrixRows;

		paintData.gradient.matrix = data.matrix + paintIndex.getGradientIndex() * size;
		paintData.gradient.colorsTexture = data.colorsTexture + 4 * paintIndex.getColorIndex();
		paintData.gradient.colorsTextureRowBytes = data.colorsTextureRowBytes;
		paintData.gradient.colorsTextureHeight = data.colorsTextureHeight;

		update(getColor(), path->getOpacity());
	}
};

class LinePaintFeed : public PaintFeedBase {
public:
	LinePaintFeed(const PathRef &path, TovePaintData &data, float scale) :
		PaintFeedBase(path, data, scale, CHANGED_LINE_STYLE) {
	}
};

class FillPaintFeed : public PaintFeedBase {
public:
	FillPaintFeed(const PathRef &path, TovePaintData &data, float scale) :
		PaintFeedBase(path, data, scale, CHANGED_FILL_STYLE) {
	}
};

END_TOVE_NAMESPACE

#endif // __TOVE_SHADER_COLOR
