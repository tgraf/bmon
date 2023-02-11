
#ifndef __BMON_LAYOUT_H_
#define __BMON_LAYOUT_H_

#include <bmon/bmon.h>
#include <bmon/conf.h>
#include <bmon/unit.h>
#include <bmon/utils.h>


static int parse_color(const char* color)
{
    int color_code = -1;

    if ((strcasestr(color, "red") != NULL))
        color_code = COLOR_RED;
    else if ((strcasestr(color, "green") != NULL))
        color_code = COLOR_GREEN;
    else if ((strcasestr(color, "white") != NULL))
        color_code = COLOR_WHITE;
    else if ((strcasestr(color, "black") != NULL))
        color_code = COLOR_BLACK;
    else if ((strcasestr(color, "blue") != NULL))
        color_code = COLOR_BLUE;
    else if ((strcasestr(color, "yellow") != NULL))
        color_code = COLOR_YELLOW;
    else if ((strcasestr(color, "magenta") != NULL))
        color_code = COLOR_MAGENTA;
    else if ((strcasestr(color, "cyan") != NULL))
        color_code = COLOR_CYAN;
    else if ((atoi(color) >= 0))
        color_code = atoi(color);

    return color_code;
}

/*
    A_NORMAL        Normal display (no highlight)
    A_STANDOUT      Best highlighting mode of the terminal.
    A_UNDERLINE     Underlining
    A_REVERSE       Reverse video
    A_BLINK         Blinking
    A_DIM           Half bright
    A_BOLD          Extra bright or bold
    A_PROTECT       Protected mode
    A_INVIS         Invisible or blank mode
    A_ALTCHARSET    Alternate character set
    A_CHARTEXT      Bit-mask to extract a character
*/

static int parse_attribute(const char* attr)
{
    /* no attribute is valid, so we have nothing to do */
    if (attr == NULL)
        return 0;

    if ((strcasestr(attr, "normal") != NULL))
        return A_NORMAL;
    else if ((strcasestr(attr, "standout") != NULL))
        return A_STANDOUT;
    else if ((strcasestr(attr, "underline") != NULL))
        return A_UNDERLINE;
    else if ((strcasestr(attr, "reverse") != NULL))
        return A_REVERSE;
    else if ((strcasestr(attr, "blink") != NULL))
        return A_BLINK;
    else if ((strcasestr(attr, "dim") != NULL))
        return A_DIM;
    else if ((strcasestr(attr, "bold") != NULL))
        return A_BOLD;
    else if ((strcasestr(attr, "protect") != NULL))
        return A_PROTECT;
    else if ((strcasestr(attr, "invis") != NULL))
        return A_INVIS;
    else if ((strcasestr(attr, "altcharset") != NULL))
        return A_ALTCHARSET;
    else if ((strcasestr(attr, "chartext") != NULL))
        return A_CHARTEXT;

    return -1;
}


static void add_layout(const char *layout_name, cfg_t *color_cfg)
{
    const char *fg, *bg, *attr_str = NULL;
    int size = -1, fg_code, bg_code, attr_mask, layout_idx = 0;

    size = cfg_size(color_cfg, "color_pair");
    fg = cfg_getnstr(color_cfg, "color_pair", 0);
    bg = cfg_getnstr(color_cfg, "color_pair", 1);
    if (size > 2)
        attr_str = cfg_getnstr(color_cfg, "color_pair", 2);

    fg_code = parse_color(fg);
    bg_code = parse_color(bg);
    if (fg_code == -1 || bg_code == -1) {
        quit("Unknown color [%s]: %s\n", (fg_code == -1) ? "fg" : "bg",
                (fg_code == -1) ? fg : bg);
    }
    attr_mask = parse_attribute(attr_str);
    if (attr_mask == -1) {
        quit("Unknown attribute: '%s'\n", attr_str);
    }

    DBG("%s:\tfg: %s bg: %s attr: %s\n", layout_name, fg, bg, attr_str);

    if ((strcasecmp(layout_name, "default") == 0))
        layout_idx = LAYOUT_DEFAULT;
    else if ((strcasecmp(layout_name, "statusbar") == 0))
        layout_idx = LAYOUT_STATUSBAR;
    else if ((strcasecmp(layout_name, "header") == 0))
        layout_idx = LAYOUT_HEADER;
    else if ((strcasecmp(layout_name, "list") == 0))
        layout_idx = LAYOUT_LIST;
    else if ((strcasecmp(layout_name, "selected") == 0))
        layout_idx = LAYOUT_SELECTED;
    else if ((strcasecmp(layout_name, "rx_graph") == 0))
        layout_idx = LAYOUT_RX_GRAPH;
    else if ((strcasecmp(layout_name, "tx_graph") == 0))
        layout_idx = LAYOUT_TX_GRAPH;
    else {
        quit("Unknown layout name: '%s'\n", layout_name);
    }

    struct layout l = { fg_code, bg_code, attr_mask};
    cfg_layout[layout_idx] = l;
}

#endif /* __BMON_LAYOUT_H_ */
