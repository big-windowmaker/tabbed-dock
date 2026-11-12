/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The dock-app's face on image surfaces, without X11: what is drawn matches
 * the regions that take the clicks (a held or hovered button changes only
 * its own pixels), nothing is drawn outside the window's shape, the states
 * show (latched edge, the shelf's alignment, the lamp), both themes have
 * contrast, the icon is large, and a redraw leaves nothing of the previous
 * one.
 *
 * BTD_DOCKAPP_RENDER_DIR=dir also writes the faces as PNG (1× and 4×).
 */
#include "btd-dockapp-draw.h"

#include <math.h>

static RsvgHandle *svg;
static cairo_surface_t *icon;

typedef struct {
    BtdAlign align;
    BtdEdge edge;
    gboolean visible, running, dark;
    BtdRegion pressed, hover;
} Case;

static cairo_surface_t *
render(const Case *c)
{
    BtdDockappState st = { .align = c->align, .edge = c->edge, .visible = c->visible };
    BtdDockappLook look = { .st = &st, .dark = c->dark, .running = c->running, .pressed = c->pressed,
                            .hover = c->hover };
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_RGB24, BTD_DOCKAPP_SIZE, BTD_DOCKAPP_SIZE);
    cairo_t *cr = cairo_create(s);
    btd_dockapp_draw(cr, &look, icon);
    cairo_destroy(cr);
    cairo_surface_flush(s);
    return s;
}

static guint32
px(cairo_surface_t *s, int x, int y)
{
    const unsigned char *d = cairo_image_surface_get_data(s);
    return ((const guint32 *)(d + y * cairo_image_surface_get_stride(s)))[x] & 0xffffff;
}

static double
lum(guint32 p)
{
    return (0.2126 * ((p >> 16) & 255) + 0.7152 * ((p >> 8) & 255) + 0.0722 * (p & 255)) / 255.0;
}

static gboolean
inside(const BtdRect *r, int x, int y)
{
    return x >= r->x && x < r->x + r->width && y >= r->y && y < r->y + r->height;
}

static const Case base_light = { BTD_ALIGN_CENTER, BTD_EDGE_BOTTOM, TRUE, TRUE, FALSE, BTD_REGION_NONE,
                                 BTD_REGION_NONE };

static void
test_icon(void)
{
    g_assert_null(btd_dockapp_icon_new(NULL));
    g_assert_nonnull(icon);
    g_assert_cmpint(cairo_image_surface_get_width(icon), ==, BTD_DOCKAPP_ICON_W);
    /* the shelf's drawing spans most of it: at least 40 px wide, 28 tall */
    int minx = 999, maxx = -1, miny = 999, maxy = -1;
    const unsigned char *d = cairo_image_surface_get_data(icon);
    int stride = cairo_image_surface_get_stride(icon);
    for (int y = 0; y < BTD_DOCKAPP_ICON_H; y++)
        for (int x = 0; x < BTD_DOCKAPP_ICON_W; x++)
            if ((((const guint32 *)(d + y * stride))[x] >> 24) > 200) {
                minx = MIN(minx, x), maxx = MAX(maxx, x), miny = MIN(miny, y), maxy = MAX(maxy, y);
            }
    g_assert_cmpint(maxx - minx + 1, >=, 40);
    g_assert_cmpint(maxy - miny + 1, >=, 28);
    /* inside the display, clear of the shelf's bars */
    BtdRect ir, w, top, bottom;
    btd_dockapp_icon_rect(&ir);
    btd_dockapp_region_rect(BTD_REGION_ICON, &w);
    btd_dockapp_bar_rect(BTD_ALIGN_FILL, BTD_EDGE_TOP, &top);
    btd_dockapp_bar_rect(BTD_ALIGN_FILL, BTD_EDGE_BOTTOM, &bottom);
    g_assert_cmpint(ir.x, >=, w.x + 2);
    g_assert_cmpint(ir.x + ir.width, <=, w.x + w.width - 2);
    g_assert_cmpint(ir.y + miny, >=, top.y + top.height);
    g_assert_cmpint(ir.y + maxy, <, bottom.y);
}

/* Every face, in every state: black outside the shape (what is drawn is
 * what the window shows). */
static void
test_shape(void)
{
    BtdRect shape[6];
    int n = btd_dockapp_shape(shape);
    g_assert_cmpint(n, ==, 6);
    for (int dark = 0; dark < 2; dark++)
        for (int running = 0; running < 2; running++)
            for (BtdRegion pr = BTD_REGION_NONE; pr <= BTD_REGION_ICON; pr++) {
                Case c = base_light;
                c.dark = dark, c.running = running, c.pressed = pr, c.hover = pr;
                c.edge = pr == BTD_REGION_TOP ? BTD_EDGE_TOP : BTD_EDGE_BOTTOM;
                cairo_surface_t *s = render(&c);
                for (int y = 0; y < BTD_DOCKAPP_SIZE; y++)
                    for (int x = 0; x < BTD_DOCKAPP_SIZE; x++) {
                        gboolean in = FALSE;
                        for (int i = 0; i < n; i++)
                            in = in || inside(&shape[i], x, y);
                        if (!in)
                            g_assert_cmphex(px(s, x, y), ==, 0);
                    }
                cairo_surface_destroy(s);
            }
    /* the shape's parts never overlap, the controls are among them, and
     * the lamp is no control */
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            for (int y = shape[i].y; y < shape[i].y + shape[i].height; y++)
                for (int x = shape[i].x; x < shape[i].x + shape[i].width; x++)
                    g_assert_false(inside(&shape[j], x, y));
    BtdRect led;
    btd_dockapp_led_rect(&led);
    g_assert_cmpint(btd_dockapp_region_at(led.x + 2, led.y + 2), ==, BTD_REGION_NONE);
}

/* A held button, or one under the pointer, changes only its own pixels: the
 * drawing and the click region are the same rectangle. */
static void
test_feedback_matches_regions(void)
{
    static const BtdRegion buttons[] = { BTD_REGION_LEFT, BTD_REGION_RIGHT, BTD_REGION_TOP, BTD_REGION_BOTTOM };
    for (int dark = 0; dark < 2; dark++)
        for (guint b = 0; b < G_N_ELEMENTS(buttons); b++)
            for (int hover = 0; hover < 2; hover++) {
                Case c = base_light;
                c.dark = dark;
                c.edge = buttons[b] == BTD_REGION_BOTTOM ? BTD_EDGE_TOP : BTD_EDGE_BOTTOM; /* not latched */
                cairo_surface_t *idle = render(&c);
                if (hover)
                    c.hover = buttons[b];
                else
                    c.pressed = buttons[b];
                cairo_surface_t *s = render(&c);
                BtdRect r;
                btd_dockapp_region_rect(buttons[b], &r);
                int changed = 0;
                for (int y = 0; y < BTD_DOCKAPP_SIZE; y++)
                    for (int x = 0; x < BTD_DOCKAPP_SIZE; x++)
                        if (px(s, x, y) != px(idle, x, y)) {
                            g_assert_true(inside(&r, x, y));
                            changed++;
                        }
                /* hover: the face lights up; held: the whole button goes down */
                g_assert_cmpint(changed, >=, hover ? 40 : 60);
                cairo_surface_destroy(s);
                cairo_surface_destroy(idle);
            }
}

/* The current edge's button stays down (latched) with its arrow in the
 * accent colour; the other edge's is raised. */
static void
test_latched_edge(void)
{
    Case c = base_light;
    c.edge = BTD_EDGE_TOP;
    cairo_surface_t *top = render(&c);
    c.edge = BTD_EDGE_BOTTOM;
    cairo_surface_t *bottom = render(&c);
    BtdRect t, b;
    btd_dockapp_region_rect(BTD_REGION_TOP, &t);
    btd_dockapp_region_rect(BTD_REGION_BOTTOM, &b);
    /* latched: dark top-left line (raised: white) */
    g_assert_cmphex(px(top, t.x + 5, t.y), ==, 0);
    g_assert_cmphex(px(bottom, t.x + 5, t.y), ==, 0xffffff);
    g_assert_cmphex(px(bottom, b.x + 5, b.y), ==, 0);
    g_assert_cmphex(px(top, b.x + 5, b.y), ==, 0xffffff);
    cairo_surface_destroy(top);
    cairo_surface_destroy(bottom);
}

/* The little shelf: at the edge, with the alignment; only while running. */
static void
test_shelf_bar(void)
{
    static const BtdAlign aligns[] = { BTD_ALIGN_FILL, BTD_ALIGN_LEFT, BTD_ALIGN_CENTER, BTD_ALIGN_RIGHT };
    for (int e = 0; e < 2; e++)
        for (guint a = 0; a < G_N_ELEMENTS(aligns); a++) {
            Case c = base_light;
            c.align = aligns[a];
            c.edge = e ? BTD_EDGE_TOP : BTD_EDGE_BOTTOM;
            cairo_surface_t *s = render(&c);
            BtdRect bar;
            btd_dockapp_bar_rect(c.align, c.edge, &bar);
            /* the bar's pixels are the phosphor (green over red) */
            for (int x = bar.x; x < bar.x + bar.width; x++) {
                guint32 p = px(s, x, bar.y);
                g_assert_cmpint((p >> 8) & 255, >, ((p >> 16) & 255) + 60);
            }
            /* and the row of the other edge is not */
            BtdRect other;
            btd_dockapp_bar_rect(c.align, e ? BTD_EDGE_BOTTOM : BTD_EDGE_TOP, &other);
            guint32 p = px(s, other.x + other.width / 2, other.y);
            g_assert_cmpint((p >> 8) & 255, <, ((p >> 16) & 255) + 60);
            cairo_surface_destroy(s);
            c.running = FALSE;
            s = render(&c);
            p = px(s, bar.x + bar.width / 2, bar.y);
            g_assert_cmpint((p >> 8) & 255, <, ((p >> 16) & 255) + 60);
            cairo_surface_destroy(s);
        }
    /* on top or at the bottom, in the display, the bar never covers the
     * buttons; the alignments are where they say */
    BtdRect l, r, f, w;
    btd_dockapp_region_rect(BTD_REGION_ICON, &w);
    btd_dockapp_bar_rect(BTD_ALIGN_LEFT, BTD_EDGE_TOP, &l);
    btd_dockapp_bar_rect(BTD_ALIGN_RIGHT, BTD_EDGE_TOP, &r);
    btd_dockapp_bar_rect(BTD_ALIGN_FILL, BTD_EDGE_BOTTOM, &f);
    g_assert_cmpint(l.x, ==, f.x);
    g_assert_cmpint(r.x + r.width, ==, f.x + f.width);
    g_assert_cmpint(f.y + f.height, <=, w.y + w.height - 2);
    g_assert_cmpint(l.y, >=, w.y + 2);
}

/* The lamp: lit (phosphor) while running, amber while hidden, off when the
 * dock does not run; the icon dims when it does not run. */
static void
test_lamp_and_dimming(void)
{
    BtdRect led, ir;
    btd_dockapp_led_rect(&led);
    btd_dockapp_icon_rect(&ir);
    for (int dark = 0; dark < 2; dark++) {
        Case c = base_light;
        c.dark = dark;
        cairo_surface_t *on = render(&c);
        c.visible = FALSE;
        cairo_surface_t *hidden = render(&c);
        c.running = FALSE;
        cairo_surface_t *off = render(&c);
        int cx = led.x + led.width - 2, cy = led.y + led.height - 2; /* off the highlight */
        guint32 pon = px(on, cx, cy), phid = px(hidden, cx, cy), poff = px(off, cx, cy);
        g_assert_cmpint((pon >> 8) & 255, >, ((pon >> 16) & 255) + 60); /* green-cyan */
        g_assert_cmpint((phid >> 16) & 255, >, (phid & 255) + 100);     /* amber */
        g_assert_cmpfloat(lum(poff), <, 0.3);
        g_assert_cmpfloat(lum(pon), >, lum(poff) + 0.3);
        double sum_on = 0, sum_off = 0;
        for (int y = ir.y; y < ir.y + ir.height; y++)
            for (int x = ir.x; x < ir.x + ir.width; x++)
                sum_on += lum(px(on, x, y)), sum_off += lum(px(off, x, y));
        g_assert_cmpfloat(sum_off, <, sum_on * 0.7);
        cairo_surface_destroy(on);
        cairo_surface_destroy(hidden);
        cairo_surface_destroy(off);
    }
}

/* Contrast in both themes: the arrows on the buttons, the icon on the
 * display, the display against the buttons. */
static void
test_contrast(void)
{
    for (int dark = 0; dark < 2; dark++) {
        Case c = base_light;
        c.dark = dark;
        cairo_surface_t *s = render(&c);
        BtdRect r;
        btd_dockapp_region_rect(BTD_REGION_LEFT, &r);
        double face = lum(px(s, r.x + 2, r.y + 2)), arrow = lum(px(s, r.x + 5, r.y + 6));
        g_assert_cmpfloat(fabs(face - arrow), >, 0.45);
        /* the latched arrow (accent) still reads on the latched face */
        btd_dockapp_region_rect(BTD_REGION_BOTTOM, &r);
        face = lum(px(s, r.x + 3, r.y + 3));
        arrow = lum(px(s, r.x + 6, r.y + 5));
        g_assert_cmpfloat(fabs(face - arrow), >, 0.2);
        /* the shelf (light grey) on the dark display */
        BtdRect w;
        btd_dockapp_region_rect(BTD_REGION_ICON, &w);
        double display = lum(px(s, w.x + 3, w.y + 20));
        g_assert_cmpfloat(display, <, 0.15);
        g_assert_cmpfloat(lum(px(s, 32, 37)) - display, >, 0.4); /* the shelf's front, under the tiles */
        cairo_surface_destroy(s);
    }
}

/* A redraw paints every pixel: drawing a state over another gives the same
 * face as drawing it on a fresh surface (no leftovers, nothing to clear). */
static void
test_redraw_complete(void)
{
    Case a = base_light, b = base_light;
    a.pressed = BTD_REGION_LEFT, a.edge = BTD_EDGE_TOP, a.align = BTD_ALIGN_FILL;
    b.dark = TRUE, b.running = FALSE;
    cairo_surface_t *fresh = render(&b);
    cairo_surface_t *s = render(&a);
    BtdDockappState st = { .align = b.align, .edge = b.edge, .visible = b.visible };
    BtdDockappLook look = { .st = &st, .dark = b.dark, .running = b.running, .pressed = b.pressed,
                            .hover = b.hover };
    cairo_t *cr = cairo_create(s);
    btd_dockapp_draw(cr, &look, icon);
    cairo_destroy(cr);
    cairo_surface_flush(s);
    for (int y = 0; y < BTD_DOCKAPP_SIZE; y++)
        for (int x = 0; x < BTD_DOCKAPP_SIZE; x++)
            g_assert_cmphex(px(s, x, y), ==, px(fresh, x, y));
    cairo_surface_destroy(s);
    cairo_surface_destroy(fresh);
}

/* For the documentation and the review: the faces as PNG. */
static void
write_pngs(const char *dir)
{
    static const struct {
        const char *name;
        Case c;
    } faces[] = {
        { "running", { BTD_ALIGN_CENTER, BTD_EDGE_BOTTOM, TRUE, TRUE, FALSE, 0, 0 } },
        { "top-left", { BTD_ALIGN_LEFT, BTD_EDGE_TOP, TRUE, TRUE, FALSE, 0, 0 } },
        { "fill", { BTD_ALIGN_FILL, BTD_EDGE_BOTTOM, TRUE, TRUE, FALSE, 0, 0 } },
        { "hover-right", { BTD_ALIGN_RIGHT, BTD_EDGE_BOTTOM, TRUE, TRUE, FALSE, 0, BTD_REGION_RIGHT } },
        { "pressed-left", { BTD_ALIGN_CENTER, BTD_EDGE_BOTTOM, TRUE, TRUE, FALSE, BTD_REGION_LEFT, 0 } },
        { "hidden", { BTD_ALIGN_CENTER, BTD_EDGE_BOTTOM, FALSE, TRUE, FALSE, 0, 0 } },
        { "stopped", { BTD_ALIGN_CENTER, BTD_EDGE_BOTTOM, TRUE, FALSE, FALSE, 0, 0 } },
    };
    g_mkdir_with_parents(dir, 0755);
    for (int dark = 0; dark < 2; dark++)
        for (guint i = 0; i < G_N_ELEMENTS(faces); i++) {
            Case c = faces[i].c;
            c.dark = dark;
            cairo_surface_t *s = render(&c);
            char *name = g_strdup_printf("%s/%s-%s.png", dir, dark ? "dark" : "light", faces[i].name);
            cairo_surface_write_to_png(s, name);
            g_free(name);
            cairo_surface_t *big = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 256, 256);
            cairo_t *cr = cairo_create(big);
            cairo_scale(cr, 4, 4);
            cairo_set_source_surface(cr, s, 0, 0);
            cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
            cairo_paint(cr);
            cairo_destroy(cr);
            name = g_strdup_printf("%s/%s-%s-x4.png", dir, dark ? "dark" : "light", faces[i].name);
            cairo_surface_write_to_png(big, name);
            g_free(name);
            cairo_surface_destroy(big);
            cairo_surface_destroy(s);
        }
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    char *path = g_test_build_filename(G_TEST_DIST, "..", "data", "icons", "tabbed-dock.svg", NULL);
    GError *err = NULL;
    svg = rsvg_handle_new_from_file(path, &err);
    g_assert_no_error(err);
    g_free(path);
    icon = btd_dockapp_icon_new(svg);
    const char *dir = g_getenv("BTD_DOCKAPP_RENDER_DIR");
    if (dir && *dir)
        write_pngs(dir);
    g_test_add_func("/dockapp-draw/icon", test_icon);
    g_test_add_func("/dockapp-draw/shape", test_shape);
    g_test_add_func("/dockapp-draw/feedback-matches-regions", test_feedback_matches_regions);
    g_test_add_func("/dockapp-draw/latched-edge", test_latched_edge);
    g_test_add_func("/dockapp-draw/shelf-bar", test_shelf_bar);
    g_test_add_func("/dockapp-draw/lamp-and-dimming", test_lamp_and_dimming);
    g_test_add_func("/dockapp-draw/contrast", test_contrast);
    g_test_add_func("/dockapp-draw/redraw-complete", test_redraw_complete);
    int r = g_test_run();
    cairo_surface_destroy(icon);
    g_object_unref(svg);
    return r;
}
