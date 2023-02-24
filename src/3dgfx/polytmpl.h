#ifdef _MSC_VER
#pragma warning (disable: 4101)
#endif

void POLYFILL(struct pvertex *varr, int vnum)
{
	int i, line, top, bot;
	struct pvertex *vlast, *v0, *vn, *tab;
	int32_t x, y0, y1, dx, dy, slope, fx, fy;
#ifdef GOURAUD
	int r, g, b, dr, dg, db;
	int32_t rslope, gslope, bslope;
#ifdef BLEND_ALPHA
	int32_t a, da, aslope;
#endif
#endif	/* GOURAUD */
#ifdef TEXMAP
	int32_t u, v, du, dv, uslope, vslope;
#endif
#ifdef ZBUF
	int32_t z, dz, zslope;
	uint16_t *zptr;
#endif

	int start, len;
	g3d_pixel *fbptr, *pptr;

#if !defined(GOURAUD) && !defined(TEXMAP)
	g3d_pixel color = G3D_PACK_RGB(varr[0].r, varr[0].g, varr[0].b);
#else
	g3d_pixel color;
#endif

	vlast = varr + vnum - 1;
	top = pfill_fb.height;
	bot = 0;

	for(i=0; i<vnum; i++) {
		v0 = varr + i;
		vn = VNEXT(v0);

		if(vn->y > v0->y) {
			tab = left;
		} else {
			tab = right;
			v0 = vn;
			vn = varr + i;
		}

		dx = vn->x - v0->x;
		dy = vn->y - v0->y;
		slope = dy ? (dx << 8) / dy : 0;

		y0 = (v0->y + 0x100) & 0xffffff00;	/* start from the next scanline */
		fy = y0 - v0->y;					/* fractional part before the next scanline */
		fx = (fy * slope) >> 8;				/* X adjust for the step to the next scanline */
		x = v0->x + fx;						/* adjust X */
		y1 = vn->y & 0xffffff00;			/* last scanline of the edge <= vn->y */

		line = y0 >> 8;
		if(line < top) top = line;
		if((y1 >> 8) > bot) bot = y1 >> 8;

		if(dy == 0) {
			/* horizontal edge */
			left[line].x = v0->x >> 8;
			right[line].x = vn->x >> 8;
#ifdef GOURAUD
			left[line].r = v0->r << COLOR_SHIFT;
			left[line].g = v0->g << COLOR_SHIFT;
			left[line].b = v0->b << COLOR_SHIFT;
			right[line].r = vn->r << COLOR_SHIFT;
			right[line].g = vn->g << COLOR_SHIFT;
			right[line].b = vn->b << COLOR_SHIFT;
#ifdef BLEND_ALPHA
			left[line].a = v0->a << COLOR_SHIFT;
			right[line].a = vn->a << COLOR_SHIFT;
#endif	/* BLEND_ALPHA */
#endif
#ifdef TEXMAP
			left[line].u = v0->u;
			left[line].v = v0->v;
			right[line].u = vn->u;
			right[line].v = vn->v;
#endif
#ifdef ZBUF
			left[line].z = v0->z;
			right[line].z = vn->z;
#endif
			continue;
		}

		/* regular non-horizontal edge */
#ifdef GOURAUD
		r = (v0->r << COLOR_SHIFT);
		g = (v0->g << COLOR_SHIFT);
		b = (v0->b << COLOR_SHIFT);
		dr = (vn->r << COLOR_SHIFT) - r;
		dg = (vn->g << COLOR_SHIFT) - g;
		db = (vn->b << COLOR_SHIFT) - b;
		rslope = (dr << 8) / dy;
		gslope = (dg << 8) / dy;
		bslope = (db << 8) / dy;
#ifdef BLEND_ALPHA
		a = (v0->a << COLOR_SHIFT);
		da = (vn->a << COLOR_SHIFT) - a;
		aslope = (da << 8) / dy;
#endif	/* BLEND_ALPHA */
#endif	/* GOURAUD */
#ifdef TEXMAP
		u = v0->u;
		v = v0->v;
		du = vn->u - v0->u;
		dv = vn->v - v0->v;
		uslope = (du << 8) / dy;
		vslope = (dv << 8) / dy;
#endif
#ifdef ZBUF
		z = v0->z;
		dz = vn->z - v0->z;
		zslope = (dz << 8) / dy;
#endif

		if(line > 0) tab += line;

		while(line <= (y1 >> 8) && line < pfill_fb.height) {
			if(line >= 0) {
				int val = x < 0 ? 0 : x >> 8;
				tab->x = val < pfill_fb.width ? val : pfill_fb.width - 1;
#ifdef GOURAUD
				/* we'll store the color in the edge tables with COLOR_SHIFT extra bits of precision */
				tab->r = r;
				tab->g = g;
				tab->b = b;
				r += rslope;
				g += gslope;
				b += bslope;
#ifdef BLEND_ALPHA
				tab->a = a;
				a += aslope;
#endif
#endif	/* GOURAUD */
#ifdef TEXMAP
				tab->u = u;
				tab->v = v;
				u += uslope;
				v += vslope;
#endif
#ifdef ZBUF
				tab->z = z;
				z += zslope;
#endif

				tab++;
			}
			x += slope;
			line++;
		}
	}

	if(top < 0) top = 0;
	if(bot >= pfill_fb.height) bot = pfill_fb.height - 1;

	fbptr = pfill_fb.pixels + top * pfill_fb.width;
	for(i=top; i<=bot; i++) {
		start = left[i].x;
		len = right[i].x - start;

#ifdef GOURAUD
		r = left[i].r;
		g = left[i].g;
		b = left[i].b;
#ifdef BLEND_ALPHA
		a = left[i].a;
#endif	/* BLEND_ALPHA */
#endif
#ifdef TEXMAP
		u = left[i].u;
		v = left[i].v;
#endif
#ifdef ZBUF
		z = left[i].z;
		zptr = pfill_zbuf + i * pfill_fb.width + (x >> 8);
#endif

#if defined(GOURAUD) || defined(TEXMAP) || defined(ZBUF)
		dx = len == 0 ? 256 : (len << 8);

#ifdef GOURAUD
		dr = right[i].r - left[i].r;
		dg = right[i].g - left[i].g;
		db = right[i].b - left[i].b;
		rslope = (dr << 8) / dx;
		gslope = (dg << 8) / dx;
		bslope = (db << 8) / dx;
#ifdef BLEND_ALPHA
		a = left[i].a;
		da = right[i].a - left[i].a;
		aslope = (da << 8) / dx;
#endif	/* BLEND_ALPHA */
#endif	/* GOURAUD */
#ifdef TEXMAP
		du = right[i].u - left[i].u;
		dv = right[i].v - left[i].v;
		uslope = (du << 8) / dx;
		vslope = (dv << 8) / dx;
#endif
#ifdef ZBUF
		dz = right[i].z - left[i].z;
		zslope = (dz << 8) / dx;
#endif
#endif	/* gouraud/texmap/zbuf */


		pptr = fbptr + start;
		while(len-- > 0) {
#if defined(GOURAUD) || defined(TEXMAP) || defined(BLEND_ALPHA) || defined(BLEND_ADD)
			int cr, cg, cb;
#endif
#if defined(BLEND_ALPHA) || defined(BLEND_ADD)
			g3d_pixel fbcol;
#endif
#ifdef BLEND_ALPHA
			int alpha, inv_alpha;
#endif

#ifdef ZBUF
			int32_t cz = z;
			z += zslope;

			if(z <= *zptr) {
				*zptr++ = z;
			} else {
#ifdef GOURAUD
				r += rslope;
				g += gslope;
				b += bslope;
#ifdef BLEND_ALPHA
				a += aslope;
#endif
#endif
#ifdef TEXMAP
				u += uslope;
				v += vslope;
#endif
				/* skip pixel */
				pptr++;
				zptr++;
				continue;
			}
#endif	/* ZBUF */

#ifdef GOURAUD
			/* we upped the color precision to while interpolating the
			 * edges, now drop the extra bits before packing
			 */
			cr = r < 0 ? 0 : (r >> COLOR_SHIFT);
			cg = g < 0 ? 0 : (g >> COLOR_SHIFT);
			cb = b < 0 ? 0 : (b >> COLOR_SHIFT);
			r += rslope;
			g += gslope;
			b += bslope;
#ifdef BLEND_ALPHA
			a += aslope;
#else
			if(cr > 255) cr = 255;
			if(cg > 255) cg = 255;
			if(cb > 255) cb = 255;
#endif	/* BLEND_ALPHA */
#endif	/* GOURAUD */
#ifdef TEXMAP
			{
				int tx = (u >> (16 - pfill_tex.xshift)) & pfill_tex.xmask;
				int ty = (v >> (16 - pfill_tex.yshift)) & pfill_tex.ymask;
				g3d_pixel texel = pfill_tex.pixels[(ty << pfill_tex.xshift) + tx];
#ifdef GOURAUD
				/* This is not correct, should be /255, but it's much faster
				 * to shift by 8 (/256), and won't make a huge difference
				 */
				cr = (cr * G3D_UNPACK_R(texel)) >> 8;
				cg = (cg * G3D_UNPACK_G(texel)) >> 8;
				cb = (cb * G3D_UNPACK_B(texel)) >> 8;
#else
				cr = G3D_UNPACK_R(texel);
				cg = G3D_UNPACK_G(texel);
				cb = G3D_UNPACK_B(texel);
#endif
			}
			u += uslope;
			v += vslope;
#endif	/* TEXMAP */

#if defined(BLEND_ALPHA) || defined(BLEND_ADD)
#if !defined(GOURAUD) && !defined(TEXMAP)
			/* flat version: cr,cg,cb are uninitialized so far */
			cr = varr[0].r;
			cg = varr[0].g;
			cb = varr[0].b;
#endif
			fbcol = *pptr;

#ifdef BLEND_ALPHA
#ifdef GOURAUD
			alpha = a >> COLOR_SHIFT;
#else
			alpha = varr[0].a;
#endif
			inv_alpha = 255 - alpha;
			cr = (cr * alpha + G3D_UNPACK_R(fbcol) * inv_alpha) >> 8;
			cg = (cg * alpha + G3D_UNPACK_G(fbcol) * inv_alpha) >> 8;
			cb = (cb * alpha + G3D_UNPACK_B(fbcol) * inv_alpha) >> 8;
#else	/* !BLEND_ALPHA (so BLEND_ADD) */
			cr += G3D_UNPACK_R(fbcol);
			cg += G3D_UNPACK_R(fbcol);
			cb += G3D_UNPACK_R(fbcol);
#endif
			if(cr > 255) cr = 255;
			if(cg > 255) cg = 255;
			if(cb > 255) cb = 255;
#endif	/* BLEND(ALPHA|ADD) */

#if defined(GOURAUD) || defined(TEXMAP) || defined(BLEND_ALPHA) || defined(BLEND_ADD)
			color = G3D_PACK_RGB(cr, cg, cb);
#endif

#ifdef DEBUG_OVERDRAW
			*pptr++ += DEBUG_OVERDRAW;
#else
			*pptr++ = color;
#endif
		}
		fbptr += pfill_fb.width;
	}
}
