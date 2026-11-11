/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The dock-app's face: a small OPENSTEP instrument panel (after YAWMPPP and
 * WMXRES). A sunken dark display shows tabbed-dock.svg large, with the
 * shelf in miniature at the dock's edge and alignment; under it two pairs
 * of buttons, ◀ ▶ (alignment) and ▲ ▼ (edge, the current one latched
 * down), and a lamp between them: lit while the dock runs, amber while it
 * is hidden, off when it is not running.
 *
 * Everything sits on whole pixels: 1 px lines on the half pixel, the arrows
 * drawn pixel by pixel, the icon rendered once at its size (no scaling at
 * each redraw).
 */
#include "btd-dockapp-draw.h"

typedef struct {
    double face[3], face_hover[3], face_latched[3], hi[3], sh[3], outer[3], arrow[3], arrow_on[3];
    double well[3], well_out[3], well_in[3], well_hi[3], well_hi_in[3];
    double lcd[3], lcd_hi[3], amber[3], amber_hi[3], lamp_off[3];
} Palette;

/* Window Maker's grey buttons and a deep display; in the dark theme,
 * graphite with softer highlights (not the light one inverted). The
 * display's phosphor and the lamp are the same in both. */
static const Palette light = {
    .face = { 0.70, 0.70, 0.71 },       .face_hover = { 0.80, 0.80, 0.81 }, .face_latched = { 0.60, 0.60, 0.61 },
    .hi = { 1, 1, 1 },                  .sh = { 0.40, 0.40, 0.42 },         .outer = { 0, 0, 0 },
    .arrow = { 0, 0, 0 },               .arrow_on = { 0.0, 0.40, 0.38 },
    .well = { 0.12, 0.13, 0.15 },       .well_out = { 0.38, 0.38, 0.40 },   .well_in = { 0, 0, 0 },
    .well_hi = { 1, 1, 1 },             .well_hi_in = { 0.80, 0.80, 0.81 },
    .lcd = { 0.25, 0.80, 0.74 },        .lcd_hi = { 0.62, 0.96, 0.90 },
    .amber = { 0.95, 0.66, 0.16 },      .amber_hi = { 1.0, 0.88, 0.55 },    .lamp_off = { 0.20, 0.23, 0.23 },
};
static const Palette dark = {
    .face = { 0.30, 0.30, 0.32 },       .face_hover = { 0.39, 0.39, 0.41 }, .face_latched = { 0.22, 0.22, 0.23 },
    .hi = { 0.54, 0.54, 0.56 },         .sh = { 0.16, 0.16, 0.17 },         .outer = { 0, 0, 0 },
    .arrow = { 0.90, 0.90, 0.90 },      .arrow_on = { 0.36, 0.90, 0.84 },
    .well = { 0.055, 0.06, 0.07 },      .well_out = { 0.14, 0.14, 0.15 },   .well_in = { 0, 0, 0 },
    .well_hi = { 0.50, 0.50, 0.52 },    .well_hi_in = { 0.34, 0.34, 0.36 },
    .lcd = { 0.25, 0.80, 0.74 },        .lcd_hi = { 0.62, 0.96, 0.90 },
    .amber = { 0.95, 0.66, 0.16 },      .amber_hi = { 1.0, 0.88, 0.55 },    .lamp_off = { 0.16, 0.18, 0.18 },
};

static void
rgb(cairo_t *cr, const double *c)
{
    cairo_set_source_rgb(cr, c[0], c[1], c[2]);
}

static void
fill(cairo_t *cr, const double *c, double x, double y, double w, double h)
{
    rgb(cr, c);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
}

/* An L of 1 px lines: from the bottom left over the top left to the top
 * right (top/left), or from the top right over the bottom right to the
 * bottom left. Inset by i. */
static void
edge_tl(cairo_t *cr, const double *c, const BtdRect *r, int i)
{
    rgb(cr, c);
    cairo_move_to(cr, r->x + i + 0.5, r->y + r->height - i - 0.5);
    cairo_line_to(cr, r->x + i + 0.5, r->y + i + 0.5);
    cairo_line_to(cr, r->x + r->width - i - 0.5, r->y + i + 0.5);
    cairo_stroke(cr);
}

static void
edge_br(cairo_t *cr, const double *c, const BtdRect *r, int i)
{
    rgb(cr, c);
    cairo_move_to(cr, r->x + r->width - i - 0.5, r->y + i + 0.5);
    cairo_line_to(cr, r->x + r->width - i - 0.5, r->y + r->height - i - 0.5);
    cairo_line_to(cr, r->x + i + 0.5, r->y + r->height - i - 0.5);
    cairo_stroke(cr);
}

/* A Window Maker button. Raised: white top/left, black and grey
 * bottom/right. Down (held, or latched as the current edge): the other way
 * round, its face a shade darker when latched. */
static void
button(cairo_t *cr, const Palette *p, const BtdRect *r, gboolean down, gboolean latched, gboolean hover)
{
    const double *face = latched ? p->face_latched : hover && !down ? p->face_hover : p->face;
    fill(cr, face, r->x, r->y, r->width, r->height);
    if (down || latched) {
        edge_tl(cr, p->outer, r, 0);
        edge_tl(cr, p->sh, r, 1);
        edge_br(cr, p->hi, r, 0);
    } else {
        edge_tl(cr, p->hi, r, 0);
        edge_br(cr, p->outer, r, 0);
        edge_br(cr, p->sh, r, 1);
    }
}

/* A pixel arrow: ▲ ▼ 8×4, ◀ ▶ 4×8, centred in the button's inside (it
 * moves 1 px down and right with the button). */
static void
arrow(cairo_t *cr, const BtdRect *r, BtdRegion dir, gboolean down, const double *c)
{
    int o = down ? 1 : 0;
    gboolean vertical = dir == BTD_REGION_TOP || dir == BTD_REGION_BOTTOM;
    int x0 = r->x + (vertical ? 2 : 4) + o, y0 = r->y + (vertical ? 4 : 2) + o;
    rgb(cr, c);
    for (int i = 0; i < 4; i++) {
        switch (dir) {
        case BTD_REGION_TOP:
            cairo_rectangle(cr, x0 + 3 - i, y0 + i, 2 + 2 * i, 1);
            break;
        case BTD_REGION_BOTTOM:
            cairo_rectangle(cr, x0 + i, y0 + i, 8 - 2 * i, 1);
            break;
        case BTD_REGION_LEFT:
            cairo_rectangle(cr, x0 + i, y0 + 3 - i, 1, 2 + 2 * i);
            break;
        default:
            cairo_rectangle(cr, x0 + i, y0 + i, 1, 8 - 2 * i);
            break;
        }
    }
    cairo_fill(cr);
}

/* The display: sunken 2 px (grey and black on top/left, white and light
 * grey on bottom/right), dark inside. */
static void
display(cairo_t *cr, const Palette *p, const BtdRect *r)
{
    fill(cr, p->well, r->x, r->y, r->width, r->height);
    edge_br(cr, p->well_hi, r, 0);
    edge_tl(cr, p->well_out, r, 0);
    edge_br(cr, p->well_hi_in, r, 1);
    edge_tl(cr, p->well_in, r, 1);
}

/* The lamp, in a small sunken socket: a 4×4 light with a highlight. */
static void
lamp(cairo_t *cr, const Palette *p, const BtdDockappLook *look)
{
    BtdRect r;
    btd_dockapp_led_rect(&r);
    fill(cr, p->well, r.x, r.y, r.width, r.height);
    edge_tl(cr, p->well_out, &r, 0);
    edge_br(cr, p->well_hi_in, &r, 0);
    const double *c = p->lamp_off, *h = NULL;
    if (look->running && look->st->visible) {
        c = p->lcd;
        h = p->lcd_hi;
    } else if (look->running) {
        c = p->amber;
        h = p->amber_hi;
    }
    fill(cr, c, r.x + 1, r.y + 1, r.width - 2, r.height - 2);
    if (h)
        fill(cr, h, r.x + 1, r.y + 1, 2, 1), fill(cr, h, r.x + 1, r.y + 2, 1, 1);
}

cairo_surface_t *
btd_dockapp_icon_new(RsvgHandle *svg)
{
    if (!svg)
        return NULL;
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, BTD_DOCKAPP_ICON_W, BTD_DOCKAPP_ICON_H);
    cairo_t *cr = cairo_create(s);
    /* the drawing of the 64×64 document (viewBox 4..60 × 12..54) fills it */
    double k = BTD_DOCKAPP_ICON_W / 56.0;
    RsvgRectangle vp = { -4 * k, -12 * k, 64 * k, 64 * k };
    if (!rsvg_handle_render_document(svg, cr, &vp, NULL)) {
        cairo_destroy(cr);
        cairo_surface_destroy(s);
        return NULL;
    }
    cairo_destroy(cr);
    cairo_surface_flush(s);
    return s;
}

void
btd_dockapp_draw(cairo_t *cr, const BtdDockappLook *look, cairo_surface_t *icon)
{
    const Palette *p = look->dark ? &dark : &light;
    const BtdDockappState *st = look->st;
    cairo_save(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE); /* lines and arrows: whole pixels */
    cairo_set_line_width(cr, 1);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr); /* outside the shape: never shown */

    BtdRect w;
    btd_dockapp_region_rect(BTD_REGION_ICON, &w);
    display(cr, p, &w);

    if (icon) {
        /* off: the display dims, as a screen without signal */
        BtdRect ir;
        btd_dockapp_icon_rect(&ir);
        cairo_set_source_surface(cr, icon, ir.x, ir.y);
        cairo_paint_with_alpha(cr, look->running ? 1.0 : 0.42);
    }

    if (look->running) {
        /* the shelf at its edge and alignment, a 2 px phosphor bar */
        BtdRect b;
        btd_dockapp_bar_rect(st->align, st->edge, &b);
        fill(cr, p->lcd, b.x, b.y, b.width, b.height);
        fill(cr, p->lcd_hi, b.x, st->edge == BTD_EDGE_TOP ? b.y + 1 : b.y, b.width, 1);
    }

    static const BtdRegion buttons[] = { BTD_REGION_LEFT, BTD_REGION_RIGHT, BTD_REGION_TOP, BTD_REGION_BOTTOM };
    for (guint i = 0; i < G_N_ELEMENTS(buttons); i++) {
        BtdRegion b = buttons[i];
        BtdRect r;
        btd_dockapp_region_rect(b, &r);
        gboolean latched = (b == BTD_REGION_TOP && st->edge == BTD_EDGE_TOP) ||
                           (b == BTD_REGION_BOTTOM && st->edge == BTD_EDGE_BOTTOM);
        gboolean down = look->pressed == b;
        button(cr, p, &r, down, latched, look->hover == b);
        arrow(cr, &r, b, down || latched, latched ? p->arrow_on : p->arrow);
    }

    lamp(cr, p, look);
    cairo_restore(cr);
}
