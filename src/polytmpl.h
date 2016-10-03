static uint32_t SCANEDGE(struct pvertex *v0, struct pvertex *v1, struct pvertex *edge)
{
	int i;
	int32_t x, dx, dy, slope;
#ifdef GOURAUD
	int r, g, b, dr, dg, db;
	int32_t rslope, gslope, bslope;
#endif
#ifdef TEXMAP
	int32_t u, v, du, dv, uslope, vslope;
#endif
	int32_t start_idx, end_idx;

	if(v0->y > v1->y) {
		struct pvertex *tmp = v0;
		v0 = v1;
		v1 = tmp;
	}

	x = v0->x;
	dy = v1->y - v0->y;
	dx = v1->x - v0->x;
	slope = (dx << 8) / dy;
#ifdef GOURAUD
	r = (v0->r << 8);
	g = (v0->g << 8);
	b = (v0->b << 8);
	dr = (v1->r << 8) - r;
	dg = (v1->g << 8) - g;
	db = (v1->b << 8) - b;
	rslope = (dr << 8) / dy;
	gslope = (dg << 8) / dy;
	bslope = (db << 8) / dy;
#endif
#ifdef TEXMAP
	u = v0->u;
	v = v0->v;
	du = v1->u - v0->u;
	dv = v1->v - v0->v;
	uslope = (du << 8) / dy;
	vslope = (dv << 8) / dy;
#endif

	start_idx = v0->y >> 8;
	end_idx = v1->y >> 8;

	for(i=start_idx; i<end_idx; i++) {
		edge[i].x = x;
		x += slope;
#ifdef GOURAUD
		/* we'll store the color in the edge tables with 8 extra bits of precision */
		edge[i].r = r;
		edge[i].g = g;
		edge[i].b = b;
		r += rslope;
		g += gslope;
		b += bslope;
#endif
#ifdef TEXMAP
		edge[i].u = u;
		edge[i].v = v;
		u += uslope;
		v += vslope;
#endif
	}

	return (uint32_t)start_idx | ((uint32_t)(end_idx - 1) << 16);
}

void POLYFILL(struct pvertex *pv, int nverts)
{
	int i;
	int topidx = 0, botidx = 0, sltop = pimg_fb.height, slbot = 0;
	struct pvertex *left, *right;
	uint16_t color;
	/* the following variables are used for interpolating horizontally accros scanlines */
#if defined(GOURAUD) || defined(TEXMAP)
	int mid;
	int32_t dx, tmp;
#else
	/* flat version, just pack the color now */
	color = PACK_RGB16(pv[0].r, pv[0].g, pv[0].b);
#endif
#ifdef GOURAUD
	int32_t r, g, b, dr, dg, db, rslope, gslope, bslope;
#endif
#ifdef TEXMAP
	int32_t u, v, du, dv, uslope, vslope;
#endif

	for(i=1; i<nverts; i++) {
		if(pv[i].y < pv[topidx].y) topidx = i;
		if(pv[i].y > pv[botidx].y) botidx = i;
	}

	left = alloca(pimg_fb.height * sizeof *left);
	right = alloca(pimg_fb.height * sizeof *right);

	for(i=0; i<nverts; i++) {
		int next = NEXTIDX(i);
		int32_t y0 = pv[i].y;
		int32_t y1 = pv[next].y;

		if((y0 >> 8) == (y1 >> 8)) {
			if(y0 > y1) {
				int idx = y0 >> 8;
				left[idx].x = pv[i].x < pv[next].x ? pv[i].x : pv[next].x;
				right[idx].x = pv[i].x < pv[next].x ? pv[next].x : pv[i].x;
			}
		} else {
			struct pvertex *edge = y0 > y1 ? left : right;
			uint32_t res = SCANEDGE(pv + i, pv + next, edge);
			uint32_t tmp = (res >> 16) & 0xffff;
			if(tmp > slbot) slbot = tmp;
			if((tmp = res & 0xffff) < sltop) {
				sltop = tmp;
			}
		}
	}

	/* find the mid-point and calculate slopes for all attributes */
#if 0
#if defined(GOURAUD) || defined(TEXMAP)
	mid = (sltop + slbot) >> 1;
	dx = right[mid].x - left[mid].x;
	if((tmp = right[sltop].x - left[sltop].x) > dx) {
		dx = tmp;
		mid = sltop;
	}
	if((tmp = right[slbot].x - left[slbot].x) > dx) {
		dx = tmp;
		mid = slbot;
	}
	if(!dx) {
		dx = 256;	/* 1 */
	}
#endif
#ifdef GOURAUD
	dr = right[mid].r - left[mid].r;
	dg = right[mid].g - left[mid].g;
	db = right[mid].b - left[mid].b;
	rslope = (dr << 8) / dx;
	gslope = (dg << 8) / dx;
	bslope = (db << 8) / dx;
#endif
#ifdef TEXMAP
	du = right[mid].u - left[mid].u;
	dv = right[mid].v - left[mid].v;
	uslope = (du << 8) / dx;
	vslope = (dv << 8) / dx;
#endif
#endif	/* 0 */

	for(i=sltop; i<=slbot; i++) {
		uint16_t *pixptr;
		int32_t x;

		x = left[i].x;
		pixptr = pimg_fb.pixels + i * pimg_fb.width + (x >> 8);

#if defined(GOURAUD) || defined(TEXMAP)
		if(!(dx = right[i].x - left[i].x)) dx = 256;	/* 1 */
#endif
#ifdef GOURAUD
		r = left[i].r;
		g = left[i].g;
		b = left[i].b;
		dr = right[i].r - left[i].r;
		dg = right[i].g - left[i].g;
		db = right[i].b - left[i].b;
		rslope = (dr << 8) / dx;
		gslope = (dg << 8) / dx;
		bslope = (db << 8) / dx;
#endif
#ifdef TEXMAP
		u = left[i].u;
		v = left[i].v;
		du = right[i].u - left[i].u;
		dv = right[i].v - left[i].v;
		uslope = (du << 8) / dx;
		vslope = (dv << 8) / dx;
#endif

		while(x <= right[i].x) {
#ifdef GOURAUD
			/* drop the extra 8 bits when packing */
			int cr = r >> 8;
			int cg = g >> 8;
			int cb = b >> 8;
			color = PACK_RGB16(cr, cg, cb);
			r += rslope;
			g += gslope;
			b += bslope;
#endif
#ifdef TEXMAP
			/* TODO */
			u += uslope;
			v += vslope;
#endif

#ifdef DEBUG_OVERDRAW
			*pixptr++ += DEBUG_OVERDRAW;
#else
			*pixptr++ = color;
#endif
			x += 256;
		}
	}
}

