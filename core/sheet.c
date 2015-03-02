

#include "bootpack.h"

#define SHEET_USE		1

struct SHTCTL *shtctl_init(struct MEMMAN * memman,unsigned char * vram,int xsize,int ysize)
{
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof (struct SHTCTL));
	if( ctl ==0){
		goto err;
		}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1;			/* default no sheet */
	for(i = 0 ;i < MAX_SHEETS;i++){
		ctl->sheets0[i].flags=0;		/* unused */
		ctl->sheets0[i].ctl = ctl; /* record ctl */
		}
err:
	return ctl;		
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
	struct SHEET *sht;
	int i;
	for(i = 0; i < MAX_SHEETS; i++){
		if(ctl->sheets0[i].flags == 0){
			sht = &ctl->sheets0[i];
			sht->flags = SHEET_USE; 
			sht->height = -1;   /* hide */
			return sht;
			}
		}
	return 0;		/* all sheets have been used */
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, c, *vram = ctl->vram;
	struct SHEET *sht;
	/* Refresh outside the range images without careful additional corrections */
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = 0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		/*  */
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				c = buf[by * sht->bxsize + bx];
				if (c != sht->col_inv) {
					vram[vy * ctl->xsize + vx] = c;
				}
			}
		}
	}
	return;
}

void sheet_updown(struct SHEET *sht, int height)
{
	struct SHTCTL *ctl = sht->ctl;
	int h, old = sht->height;  /*save the height before set in  old*/

	/* if height > top or < -1, refine */
	if(height > ctl->top + 1){
		height = ctl->top + 1;
		}
	if(height < -1){
		height = -1;
		}
	sht->height = height;  /* set height */

/* rearrangment */
	if(old > height){     /* lower than before */
		if(height >= 0){
			for(h =old; h> height;h--){
				/* put up in the middle of the raise */
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
				}
			ctl->sheets[height] = sht;
			}else {   /* hide */
			if(ctl->top > old){ 
				/* put the top down */
				for(h = old ; h < ctl->top; h++){
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
					}
				}
			ctl->top--;	/*because the sheets to display - 1*/
				}
			sheet_refreshsub( ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize );  /* Redraw the sheets */
		}else if (old < height){ /* lower than before */
		if(old >= 0) {
			/*put down in the middle of the raise */
			for(h = old; h < height ; h++){
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h] ->height = h;
				}
			ctl->sheets[height] = sht;
			}else {       /* change hide to display */
			/* put the top up */
			for(h =ctl->top; h >= height; h--){
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1] ->height = h + 1;
				}
			ctl->sheets[height] = sht;
			ctl->top++;
				}				
				sheet_refreshsub( ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize );  /* Redraw the sheets */
		}
		return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
	if (sht->height >= 0) { /* If it is showing, refresh is according to the new layer information*/
		sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
	}
	return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0)
{
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) { /* If it is showing, refresh is according to the new layer information*/
		sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
		sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
	}
	return;
}

void sheet_free(struct SHEET *sht)
{
	if( sht->height >=0){
		sheet_updown(sht, -1); /* */
		}
	sht->flags = 0;
	return;
}
