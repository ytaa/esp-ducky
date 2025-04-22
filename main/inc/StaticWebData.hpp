#pragma once

extern char _binary_index_html_start;
extern char _binary_index_html_end;
extern char _binary_script_js_start;
extern char _binary_script_js_end;
extern char _binary_style_css_start;
extern char _binary_style_css_end;
extern char _binary_favicon_ico_start;
extern char _binary_favicon_ico_end;

#define STATIC_WEB_DATA_PTR(file) ((const char*)&_binary_ ## file ## _start)
#define STATIC_WEB_DATA_SIZE(file) ((size_t)(&_binary_ ## file ## _end - &_binary_ ## file ## _start))
