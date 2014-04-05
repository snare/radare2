/* radare - Apache - Copyright 2014 dso <adam.pridgen@thecoverofnight.com | dso@rice.edu> */

#include <r_types.h>
#include <r_lib.h>
#include <r_cmd.h>
#include <r_core.h>
#include <r_cons.h>
#include <string.h>
#include <r_anal.h>

#undef R_API
#define R_API static
#undef R_IPI
#define R_IPI static

#include "../../../shlr/java/ops.c"
#include "../../../shlr/java/code.c"
#include "../../../shlr/java/class.c"
//#include "../../../shlr/java/class.h"
#undef R_API
#define R_API
#undef R_IPI
#define R_IPI

#define DO_THE_DBG 0
#undef IFDBG
#define IFDBG if (DO_THE_DBG)


typedef struct found_idx_t {
	ut16 idx;
	ut64 addr;
	const RBinJavaCPTypeObj *obj;
} RCmdJavaCPResult;

typedef int (*RCMDJavaCmdHandler) (RCore *core, const char *cmd);

static const char * r_cmd_java_strtok (const char *str1, const char b, size_t len);
static const char * r_cmd_java_consumetok (const char *str1, const char b, size_t len);
static int r_cmd_java_reload_bin_from_buf (RCore *core, RBinJavaObj *obj, ut8* buffer, ut64 len);

static int r_cmd_java_print_all_definitions( RAnal *anal );
static int r_cmd_java_print_class_definitions( RBinJavaObj *obj );
static int r_cmd_java_print_field_definitions( RBinJavaObj *obj );
static int r_cmd_java_print_method_definitions( RBinJavaObj *obj );
static int r_cmd_java_print_import_definitions( RBinJavaObj *obj );

static int r_cmd_java_resolve_cp_idx (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_resolve_cp_type (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_resolve_cp_idx_b64 (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_resolve_cp_address (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_resolve_cp_to_key (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_resolve_cp_summary (RBinJavaObj *obj, ut16 idx);

static int r_cmd_java_print_class_access_flags_value( const char * flags );
static int r_cmd_java_print_field_access_flags_value( const char * flags );
static int r_cmd_java_print_method_access_flags_value( const char * flags );
static int r_cmd_java_get_all_access_flags_value (const char *cmd);

static int r_cmd_java_set_acc_flags (RCore *core, ut64 addr, ut16 num_acc_flag);


static int r_cmd_java_print_field_summary (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_print_field_count (RBinJavaObj *obj);
static int r_cmd_java_print_field_name (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_print_field_num_name (RBinJavaObj *obj);
static int r_cmd_java_print_method_summary (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_print_method_count (RBinJavaObj *obj);
static int r_cmd_java_print_method_name (RBinJavaObj *obj, ut16 idx);
static int r_cmd_java_print_method_num_name (RBinJavaObj *obj);

static RBinJavaObj * r_cmd_java_get_bin_obj(RAnal *anal);
static RList * r_cmd_java_get_bin_obj_list(RAnal *anal);
static ut64 r_cmd_java_get_input_num_value(RCore *core, const char *input_value);
static int r_cmd_java_is_valid_input_num_value(RCore *core, const char *input_value);


static int r_cmd_java_call(void *user, const char *input);
static int r_cmd_java_handle_help (RCore * core, const char * input);
static int r_cmd_java_handle_set_flags (RCore * core, const char * cmd);
static int r_cmd_java_handle_prototypes (RCore * core, const char * cmd);
static int r_cmd_java_handle_resolve_cp (RCore * core, const char * cmd);
static int r_cmd_java_handle_calc_flags (RCore * core, const char * cmd);
static int r_cmd_java_handle_flags_str (RCore *core, const char *cmd);
static int r_cmd_java_handle_flags_str_at (RCore *core, const char *cmd);
static int r_cmd_java_handle_field_info (RCore *core, const char *cmd);
static int r_cmd_java_handle_method_info (RCore *core, const char *cmd);

static int r_cmd_java_handle_find_cp_const (RCore *core, const char *cmd);

static RList * r_cmd_java_handle_find_cp_value_float (RCore *core, RBinJavaObj *obj, const char *cmd);
static RList * r_cmd_java_handle_find_cp_value_double (RCore *core, RBinJavaObj *obj, const char *cmd);
static RList * r_cmd_java_handle_find_cp_value_long (RCore *core, RBinJavaObj *obj, const char *cmd);
static RList * r_cmd_java_handle_find_cp_value_int (RCore *core, RBinJavaObj *obj, const char *cmd);
static RList * r_cmd_java_handle_find_cp_value_str (RCore *core, RBinJavaObj *obj, const char *cmd);

static int r_cmd_java_handle_find_cp_value (RCore *core, const char *cmd);

static int r_cmd_java_get_cp_bytes_and_write (RCore *core, RBinJavaObj *obj, ut16 idx, ut64 addr, const ut8* buf, const ut64 len);
static int r_cmd_java_handle_replace_cp_value_float (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr);
static int r_cmd_java_handle_replace_cp_value_double (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr);
static int r_cmd_java_handle_replace_cp_value_long (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr);
static int r_cmd_java_handle_replace_cp_value_int (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr);
static int r_cmd_java_handle_replace_cp_value_str (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr);
static int r_cmd_java_handle_replace_cp_value (RCore *core, const char *cmd);

static int r_cmd_java_handle_replace_classname_value (RCore *core, const char *cmd);
static char * r_cmd_replace_name_def (const char *s_new, ut32 replace_len, const char *s_old, ut32 match_len, const char *buffer, ut32 buf_len, ut32 *res_len);
static char * r_cmd_replace_name (const char *s_new, ut32 replace_len, const char *s_old, ut32 match_len, const char *buffer, ut32 buf_len, ut32 *res_len);
static int r_cmd_is_object_descriptor (const char *name, ut32 name_len);
static ut32 r_cmd_get_num_classname_str_occ (const char * str, const char *match_me);
static const char * r_cmd_get_next_classname_str (const char * str, const char *match_me);

static int r_cmd_java_handle_summary_info (RCore *core, const char *cmd);
static int r_cmd_java_handle_reload_bin (RCore *core, const char *cmd);
static int r_cmd_java_handle_list_code_references (RCore *core, const char *cmd);
static char * r_cmd_java_get_descriptor (RCore *core, RBinJavaObj *bin, ut16 idx);

static int r_cmd_java_handle_print_exceptions (RCore *core, const char *input);

typedef struct r_cmd_java_cms_t {
	const char *name;
	const char *args;
	const char *desc;
	const ut32 name_len;
	RCMDJavaCmdHandler handler;
} RCmdJavaCmd;

#define SET_ACC_FLAGS "set_flags"
#define SET_ACC_FLAGS_ARGS "[<addr> <c | m | f> <num_flag_val>] | [<addr> < c | m | f> <flag value separated by space> ]"
#define SET_ACC_FLAGS_DESC "set the access flags attributes for a field or method"
#define SET_ACC_FLAGS_LEN 9

#define PROTOTYPES "prototypes"
#define PROTOTYPES_ARGS "< a | i | c | m | f>"
#define PROTOTYPES_DESC "print prototypes for all bins, imports only, class, methods, and fields, methods only, or fields only"
#define PROTOTYPES_LEN 10

#define RESOLVE_CP "resolve_cp"
#define RESOLVE_CP_ARGS "< <s | t | e | c | a | d | g > idx>"
#define RESOLVE_CP_DESC "resolve and print cp type or value @ idx. d = dump all,  g = summarize all, s = summary, a = address, t = type, c = get value, e = base64 enode the result"
#define RESOLVE_CP_LEN 10

#define CALC_FLAGS "calc_flags"
#define CALC_FLAGS_ARGS "[ <l <[c|f|m]>> | <c [public,private,static...]>  | <f [public,private,static...]> | <m c [public,private,static...]>]"
#define CALC_FLAGS_DESC "output a value for the given access flags: l = list all flags, c = class, f = field, m = method"
#define CALC_FLAGS_LEN 10

#define FLAGS_STR_AT "flags_str_at"
#define FLAGS_STR_AT_ARGS "[<c | f | m> <addr>]"
#define FLAGS_STR_AT_DESC "output a string value for the given access flags @ addr: c = class, f = field, m = method"
#define FLAGS_STR_AT_LEN 11

#define FLAGS_STR "flags_str"
#define FLAGS_STR_ARGS "[<c | f | m> <acc_flags_value>]"
#define FLAGS_STR_DESC "output a string value for the given access flags number: c = class, f = field, m = method"
#define FLAGS_STR_LEN 9

#define METHOD_INFO "m_info"
#define METHOD_INFO_ARGS "[<[ p | c | <s idx> | <n idx>>]"
#define METHOD_INFO_DESC "output method information at index : c = dump methods and ord , s = dump of all meta-data, n = method"
#define METHOD_INFO_LEN 6

#define FIELD_INFO "f_info"
#define FIELD_INFO_ARGS "[<[p |c | <s idx> | <n idx>>]"
#define FIELD_INFO_DESC "output method information at index : c = dump field and ord , s = dump of all meta-data, n = method"
#define FIELD_INFO_LEN 6

#define HELP "help"
#define HELP_DESC "displays this message"
#define HELP_ARGS "NONE"
#define HELP_LEN 4

#define FIND_CP_CONST "find_cp_const"
#define FIND_CP_CONST_ARGS "[ <a> | <idx>]"
#define FIND_CP_CONST_DESC "find references to constant CP Object in code: a = all references, idx = specific reference"
#define FIND_CP_CONST_LEN 13

#define FIND_CP_VALUE "find_cp_value"
#define FIND_CP_VALUE_ARGS "[ <s | i | l | f | d > <value> ]"
#define FIND_CP_VALUE_DESC "find references to CP constants by value"
#define FIND_CP_VALUE_LEN 13

#define REPLACE_CP_VALUE "replace_cp_value"
#define REPLACE_CP_VALUE_ARGS "[ <idx> <value> ]"
#define REPLACE_CP_VALUE_DESC "replace CP constants with value if the no resizing is required"
#define REPLACE_CP_VALUE_LEN 16

#define REPLACE_CLASS_NAME "replace_classname_value"
#define REPLACE_CLASS_NAME_ARGS "<class_name> <new_class_name>"
#define REPLACE_CLASS_NAME_DESC "replace CP constants with value if the no resizing is required"
#define REPLACE_CLASS_NAME_LEN 23

#define RELOAD_BIN "reload_bin"
#define RELOAD_BIN_ARGS " addr [size]"
#define RELOAD_BIN_DESC "reload and reanalyze the Java class file starting at address"
#define RELOAD_BIN_LEN 10

#define SUMMARY_INFO "summary"
#define SUMMARY_INFO_ARGS "NONE"
#define SUMMARY_INFO_DESC "print summary information for the current java class file"
#define SUMMARY_INFO_LEN 7

#define LIST_CODE_REFS "lcr"
#define LIST_CODE_REFS_ARGS "NONE | <addr>"
#define LIST_CODE_REFS_DESC "list all references to fields and methods in code sections"
#define LIST_CODE_REFS_LEN 3

#define PRINT_EXC "exc"
#define PRINT_EXC_ARGS "NONE | <addr>"
#define PRINT_EXC_DESC "list all exceptions to fields and methods in code sections"
#define PRINT_EXC_LEN 3



static RCmdJavaCmd JAVA_CMDS[] = {
	{HELP, HELP_ARGS, HELP_DESC, HELP_LEN, r_cmd_java_handle_help},
	{SET_ACC_FLAGS, SET_ACC_FLAGS_ARGS, SET_ACC_FLAGS_DESC, SET_ACC_FLAGS_LEN, r_cmd_java_handle_set_flags},
	{PROTOTYPES, PROTOTYPES_ARGS, PROTOTYPES_DESC, PROTOTYPES_LEN, r_cmd_java_handle_prototypes},
	{RESOLVE_CP, RESOLVE_CP_ARGS, RESOLVE_CP_DESC, RESOLVE_CP_LEN, r_cmd_java_handle_resolve_cp},
	{CALC_FLAGS, CALC_FLAGS_ARGS, CALC_FLAGS_DESC, CALC_FLAGS_LEN, r_cmd_java_handle_calc_flags},
	{FLAGS_STR_AT, FLAGS_STR_AT_ARGS, FLAGS_STR_AT_DESC, FLAGS_STR_AT_LEN, r_cmd_java_handle_flags_str_at},
	{FLAGS_STR, FLAGS_STR_ARGS, FLAGS_STR_DESC, FLAGS_STR_LEN, r_cmd_java_handle_flags_str},
	{METHOD_INFO, METHOD_INFO_ARGS, METHOD_INFO_DESC, METHOD_INFO_LEN, r_cmd_java_handle_method_info},
	{FIELD_INFO, FIELD_INFO_ARGS, FIELD_INFO_DESC, FIELD_INFO_LEN, r_cmd_java_handle_field_info},
	{FIND_CP_CONST, FIND_CP_CONST_ARGS, FIND_CP_CONST_DESC, FIND_CP_CONST_LEN, r_cmd_java_handle_find_cp_const},
	{FIND_CP_VALUE, FIND_CP_VALUE_ARGS, FIND_CP_VALUE_DESC, FIND_CP_VALUE_LEN, r_cmd_java_handle_find_cp_value},
	{REPLACE_CP_VALUE, REPLACE_CP_VALUE_ARGS, REPLACE_CP_VALUE_DESC, REPLACE_CP_VALUE_LEN, r_cmd_java_handle_replace_cp_value},
	{REPLACE_CLASS_NAME, REPLACE_CLASS_NAME_ARGS, REPLACE_CLASS_NAME_DESC, REPLACE_CLASS_NAME_LEN, r_cmd_java_handle_replace_classname_value},
	{RELOAD_BIN, RELOAD_BIN_ARGS, RELOAD_BIN_DESC, RELOAD_BIN_LEN, r_cmd_java_handle_reload_bin},
	{SUMMARY_INFO, SUMMARY_INFO_ARGS, SUMMARY_INFO_DESC, REPLACE_CLASS_NAME_LEN, r_cmd_java_handle_summary_info},
	{LIST_CODE_REFS, LIST_CODE_REFS_ARGS, LIST_CODE_REFS_DESC, LIST_CODE_REFS_LEN, r_cmd_java_handle_list_code_references},
	{PRINT_EXC, PRINT_EXC_ARGS, PRINT_EXC_DESC, PRINT_EXC_LEN, r_cmd_java_handle_print_exceptions},
};

enum {
	HELP_IDX = 0,
	SET_ACC_FLAGS_IDX = 1,
	PROTOTYPES_IDX = 2,
	RESOLVE_CP_IDX = 3,
	CALC_FLAGS_IDX = 4,
	FLAGS_STR_AT_IDX = 5,
	FLAGS_STR_IDX = 6,
	METHOD_INFO_IDX = 7,
	FIELD_INFO_IDX = 8,
	FIND_CP_CONST_IDX = 9,
	FIND_CP_VALUE_IDX = 10,
	REPLACE_CP_VALUE_IDX = 11,
	REPLACE_CLASS_NAME_IDX = 12,
	RELOAD_BIN_IDX = 13,
	SUMMARY_INFO_IDX = 14,
	LIST_CODE_REFS_IDX = 15,
	PRINT_EXC_IDX = 16,
	END_CMDS = 17,
};

static ut8 r_cmd_java_obj_ref (const char *name, const char *class_name, ut32 len) {
	if (!name || !class_name) return R_FALSE;
	else if (strncmp (class_name, name, len)) return R_FALSE;
	else if ( *(name-1) == 'L' && *(name+len) == ';') return R_TRUE;
	else if ( !strncmp (class_name, name, len) && *(name+len) == 0) return R_TRUE;
	return R_FALSE;
}

static const char * r_cmd_get_next_classname_str (const char * str, const char *match_me) {
	const char *result = NULL;
	ut32 len = match_me && *match_me ? strlen (match_me) : 0;

	if (len == 0 || !str || !*str ) return NULL;
	result = str;
	while ( result && *result && (result - str < len) ) {
		result = strstr (result, match_me);
		if (result ) break;
	}
	return result;
}

static ut32 r_cmd_get_num_classname_str_occ (const char * str, const char *match_me) {
	const char *result = NULL;
	ut32 len = match_me && *match_me ? strlen (match_me) : 0;
	ut32 occ = 0;

	if (len == 0 || !str || !*str ) return NULL;
	result = str;
	while ( result && *result && (result - str < len)) {
		result = strstr (result, match_me);
		if (result) {
			IFDBG eprintf ("result: %s\n", result);
			result+=len;
			occ++;
		}
	}
	return occ;
}

static const char * r_cmd_java_consumetok (const char *str1, const char b, size_t len) {
	const char *p = str1;
	size_t i = 0;
	if (!p) return p;
	if (len == -1) len = strlen (str1);
	for ( ; i < len; i++,p++) {
		if (*p != b) {
			break;
		}
	}
	return p;
}
static const char * r_cmd_java_strtok (const char *str1, const char b, size_t len) {
	const char *p = str1;
	size_t i = 0;
	if (!p || !*p) return p;
	if (len == -1) len = strlen (str1);
	IFDBG r_cons_printf ("Looking for char (%c) in (%s) up to %d\n", b, p, len);
	for ( ; i < len; i++,p++) {
		if (*p == b) {
			IFDBG r_cons_printf ("Found? for char (%c) @ %d: (%s)\n", b, i, p);
			break;
		}
	}
	if (i == len) p = NULL;
	IFDBG r_cons_printf ("Found? for char (%c) @ %d: (%s)\n", b, len, p);
	return p;
}

static RAnal * get_anal (RCore *core) {
	return core->anal;
}

static void r_cmd_java_print_cmd_help (RCmdJavaCmd *cmd) {
	eprintf ("[*] %s %s\n[+]\t %s\n\n", cmd->name, cmd->args, cmd->desc);
}

static int r_cmd_java_handle_help (RCore * core, const char * input) {
	ut32 i = 0;
	eprintf ("\n%s %s\n", r_core_plugin_java.name, r_core_plugin_java.desc);
	eprintf ("[*] Help Format: Command Arguments\n[+]\t Description\n\n");
	for (i = 0; i <END_CMDS; i++)
		r_cmd_java_print_cmd_help (JAVA_CMDS+i);
	return R_TRUE;
}


static int r_cmd_java_handle_prototypes (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	IFDBG r_cons_printf ("Function call made: %s\n", cmd);

	if (!obj) {
		eprintf ("[-] r_cmd_java: no valid java bins found.\n");
		return R_TRUE;
	}

	switch (*(cmd)) {
		case 'm': return r_cmd_java_print_method_definitions (obj);
		case 'f': return r_cmd_java_print_field_definitions (obj);
		case 'i': return r_cmd_java_print_import_definitions (obj);
		case 'c': return r_cmd_java_print_class_definitions (obj);
		case 'a': return r_cmd_java_print_all_definitions (anal);
	}
	return R_FALSE;
}

static int r_cmd_java_handle_summary_info (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	IFDBG r_cons_printf ("Function call made: %s\n", cmd);

	if (!obj) {
		eprintf ("[-] r_cmd_java: no valid java bins found.\n");
		return R_TRUE;
	}

	r_cons_printf ("Summary for %s:\n", obj->file);
	r_cons_printf ("\tSize 0x%"PFMT64x":\n", obj->size);
	r_cons_printf ("\tConstants (size: 0x%"PFMT64x")Count: %d:\n", obj->cp_size, obj->cp_count);
	r_cons_printf ("\tMethods (size: 0x%"PFMT64x")Count: %d:\n", obj->methods_size, obj->methods_count);
	r_cons_printf ("\tFields (size: 0x%"PFMT64x")Count: %d:\n", obj->fields_size, obj->fields_count);
	r_cons_printf ("\tAttributes (size: 0x%"PFMT64x")Count: %d:\n", obj->attrs_size, obj->attrs_count);
	r_cons_printf ("\tInterfaces (size: 0x%"PFMT64x")Count: %d:\n", obj->interfaces_size, obj->interfaces_count);

	return R_TRUE;
}

static int r_cmd_java_check_op_idx (const ut8 *op_bytes, ut16 idx) {
	return R_BIN_JAVA_USHORT (op_bytes, 0) == idx;
}

static RList * r_cmd_java_handle_find_cp_value_double (RCore *core, RBinJavaObj *obj, const char *cmd) {
	double value = cmd && *cmd ? strtod (cmd, NULL) : 0.0;
	if (value == 0.0 && !(cmd && cmd[0] == '0' && cmd[1] == '.' && cmd[2] == '0') ) return r_list_new();
	return r_bin_java_find_cp_const_by_val ( obj, (const ut8 *) &value, 8, R_BIN_JAVA_CP_DOUBLE);
}
static RList * r_cmd_java_handle_find_cp_value_float (RCore *core, RBinJavaObj *obj, const char *cmd) {
	float value = cmd && *cmd ? atof (cmd) : 0.0;
	if (value == 0.0 && !(cmd && cmd[0] == '0' && cmd[1] == '.' && cmd[2] == '0') ) return r_list_new();
	return r_bin_java_find_cp_const_by_val ( obj, (const ut8 *) &value, 4, R_BIN_JAVA_CP_FLOAT);
}
static RList * r_cmd_java_handle_find_cp_value_long (RCore *core, RBinJavaObj *obj, const char *cmd) {
	ut64 value = r_cmd_java_get_input_num_value (core, cmd);
	if ( !r_cmd_java_is_valid_input_num_value (core, cmd) ) return r_list_new ();
	return r_bin_java_find_cp_const_by_val ( obj, (const ut8 *) &value, 8, R_BIN_JAVA_CP_LONG);
}
static RList * r_cmd_java_handle_find_cp_value_int (RCore *core, RBinJavaObj *obj, const char *cmd) {
	ut32 value = (ut32) r_cmd_java_get_input_num_value (core, cmd);
	if ( !r_cmd_java_is_valid_input_num_value (core, cmd) ) return r_list_new ();
	return r_bin_java_find_cp_const_by_val ( obj, (const ut8 *) &value, 4, R_BIN_JAVA_CP_INTEGER);
}
static RList * r_cmd_java_handle_find_cp_value_str (RCore *core, RBinJavaObj *obj, const char *cmd) {
	if (!cmd) return r_list_new();
	IFDBG r_cons_printf ("Looking for str: %s (%d)\n", cmd, strlen (cmd));
	return r_bin_java_find_cp_const_by_val ( obj, (const ut8 *) cmd, strlen (cmd), R_BIN_JAVA_CP_UTF8);
}

static int r_cmd_java_handle_find_cp_value (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	RList *find_list = NULL;
	RListIter *iter;
	ut32 *idx;
	const char *p = cmd;
	char f_type = 0;
	IFDBG r_cons_printf ("Function call made: %s\n", p);
	if (p && *p) {
		p = r_cmd_java_consumetok (cmd, ' ', -1);
		f_type = *p;
		p+=2;
	}
	IFDBG r_cons_printf ("Function call made: %s\n", p);
	switch (f_type) {
		case 's': find_list = r_cmd_java_handle_find_cp_value_str (core, obj, p); break;
		case 'i': find_list = r_cmd_java_handle_find_cp_value_int (core, obj, r_cmd_java_consumetok (p, ' ', -1)); break;
		case 'l': find_list = r_cmd_java_handle_find_cp_value_long (core, obj, r_cmd_java_consumetok (p, ' ', -1)); break;
		case 'f': find_list = r_cmd_java_handle_find_cp_value_float (core, obj, r_cmd_java_consumetok (p, ' ', -1)); break;
		case 'd': find_list = r_cmd_java_handle_find_cp_value_double (core, obj, r_cmd_java_consumetok (p, ' ', -1)); break;
		default:
			eprintf ("[-] r_cmd_java: invalid java type to search for.\n");
			return R_TRUE;
	}

	r_list_foreach (find_list, iter, idx) {
		ut64 addr = r_bin_java_resolve_cp_idx_address (obj, (ut16) *idx);
		r_cons_printf ("Offset: 0x%"PFMT64x" idx: %d\n", addr, *idx);
	}
	r_list_free (find_list);
	return R_TRUE;
}

static int r_cmd_java_reload_bin_from_buf (RCore *core, RBinJavaObj *obj, ut8* buffer, ut64 len) {
	if (!buffer || len < 10) return R_FALSE;
	int res = r_bin_java_load_bin (obj, buffer, len);

	if (res == R_TRUE) {
		RBinPlugin *cp;
		RListIter *iter;
		r_list_foreach (core->bin->plugins, iter, cp) {
			if (!strncmp ("java", cp->name, 4)) break;
		}
		if (cp) r_bin_update_items (core->bin, cp);
	}
	return res;
}

static int r_cmd_java_get_cp_bytes_and_write (RCore *core, RBinJavaObj *obj, ut16 idx, ut64 addr, const ut8 * buf, const ut64 len) {
	int res = R_FALSE;
	RBinJavaCPTypeObj *cp_obj = r_bin_java_get_item_from_bin_cp_list (obj, idx);
	ut64 c_file_sz = r_io_size (core->io);
	ut32 n_sz = 0, c_sz = obj ? r_bin_java_cp_get_size (obj, idx): -1;

	ut8 * bytes = NULL;

	if (c_sz == -1) return res;

	bytes = r_bin_java_cp_get_bytes (cp_obj->tag, &n_sz, buf, len);

	if (n_sz < c_sz) {
		res = r_core_shift_block (core, addr+c_sz, 0, (int)n_sz - (int)c_sz) &&
		r_io_resize(core->io, c_file_sz + (int) n_sz - (int) c_sz);
	} else if (n_sz > c_sz) {
		res = r_core_extend_at(core, addr,  (int)n_sz - (int)c_sz);
	} else {
		eprintf ("[X] r_cmd_java_get_cp_bytes_and_write: Failed to resize the file correctly aborting.\n");
		return res;
	}

	if (n_sz > 0 && bytes) {
		res = r_core_write_at(core, addr, (const ut8 *)bytes, n_sz) && r_core_seek (core, addr, 1);
	}

	if (res == R_FALSE) {
		eprintf ("[X] r_cmd_java_get_cp_bytes_and_write: Failed to write the bytes to the file correctly aborting.\n");
		return res;
	}

	free (bytes);
	bytes = NULL;

	if (res == R_TRUE) {
		ut64 n_file_sz = 0;
		ut8 * bin_buffer = NULL;
		res = r_io_set_fd (core->io, core->file->fd);
		n_file_sz = r_io_size (core->io);
		bin_buffer = n_file_sz > 0 ? malloc (n_file_sz) : NULL;

		if (bin_buffer) {
			memset (bin_buffer, 0, n_file_sz);
			res = n_file_sz == r_io_read_at (core->io, obj->loadaddr, bin_buffer, n_file_sz) ? R_TRUE : R_FALSE;
			if (res == R_TRUE) res = r_cmd_java_reload_bin_from_buf (core, obj, bin_buffer, n_file_sz);
			else eprintf ("[X] r_cmd_java_get_cp_bytes_and_write: Failed to read the file in aborted, bin reload.\n");
		}
		free (bin_buffer);
	}
	return res;
}


static int r_cmd_java_handle_replace_cp_value_float (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr) {
	float value = cmd && *cmd ? atof (cmd) : 0.0;
	int res = R_FALSE;
	res = r_cmd_java_get_cp_bytes_and_write (core, obj, idx, addr, (ut8 *) &value, 4);
	return res;
}

static int r_cmd_java_handle_replace_cp_value_double (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr) {
	double value = cmd && *cmd ? strtod (cmd, NULL) : 0.0;
	int res = R_FALSE;
	res = r_cmd_java_get_cp_bytes_and_write (core, obj, idx, addr, (ut8 *) &value, 8);
	return res;
}

static int r_cmd_java_handle_replace_cp_value_long (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr) {
	ut64 value = r_cmd_java_get_input_num_value (core, cmd);
	int res = R_FALSE;
	res = r_cmd_java_get_cp_bytes_and_write (core, obj, idx, addr, (ut8 *) &value, 8);
	return res;
}

static int r_cmd_java_handle_replace_cp_value_int (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr) {
	ut32 value = (ut32) r_cmd_java_get_input_num_value (core, cmd);
	int res = R_FALSE;
	res = r_cmd_java_get_cp_bytes_and_write (core, obj, idx, addr, (ut8 *) &value, 4);
	return res;
}

static int r_cmd_java_handle_replace_cp_value_str (RCore *core, RBinJavaObj *obj, const char *cmd, ut16 idx, ut64 addr) {
	int res = R_FALSE;
	ut32 len = cmd && *cmd ? strlen (cmd) : 0;

	if (len > 0 && cmd && *cmd == '"') {
		cmd++;
		len = cmd && *cmd ? strlen (cmd) : 0;
	}
	if (cmd && len > 0)
		res = r_cmd_java_get_cp_bytes_and_write (core, obj, idx, addr, (ut8 *) cmd, len);
	return res;
}

static int r_cmd_java_handle_replace_cp_value (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	ut16 idx = -1;
	ut64 addr = 0;
	const char *p = cmd;
	char cp_type = 0;
	IFDBG r_cons_printf ("Function call made: %s\n", p);
	if (p && *p) {
		p = r_cmd_java_consumetok (cmd, ' ', -1);
		if (r_cmd_java_is_valid_input_num_value (core, p)) {
			idx = r_cmd_java_get_input_num_value (core, p);
			p = r_cmd_java_strtok (p, ' ', strlen(p));
		}
	}
	if (idx == (ut16) -1 ) {
		eprintf ("[-] r_cmd_java:  invalid index value.\n");
		return R_TRUE;
	} else if (!obj) {
		eprintf ("[-] r_cmd_java: The current binary is not a Java Bin Object.\n");
		return R_TRUE;
	} else if (!*p) {
		r_cmd_java_print_cmd_help (JAVA_CMDS+REPLACE_CP_VALUE_IDX);
		return R_TRUE;
	}
	cp_type = r_bin_java_resolve_cp_idx_tag(obj, idx);
	addr = r_bin_java_resolve_cp_idx_address (obj, idx);
	IFDBG r_cons_printf ("Function call made: %s\n", p);
	switch (cp_type) {
		case R_BIN_JAVA_CP_UTF8: return r_cmd_java_handle_replace_cp_value_str (core, obj, r_cmd_java_consumetok (p, ' ', -1), idx, addr);
		case R_BIN_JAVA_CP_INTEGER: return r_cmd_java_handle_replace_cp_value_int (core, obj, r_cmd_java_consumetok (p, ' ', -1), idx, addr);
		case R_BIN_JAVA_CP_LONG: return r_cmd_java_handle_replace_cp_value_long (core, obj, r_cmd_java_consumetok (p, ' ', -1), idx, addr);
		case R_BIN_JAVA_CP_FLOAT: return r_cmd_java_handle_replace_cp_value_float (core, obj, r_cmd_java_consumetok (p, ' ', -1), idx, addr);
		case R_BIN_JAVA_CP_DOUBLE: return r_cmd_java_handle_replace_cp_value_double (core, obj, r_cmd_java_consumetok (p, ' ', -1), idx, addr);
		default:
			eprintf ("[-] r_cmd_java: invalid java type to search for.\n");
			return R_TRUE;
	}
}


static char * r_cmd_replace_name_def (const char *s_new, ut32 replace_len, const char *s_old, ut32 match_len, const char *buffer, ut32 buf_len, ut32 *res_len) {
	const char * fmt = "L%s;";
	char *s_new_ref = s_new && replace_len > 0 ? malloc (3 + replace_len) : NULL;
	char *s_old_ref = s_old && match_len > 0 ? malloc (3 + match_len) : NULL;
	char *result = NULL;
	*res_len = 0;
	if (s_new_ref && s_old_ref) {
		snprintf (s_new_ref, replace_len+3, fmt, s_new);
		snprintf (s_old_ref, match_len+3, fmt, s_old);
		result = r_cmd_replace_name (s_new_ref, replace_len+2, s_old_ref, match_len+2, buffer, buf_len, res_len);
	}
	free (s_new_ref);
	free (s_old_ref);
	return result;
}

static int r_cmd_is_object_descriptor (const char *name, ut32 name_len) {

	int found_L = R_FALSE, found_Semi = R_FALSE;
	ut32 idx = 0, L_pos, Semi_pos;
	const char *p_name = name;

	for (idx = 0, L_pos = 0; idx < name_len; idx++,p_name++) {
		if (*p_name == 'L') {
			found_L = R_TRUE;
			L_pos = idx;
			break;
		}
	}

	for (idx = 0, L_pos = 0; idx < name_len; idx++,p_name++) {
		if (*p_name == ';') {
			found_Semi = R_TRUE;
			Semi_pos = idx;
			break;
		}
	}

	return R_TRUE ? found_L == found_Semi && found_L == R_TRUE && L_pos < Semi_pos : R_FALSE;
}

static char * r_cmd_replace_name (const char *s_new, ut32 replace_len, const char *s_old, ut32 match_len, const char *buffer, ut32 buf_len, ut32 *res_len) {
	ut32 num_occurences = 0, i = 0;
	char * result = NULL, *p_result = NULL;

	num_occurences = r_cmd_get_num_classname_str_occ (buffer, s_old);
	*res_len = 0;
	if (num_occurences > 0 && replace_len > 0 && s_old) {
		ut32 consumed = 0;
		char * next = r_cmd_get_next_classname_str (buffer+consumed, s_old);
		IFDBG r_cons_printf ("Replacing \"%s\" with \"%s\" in: %s\n", s_old, s_new, buffer);
		result = malloc (num_occurences*replace_len + buf_len);
		memset (result, 0, num_occurences*replace_len + buf_len);
		p_result = result;
		while (next && consumed < buf_len) {
			// replace up to next
			IFDBG r_cons_printf ("next: \"%s\", len to: %d\n", next, next-buffer );
			for (; buffer + consumed < next  && consumed < buf_len; consumed++, p_result++) {
				*p_result = *(buffer + consumed);
				(*res_len)++;
			}

			for (i=0; i < replace_len; i++,  p_result++){
				*p_result = *(s_new + i);
				(*res_len)++;
			}
			consumed += match_len;
			next = r_cmd_get_next_classname_str (buffer+consumed, s_old);
		}
		IFDBG r_cons_printf ("Found last occurrence of: \"%s\", remaining: %s\n", s_old, buffer+consumed);
		IFDBG r_cons_printf ("result is: \"%s\"\n", result);
		for (; consumed < buf_len; consumed++, p_result++, (*res_len)++)
			*p_result = *(buffer+consumed);
		IFDBG r_cons_printf ("Old: %s\nNew: %s\n", buffer, result);
	}
	return result;
}


static int r_cmd_java_get_class_names_from_input (const char *input, char **class_name, ut32 *class_name_len, char **new_class_name, ut32 *new_class_name_len) {
	const char *p = input;

	ut32 cmd_sz = input && *input ? strlen (input) : 0;
	int res = R_FALSE;

	if (!class_name || *class_name) return res;
	else if (!new_class_name || *new_class_name) return res;
	else if (!new_class_name_len || !class_name_len) return res;

	*new_class_name = *class_name_len = 0;

	if (p && *p && cmd_sz > 1) {
		const char *end = p;
		p = r_cmd_java_consumetok (p, ' ', cmd_sz);
		end = p && *p ? r_cmd_java_strtok (p, ' ', -1) : NULL;

		if (p && end && p != end) {
			*class_name_len = end - p + 1;
			*class_name = malloc (*class_name_len);
			snprintf (*class_name, *class_name_len, "%s", p );
			cmd_sz = *class_name_len - 1 < cmd_sz ? cmd_sz - *class_name_len : 0;
		}

		if (*class_name && cmd_sz > 0) {
			p = r_cmd_java_consumetok (end+1, ' ', cmd_sz);
			end = p && *p ? r_cmd_java_strtok (p, ' ', -1) : NULL;

			if (!end && p && *p) end = p + cmd_sz;

			if (p && end && p != end) {
				*new_class_name_len = end - p + 1;
				*new_class_name = malloc (*new_class_name_len);
				snprintf (*new_class_name, *new_class_name_len, "%s", p );
				res = R_TRUE;
			}

		}
	}
	return res;

}


static int r_cmd_java_handle_replace_classname_value (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	int res = R_FALSE;
	ut16 idx = -1;
	ut64 addr = 0;
	ut32 cmd_sz = cmd && *cmd ? strlen (cmd) : 0;
	const char *p = cmd;

	char *class_name = NULL, *new_class_name = NULL;
	ut32 class_name_len = 0, new_class_name_len = 0;
	char cp_type = 0;
	IFDBG r_cons_printf ("Function call made: %s\n", p);

	res = r_cmd_java_get_class_names_from_input (cmd, &class_name, &class_name_len, &new_class_name, &new_class_name_len);

	if ( !class_name || !new_class_name ) {
		r_cmd_java_print_cmd_help (JAVA_CMDS+REPLACE_CLASS_NAME_IDX);
		free (class_name);
		free (new_class_name);
		return R_TRUE;
	} else if (!obj) {
		eprintf ("The current binary is not a Java Bin Object.\n");
		free (class_name);
		free (new_class_name);
		return R_TRUE;
	}
	for (idx = 1; idx <=obj->cp_count; idx++) {
		RBinJavaCPTypeObj* cp_obj = r_bin_java_get_item_from_bin_cp_list (obj, idx);
		char *name = NULL;
		ut8 * buffer = NULL;
		ut32 buffer_sz = 0;
		ut16 len = 0;
		if (cp_obj && cp_obj->tag == R_BIN_JAVA_CP_UTF8 &&
			cp_obj->info.cp_utf8.length && cp_obj->info.cp_utf8.length >= class_name_len-1) {
			ut32 num_occurences = 0;
			ut64 addr = cp_obj->file_offset + cp_obj->loadaddr;
			buffer = r_bin_java_cp_get_idx_bytes (obj, idx, &buffer_sz);

			if (!buffer) continue;
			len = R_BIN_JAVA_USHORT ( buffer, 1);
			name = malloc (len+3);
			memcpy (name, buffer+3, len);
			name[len] = 0;

			num_occurences = r_cmd_get_num_classname_str_occ (name, class_name);

			if (num_occurences > 0) {
				// perform inplace replacement
				ut32 res_len = 0;
				char * result = NULL;

				if ( r_cmd_is_object_descriptor (name, len) == R_TRUE) {
					result = r_cmd_replace_name_def (new_class_name, new_class_name_len-1, class_name, class_name_len-1, name, len, &res_len);
				} else {
					result = r_cmd_replace_name (new_class_name, new_class_name_len-1, class_name, class_name_len-1, name, len, &res_len);
				}
				if (result) {
					res = r_cmd_java_get_cp_bytes_and_write (core, obj, idx, addr, result, res_len);
					if  (res == R_FALSE) {
						eprintf ("r_cmd_java: Failed to write bytes or reload the binary.  Aborting before the binary is ruined.\n");
					}
				}
				free (result);
			}

			free (buffer);
			free (name);
		}

	}
	free (class_name);
	free (new_class_name);
	return R_TRUE;
}

static int r_cmd_java_handle_reload_bin (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	char *p = cmd;
	ut64 cur_offset = core->offset, addr = 0;
	ut64 buf_size = 0;
	ut8 * buf = NULL;
	int res = R_FALSE;

	if (*cmd == ' ') p = r_cmd_java_consumetok (p, ' ', -1);

	if (!*cmd) {
		r_cmd_java_print_cmd_help (JAVA_CMDS+RELOAD_BIN_IDX);
		return R_TRUE;
	}


	addr = r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;
	if (*cmd == ' ') p = r_cmd_java_consumetok (p, ' ', -1);
	buf_size = r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;

	// XXX this may cause problems cause the file we are looking at may not be the bin we want.
	// lets pretend it is for now
	if (buf_size == 0) {
		res = r_io_set_fd (core->io, core->file->fd);
		buf_size = r_io_size (core->io);
		buf = malloc (buf_size);
		memset (buf, 0, buf_size);
		r_io_read_at (core->io, addr, buf, buf_size);
	}
	if (buf && obj) {
		res = r_cmd_java_reload_bin_from_buf (core, obj, buf, buf_size);
	}
	free (buf);
	return res;
}

static int r_cmd_java_handle_find_cp_const (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	RAnalFunction *fcn = NULL;
	RAnalBlock *bb = NULL;
	RListIter *bb_iter, *fn_iter, *iter;
	RCmdJavaCPResult *cp_res = NULL;
	ut16 idx = -1;
	RList *find_list;
	const char *p;

	if (cmd && *cmd == ' ') p = r_cmd_java_consumetok (cmd, ' ', -1);

	if (p && *p == 'a') idx = -1;
	else idx = r_cmd_java_get_input_num_value (core, p);

	IFDBG r_cons_printf ("Function call made: %s\n", cmd);

	if (!obj) {
		eprintf ("[-] r_cmd_java: no valid java bins found.\n");
		return R_TRUE;
	} else if (!cmd || !*cmd) {
		eprintf ("[-] r_cmd_java: invalid command syntax.\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+FIND_CP_CONST_IDX);
		return R_TRUE;
	} else if (idx == 0) {
		eprintf ("[-] r_cmd_java: invalid CP Obj Index Supplied.\n");
		return R_TRUE;
	// not idx is set in previous operation
	}
	find_list = r_list_new ();
	find_list->free = free;
	// XXX - this will break once RAnal moves to sdb
	r_list_foreach (anal->fcns, fn_iter, fcn) {
		r_list_foreach (fcn->bbs, bb_iter, bb) {
			char op = bb->op_bytes[0];
			cp_res = NULL;
			switch (op) {
				case 0x12:
					cp_res = (idx == (ut16) -1) || (bb->op_bytes[1] == idx) ?
								R_NEW0(RCmdJavaCPResult) : NULL;
					if (cp_res) cp_res->idx = bb->op_bytes[1];
					break;
				case 0x13:
				case 0x14:
					cp_res = (idx == (ut16) -1) || (R_BIN_JAVA_USHORT (bb->op_bytes, 1) == idx) ?
								R_NEW0(RCmdJavaCPResult) : NULL;
					if (cp_res) cp_res->idx = R_BIN_JAVA_USHORT (bb->op_bytes, 1);
					break;
			}

			if (cp_res) {
				cp_res->addr = bb->addr;
				cp_res->obj = r_bin_java_get_item_from_cp (obj, cp_res->idx);
				r_list_append (find_list, cp_res);
			}
		}
	}
	if (idx == (ut16) -1) {
		r_list_foreach (find_list, iter, cp_res) {
			const char *t = ((RBinJavaCPTypeMetas *) cp_res->obj->metas->type_info)->name;
			r_cons_printf ("@0x%"PFMT64x" idx = %d Type = %s\n", cp_res->addr, cp_res->idx, t);
		}

	} else {
		r_list_foreach (find_list, iter, cp_res) {
			r_cons_printf ("@0x%"PFMT64x"\n", cp_res->addr);
		}
	}
	r_list_free (find_list);
	return R_TRUE;
}

static int r_cmd_java_handle_field_info (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	IFDBG r_cons_printf ("Function call made: %s\n", cmd);
	ut16 idx = -1;

	if (!obj) {
		eprintf ("[-] r_cmd_java: no valid java bins found.\n");
		return R_TRUE;
	} else if (!cmd || !*cmd) {
		eprintf ("[-] r_cmd_java: invalid command syntax.\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+FIELD_INFO_IDX);
	}

	if (*(cmd) == 's' || *(cmd) == 'n') {
		idx = r_cmd_java_get_input_num_value (core, cmd+2);
	}

	switch (*(cmd)) {
		case 'c': return r_cmd_java_print_field_num_name (obj);
		case 's': return r_cmd_java_print_field_summary (obj, idx);
		case 'n': return r_cmd_java_print_field_name (obj, idx);
	}
	IFDBG r_cons_printf ("Command is (%s)\n", cmd);
	eprintf ("[-] r_cmd_java: invalid command syntax.\n");
	r_cmd_java_print_cmd_help (JAVA_CMDS+FIELD_INFO_IDX);
	return R_FALSE;
}

static int r_cmd_java_handle_method_info (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *obj = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	IFDBG r_cons_printf ("Command is (%s)\n", cmd);
	ut16 idx = -1;

	if (!obj) {
		eprintf ("[-] r_cmd_java: no valid java bins found.\n");
		return R_TRUE;
	} else if (!cmd || !*cmd) {
		eprintf ("[-] r_cmd_java: invalid command syntax.\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+METHOD_INFO_IDX);
	}

	if (*(cmd) == 's' || *(cmd) == 'n') {
		idx = r_cmd_java_get_input_num_value (core, cmd+2);
	}

	switch (*(cmd)) {
		case 'c': return r_cmd_java_print_method_num_name (obj);
		case 's': return r_cmd_java_print_method_summary (obj, idx);
		case 'n': return r_cmd_java_print_method_name (obj, idx);
	}

	IFDBG r_cons_printf ("Command is (%s)\n", cmd);
	eprintf ("[-] r_cmd_java: invalid command syntax.\n");
	r_cmd_java_print_cmd_help (JAVA_CMDS+METHOD_INFO_IDX);
	return R_FALSE;
}


static int r_cmd_java_handle_resolve_cp (RCore *core, const char *cmd) {
	RAnal *anal = get_anal (core);
	char c_type = cmd && *cmd ? *cmd : 0;
	RBinJavaObj *obj = r_cmd_java_get_bin_obj (anal);
	ut16 idx = r_cmd_java_get_input_num_value (core, cmd+2);
	IFDBG r_cons_printf ("Function call made: %s\n", cmd);
	IFDBG r_cons_printf ("Ctype: %d (%c) RBinJavaObj points to: %p and the idx is (%s): %d\n", c_type, c_type, obj, cmd+2, idx);
	int res = R_FALSE;
	if (idx > 0 && obj) {
		switch (c_type) {
			case 't': return r_cmd_java_resolve_cp_type (obj, idx);
			case 'c': return r_cmd_java_resolve_cp_idx (obj, idx);
			case 'e': return r_cmd_java_resolve_cp_idx_b64 (obj, idx);
			case 'a': return r_cmd_java_resolve_cp_address (obj, idx);
			case 's': return r_cmd_java_resolve_cp_summary (obj, idx);
			case 'k': return r_cmd_java_resolve_cp_to_key (obj, idx);
		}
	} else if (obj && c_type == 'g') {
		for (idx = 1; idx <=obj->cp_count; idx++) {
			ut64 addr = r_bin_java_resolve_cp_idx_address (obj, idx) ;
			char * str = r_bin_java_resolve_cp_idx_type (obj, idx);
			r_cons_printf ("CP_OBJ Type %d =  %s @ 0x%"PFMT64x"\n", idx, str, addr);
			free (str);
		}
		res = R_TRUE;
	} else if (obj && c_type == 'd') {
		for (idx = 1; idx <=obj->cp_count; idx++) {
			r_cmd_java_resolve_cp_summary (obj, idx);
		}
		res = R_TRUE;
	} else {
		if (!obj) {
			eprintf ("[-] r_cmd_java: no valid java bins found.\n");
		} else {
			eprintf ("[-] r_cmd_java: invalid cp index given, must idx > 1.\n");
			r_cmd_java_print_cmd_help (JAVA_CMDS+RESOLVE_CP_IDX);
		}
		res = R_TRUE;
	}
	return res;
}

static int r_cmd_java_get_all_access_flags_value (const char *cmd) {
	RList *the_list = NULL;
	RListIter *iter = NULL;
	char *str = NULL;

	switch (*(cmd)) {
		case 'f': the_list = retrieve_all_field_access_string_and_value (); break;
		case 'm': the_list = retrieve_all_method_access_string_and_value (); break;
		case 'c': the_list = retrieve_all_class_access_string_and_value (); break;
	}
	if (!the_list) {
		eprintf ("[-] r_cmd_java: incorrect syntax for the flags calculation.\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+CALC_FLAGS_IDX);
		return R_FALSE;
	}
	switch (*(cmd)) {
		case 'f': r_cons_printf ("[=] Fields Access Flags List\n"); break;
		case 'm': r_cons_printf ("[=] Methods Access Flags List\n"); break;
		case 'c': r_cons_printf ("[=] Class Access Flags List\n");; break;
	}

	r_list_foreach (the_list, iter, str) {
		r_cons_printf ("%s\n", str);
	}
	r_list_free (the_list);
	return R_TRUE;
}

static int r_cmd_java_handle_calc_flags (RCore *core, const char *cmd) {
	IFDBG r_cons_printf ("Function call made: %s\n", cmd);
	int res = R_FALSE;

	switch (*(cmd)) {
		case 'f': return r_cmd_java_print_field_access_flags_value (cmd+2);
		case 'm': return r_cmd_java_print_method_access_flags_value (cmd+2);
		case 'c': return r_cmd_java_print_class_access_flags_value (cmd+2);
	}

	if ( *(cmd) == 'l') {
		const char *lcmd = *cmd+1 == ' '? cmd+2 : cmd+1;
		IFDBG eprintf ("Seeing %s and accepting %s\n", cmd, lcmd);
		switch (*(lcmd)) {
			case 'f':
			case 'm':
			case 'c': res = r_cmd_java_get_all_access_flags_value (lcmd); break;
		}
		// Just print them all out
		if (res == R_FALSE) {
			r_cmd_java_get_all_access_flags_value ("c");
			r_cmd_java_get_all_access_flags_value ("m");
			res = r_cmd_java_get_all_access_flags_value ("f");
		}
	}
	if (res == R_FALSE) {
		eprintf ("[-] r_cmd_java: incorrect syntax for the flags calculation.\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+CALC_FLAGS_IDX);
		res = R_TRUE;
	}
	return res;
}

static int r_cmd_java_handle_flags_str (RCore *core, const char *cmd) {

	int res = R_FALSE;
	ut32 flag_value = -1;
	const char f_type = cmd ? *cmd : 0;
	const char *p = cmd ? cmd + 2: NULL;
	char * flags_str = NULL;

	IFDBG r_cons_printf ("r_cmd_java_handle_flags_str: ftype = %c, idx = %s\n", f_type, p);
	if (p)
		flag_value = r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;

	if (p && f_type) {
		switch (f_type) {
			case 'm': flags_str = retrieve_method_access_string((ut16) flag_value); break;
			case 'f': flags_str = retrieve_field_access_string((ut16) flag_value); break;
			case 'c': flags_str = retrieve_class_method_access_string((ut16) flag_value); break;
			default: flags_str = NULL;
		}
	}

	if (flags_str) {
		switch (f_type) {
			case 'm': r_cons_printf ("Method Access Flags String: "); break;
			case 'f': r_cons_printf ("Field Access Flags String: "); break;
			case 'c': r_cons_printf ("Class Access Flags String: "); break;
		}
		r_cons_printf ("%s\n", flags_str);
		free (flags_str);
		res = R_TRUE;
	}
	if (res == R_FALSE) {
		eprintf ("[-] r_cmd_java: incorrect syntax for the flags calculation.\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+FLAGS_STR_IDX);
		res = R_TRUE;
	}
	return res;
}

static int r_cmd_java_handle_flags_str_at (RCore *core, const char *cmd) {

	int res = R_FALSE;
	ut64 flag_value_addr = -1;
	ut32 flag_value = -1;
	const char f_type = cmd ? *r_cmd_java_consumetok (cmd, ' ', -1) : 0;
	const char *p = cmd ? cmd + 2: NULL;
	char * flags_str = NULL;

	IFDBG r_cons_printf ("r_cmd_java_handle_flags_str_at: ftype = 0x%02x, idx = %s\n", f_type, p);
	if (p) {
		flag_value = 0;
		ut64 cur_offset = core->offset;
		flag_value_addr = r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;
		r_core_read_at (core, flag_value_addr, (ut8 *) &flag_value, 2);
		IFDBG r_cons_printf ("r_cmd_java_handle_flags_str_at: read = 0x%04x\n", flag_value);
		if (cur_offset != core->offset) r_core_seek (core, cur_offset-2, 1);
		flag_value = R_BIN_JAVA_USHORT (((ut8 *) &flag_value), 0);
	}

	if (p && f_type) {
		switch (f_type) {
			case 'm': flags_str = retrieve_method_access_string((ut16) flag_value); break;
			case 'f': flags_str = retrieve_field_access_string((ut16) flag_value); break;
			case 'c': flags_str = retrieve_class_method_access_string((ut16) flag_value); break;
			default: flags_str = NULL;
		}
	}

	if (flags_str) {
		switch (f_type) {
			case 'm': r_cons_printf ("Method Access Flags String: "); break;
			case 'f': r_cons_printf ("Field Access Flags String: "); break;
			case 'c': r_cons_printf ("Class Access Flags String: "); break;
		}
		r_cons_printf ("%s\n", flags_str);
		free (flags_str);
		res = R_TRUE;
	}
	if (res == R_FALSE) {
		eprintf ("[-] r_cmd_java: incorrect syntax for the flags calculation.\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+FLAGS_STR_IDX);
		res = R_TRUE;
	}
	return res;
}


static char r_cmd_java_is_valid_java_mcf (char b) {
	char c = 0;
	switch (b) {
		case 'c':
		case 'f':
		case 'm': c = b;
	}
	return c;
}

static int r_cmd_java_handle_set_flags (RCore * core, const char * input) {
	//#define SET_ACC_FLAGS_ARGS "< c | m | f> <addr> <d | <s <flag value separated by space> >"
	const char *p = r_cmd_java_consumetok (input, ' ', -1);

	ut64 addr = p && r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;
	ut32 flag_value = -1;
	char f_type = '?';

	p = r_cmd_java_strtok (p+1, ' ', -1);
	f_type = r_cmd_java_is_valid_java_mcf (*(++p));

	flag_value = r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;

	if (flag_value == 16 && f_type == 'f') {
		flag_value = -1;
	}
	IFDBG r_cons_printf ("Converting %s to flags\n",p);

	if (p) p+=2;
	if (flag_value == -1)
		flag_value = r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;
	int res = R_FALSE;
	if (!input) {
		eprintf ("[-] r_cmd_java: no address provided .\n");
		res = R_TRUE;
	} else if (addr == -1) {
		eprintf ("[-] r_cmd_java: no address provided .\n");
		res = R_TRUE;
	} else if (!f_type && flag_value == -1) {
		eprintf ("[-] r_cmd_java: no flag type provided .\n");
		res = R_TRUE;
	}

	if (res) {
		r_cmd_java_print_cmd_help (JAVA_CMDS+SET_ACC_FLAGS_IDX);
		return res;
	}

	IFDBG r_cons_printf ("Writing to %c to 0x%"PFMT64x", %s.\n", f_type, addr, p);

	//  handling string based access flags (otherwise skip ahead)
	IFDBG r_cons_printf ("Converting %s to flags\n",p);
	if (f_type && flag_value != -1) {
		switch (f_type) {
			case 'f': flag_value = r_bin_java_calculate_field_access_value (p); break;
			case 'm': flag_value = r_bin_java_calculate_method_access_value (p); break;
			case 'c': flag_value = r_bin_java_calculate_class_access_value (p); break;
			default: flag_value = -1;
		}
		if (flag_value == -1) {
			eprintf ("[-] r_cmd_java: in valid flag type provided .\n");
			res = R_TRUE;
		}
	}
	IFDBG r_cons_printf ("Current args: (flag_value: 0x%04x addr: 0x%"PFMT64x")\n.", flag_value, addr, res);
	if (flag_value != -1) {
		res = r_cmd_java_set_acc_flags (core, addr, ((ut16) flag_value) & 0xffff);
		IFDBG r_cons_printf ("Writing 0x%04x to 0x%"PFMT64x": %d.", flag_value, addr, res);
	} else {
		eprintf ("[-] r_cmd_java: invalid flag value or type provided .\n");
		r_cmd_java_print_cmd_help (JAVA_CMDS+SET_ACC_FLAGS_IDX);
		res = R_TRUE;
	}
	return res;
}

static int r_cmd_java_call(void *user, const char *input) {
	RCore *core = (RCore *) user;
	int res = R_FALSE;
	ut32 i = 0;
	IFDBG r_cons_printf ("Function call made: %s\n", input);
	if (strncmp (input, "java",4)) return R_FALSE;
	else if (strncmp (input, "java ",5)) {
		return r_cmd_java_handle_help (core, input);
	}

	for (; i <END_CMDS; i++) {
		//IFDBG r_cons_printf ("Checking cmd: %s %d %s\n", JAVA_CMDS[i].name, JAVA_CMDS[i].name_len, p);
		IFDBG r_cons_printf ("Checking cmd: %s %d\n", JAVA_CMDS[i].name, strncmp (input+5, JAVA_CMDS[i].name, JAVA_CMDS[i].name_len));
		if (!strncmp (input+5, JAVA_CMDS[i].name, JAVA_CMDS[i].name_len)) {
			//IFDBG r_cons_printf ("Executing cmd: %s (%s)\n", JAVA_CMDS[i].name, cmd+5+JAVA_CMDS[i].name_len );
			res =  JAVA_CMDS[i].handler (core, input+5+JAVA_CMDS[i].name_len+1);
			break;
		}
	}

	if (res == R_FALSE) res = r_cmd_java_handle_help (core, input);
	return R_TRUE;
}


static int r_cmd_java_print_method_definitions ( RBinJavaObj *obj ) {
	RList * the_list = r_bin_java_get_method_definitions (obj),
			* off_list = r_bin_java_get_method_offsets (obj);
	char * str = NULL;
	ut32 idx = 0, end = r_list_length (the_list);

	while (idx < end) {
		ut64 *addr = r_list_get_n (off_list, idx);
		str = r_list_get_n (the_list, idx);
		r_cons_printf("%s; // @0x%04"PFMT64x"\n", str, *addr);
		idx++;
	}

	r_list_free(the_list);
	r_list_free(off_list);
	return R_TRUE;
}

static int r_cmd_java_print_field_definitions ( RBinJavaObj *obj ) {
	RList * the_list = r_bin_java_get_field_definitions (obj),
			* off_list = r_bin_java_get_field_offsets (obj);
	char * str = NULL;
	ut32 idx = 0, end = r_list_length (the_list);

	while (idx < end) {
		ut64 *addr = r_list_get_n (off_list, idx);
		str = r_list_get_n (the_list, idx);
		r_cons_printf("%s; // @0x%04"PFMT64x"\n", str, *addr);
		idx++;
	}

	r_list_free(the_list);
	r_list_free(off_list);
	return R_TRUE;
}

static int r_cmd_java_print_import_definitions ( RBinJavaObj *obj ) {
	RList * the_list = r_bin_java_get_import_definitions (obj);
	char * str = NULL;
	RListIter *iter;
	r_list_foreach (the_list, iter, str) {
		r_cons_printf("import %s;\n", str);
	}
	r_list_free(the_list);
	return R_TRUE;
}

static int r_cmd_java_print_all_definitions( RAnal *anal ) {
	RList * obj_list  = r_cmd_java_get_bin_obj_list (anal);
	RListIter *iter;
	RBinJavaObj *obj;

	if (!obj_list) return 1;
	r_list_foreach (obj_list, iter, obj) {
		r_cmd_java_print_class_definitions (obj);
	}
	return R_TRUE;
}
static int r_cmd_java_print_class_definitions( RBinJavaObj *obj ) {
	RList * the_fields = r_bin_java_get_field_definitions (obj),
			* the_methods = r_bin_java_get_method_definitions (obj),
			* the_imports = r_bin_java_get_import_definitions (obj),
			* the_moffsets = r_bin_java_get_method_offsets (obj),
			* the_foffsets = r_bin_java_get_field_offsets (obj);

	char * class_name = r_bin_java_get_this_class_name(obj),
		 * str = NULL;

	r_cmd_java_print_import_definitions (obj);
	r_cons_printf ("\nclass %s { // @0x%04"PFMT64x"\n", class_name, obj->loadaddr);

	if (the_fields && the_foffsets && r_list_length (the_fields) > 0) {
		r_cons_printf ("\n\t// Fields defined in the class\n");
		ut32 idx = 0, end = r_list_length (the_fields);

		while (idx < end) {
			ut64 *addr = r_list_get_n (the_foffsets, idx);
			str = r_list_get_n (the_fields, idx);
			r_cons_printf("\t%s; // @0x%04"PFMT64x"\n", str, *addr);
			idx++;
		}
	}

	if (the_methods && the_moffsets && r_list_length (the_methods) > 0) {
		r_cons_printf ("\n\t// Methods defined in the class\n");
		ut32 idx = 0, end = r_list_length (the_methods);

		while (idx < end) {
			ut64 *addr = r_list_get_n (the_moffsets, idx);
			str = r_list_get_n (the_methods, idx);
			r_cons_printf("\t%s; // @0x%04"PFMT64x"\n", str, *addr);
			idx++;
		}
	}
	r_cons_printf ("}\n");

	r_list_free (the_imports);
	r_list_free (the_fields);
	r_list_free (the_methods);
	r_list_free (the_foffsets);
	r_list_free (the_moffsets);

	free(class_name);
	return R_TRUE;
}

static RList * r_cmd_java_get_bin_obj_list(RAnal *anal) {
	RBinJavaObj *bin_obj = (RBinJavaObj * ) r_cmd_java_get_bin_obj(anal);
	// See libr/bin/p/bin_java.c to see what is happening here.  The original intention
	// was to use a shared global db variable from shlr/java/class.c, but the
	// BIN_OBJS_ADDRS variable kept getting corrupted on Mac, so I (deeso) switched the
	// way the access to the db was taking place by using the bin_obj as a proxy back
	// to the BIN_OBJS_ADDRS which is instantiated in libr/bin/p/bin_java.c
	// not the easiest way to make sausage, but its getting made.
	return  r_bin_java_get_bin_obj_list_thru_obj (bin_obj);
}

static RBinJavaObj * r_cmd_java_get_bin_obj(RAnal *anal) {
	RBin *b = anal->binb.bin;
	ut8 is_java = (b && b->cur->curplugin && strcmp (b->cur->curplugin->name, "java") == 0) ? 1 : 0;
	return is_java ? b->cur->o->bin_obj : NULL;
}

static int r_cmd_java_resolve_cp_idx (RBinJavaObj *obj, ut16 idx) {
	if (obj && idx){
		char * str = r_bin_java_resolve_without_space (obj, idx);
		r_cons_printf ("%s\n", str);
		free (str);
	}
	return R_TRUE;
}

static int r_cmd_java_resolve_cp_type (RBinJavaObj *obj, ut16 idx) {
	if (obj && idx){
		char * str = r_bin_java_resolve_cp_idx_type (obj, idx);
		r_cons_printf ("%s\n", str);
		free (str);
	}
	return R_TRUE;
}

static int r_cmd_java_resolve_cp_idx_b64 (RBinJavaObj *obj, ut16 idx) {
	if (obj && idx){
		char * str = r_bin_java_resolve_b64_encode (obj, idx) ;
		r_cons_printf ("%s\n", str);
		free (str);
	}
	return R_TRUE;
}

static int r_cmd_java_resolve_cp_address (RBinJavaObj *obj, ut16 idx) {
	if (obj && idx){
		ut64 addr = r_bin_java_resolve_cp_idx_address (obj, idx) ;
		if (addr == -1)
			r_cons_printf ("Unable to resolve CP Object @ index: 0x%04x\n", idx);
		else
			r_cons_printf ("0x%"PFMT64x"\n", addr);
	}
	return R_TRUE;
}

static int r_cmd_java_resolve_cp_to_key (RBinJavaObj *obj, ut16 idx) {
	if (obj && idx){
		char * str = r_bin_java_resolve_cp_idx_to_string (obj, idx) ;
		r_cons_printf ("%s\n", str);
		free (str);
	}
	return R_TRUE;
}
static int r_cmd_java_resolve_cp_summary (RBinJavaObj *obj, ut16 idx) {
	if (obj && idx){
		r_bin_java_resolve_cp_idx_print_summary (obj, idx) ;
	}
	return R_TRUE;
}

static int r_cmd_java_is_valid_input_num_value(RCore *core, const char *input_value){
	ut64 value = input_value ? r_num_math (core->num, input_value) : 0;
	return !(value == 0 && input_value && *input_value == '0');
}

static ut64 r_cmd_java_get_input_num_value(RCore *core, const char *input_value){
	ut64 value = input_value ? r_num_math (core->num, input_value) : 0;
	return value;
}

static int r_cmd_java_print_class_access_flags_value( const char * flags ){
	ut16 result = r_bin_java_calculate_class_access_value (flags);
	r_cons_printf ("Access Value for %s = 0x%04x\n", flags, result);
	return R_TRUE;
}
static int r_cmd_java_print_field_access_flags_value( const char * flags ){
	ut16 result = r_bin_java_calculate_field_access_value (flags);
	r_cons_printf ("Access Value for %s = 0x%04x\n", flags,  result);
	return R_TRUE;
}
static int r_cmd_java_print_method_access_flags_value( const char * flags ){
	ut16 result = r_bin_java_calculate_method_access_value (flags);
	r_cons_printf ("Access Value for %s = 0x%04x\n", flags,  result);
	return R_TRUE;
}

static int r_cmd_java_set_acc_flags (RCore *core, ut64 addr, ut16 num_acc_flag) {
	char cmd_buf [50];
	//const char * fmt = "wx %04x @ 0x%"PFMT64x;

	int res = R_FALSE;
	//ut64 cur_offset = core->offset;
	num_acc_flag = R_BIN_JAVA_USHORT (((ut8*) &num_acc_flag), 0);
	res = r_core_write_at(core, addr, (const ut8 *)&num_acc_flag, 2);
	//snprintf (cmd_buf, 50, fmt, num_acc_flag, addr);
	//res = r_core_cmd0(core, cmd_buf);
	res = R_TRUE;
	IFDBG r_cons_printf ("Executed cmd: %s == %d\n", cmd_buf, res);
	/*if (cur_offset != core->offset) {
		IFDBG eprintf ("Ooops, write advanced the cursor, moving it back.");
		r_core_seek (core, cur_offset-2, 1);
	}*/
	return res;
}
static int r_cmd_java_print_field_num_name (RBinJavaObj *obj) {
	RList * the_list = r_bin_java_get_field_num_name (obj);
	char * str;
	RListIter *iter = NULL;
	r_list_foreach (the_list, iter, str) {
		r_cons_printf ("%s\n", str);
	}
	return R_TRUE;
}
static int r_cmd_java_print_method_num_name (RBinJavaObj *obj) {
	RList * the_list = r_bin_java_get_method_num_name (obj);
	char * str;
	RListIter *iter = NULL;
	r_list_foreach (the_list, iter, str) {
		r_cons_printf ("%s\n", str);
	}
	return R_TRUE;
}
static int r_cmd_java_print_field_summary (RBinJavaObj *obj, ut16 idx) {
	int res = r_bin_java_print_field_idx_summary (obj, idx);
	if (res == R_FALSE) {
		eprintf ("Error: Field or Method @ index (%d) not found in the RBinJavaObj.\n", idx);
		res = R_TRUE;
	}
	return res;
}
static int r_cmd_java_print_field_count (RBinJavaObj *obj) {
	ut32 res = r_bin_java_get_field_count (obj);
	r_cons_printf ("%d\n", res);
	r_cons_flush();
	return R_TRUE;
}

static int r_cmd_java_print_field_name (RBinJavaObj *obj, ut16 idx) {
	char * res = r_bin_java_get_field_name (obj, idx);
	if (res) {
		r_cons_printf ("%s\n", res);
	} else {
		eprintf ("Error: Field or Method @ index (%d) not found in the RBinJavaObj.\n", idx);
	}
	free (res);
	return R_TRUE;
}

static int r_cmd_java_print_method_summary (RBinJavaObj *obj, ut16 idx) {
	int res = r_bin_java_print_method_idx_summary (obj, idx);
	if (res == R_FALSE) {
		eprintf ("Error: Field or Method @ index (%d) not found in the RBinJavaObj.\n", idx);
		res = R_TRUE;
	}
	return res;
}

static int r_cmd_java_print_method_count (RBinJavaObj *obj) {
	ut32 res = r_bin_java_get_method_count (obj);
	r_cons_printf ("%d\n", res);
	r_cons_flush();
	return R_TRUE;
}

static int r_cmd_java_print_method_name (RBinJavaObj *obj, ut16 idx) {
	char * res = r_bin_java_get_method_name (obj, idx);
	if (res) {
		r_cons_printf ("%s\n", res);
	} else {
		eprintf ("Error: Field or Method @ index (%d) not found in the RBinJavaObj.\n", idx);
	}
	free (res);
	return R_TRUE;
}
static char * r_cmd_java_get_descriptor (RCore *core, RBinJavaObj *bin, ut16 idx) {
	char *class_name = NULL, *name = NULL, *descriptor = NULL, *full_bird = NULL;
	ut32 len = 0;
	RBinJavaCPTypeObj * obj = r_bin_java_get_item_from_bin_cp_list (bin, idx);
	const char *type = NULL;

	if (obj->tag == R_BIN_JAVA_CP_INTERFACEMETHOD_REF ||
		obj->tag == R_BIN_JAVA_CP_METHODREF ||
		obj->tag == R_BIN_JAVA_CP_FIELDREF) {
		class_name = r_bin_java_get_name_from_bin_cp_list (bin, obj->info.cp_method.class_idx);
		name = r_bin_java_get_item_name_from_bin_cp_list (bin, obj);
		descriptor = r_bin_java_get_item_desc_from_bin_cp_list (bin, obj);
	}

	len += (class_name ? strlen (class_name) + 1 : 0);
	len += (name ? strlen (name) + 1 : 0);
	len += (descriptor ? strlen (descriptor) + 1 : 0);
	if (len > 0){
		full_bird = malloc (len + 100);
		memset (full_bird, 0, len+100);
	}

	if (full_bird && (obj->tag == R_BIN_JAVA_CP_INTERFACEMETHOD_REF ||
		obj->tag == R_BIN_JAVA_CP_METHODREF)) {
		if (obj->tag == R_BIN_JAVA_CP_INTERFACEMETHOD_REF) type = "INTERFACE";
		else type = "FUNCTION";

		snprintf (full_bird, len+100, "%s.%s %s", class_name, name, descriptor);

	} else if (full_bird && obj->tag == R_BIN_JAVA_CP_FIELDREF) {
		type = "FIELD";
		snprintf (full_bird, len+100, "%s %s.%s", descriptor, class_name, name);
	}

	free (class_name);
	free (name);
	free (descriptor);
	return full_bird;
}


static int r_cmd_java_handle_list_code_references (RCore *core, const char *input) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *bin = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	RAnalBlock *bb = NULL;
	RAnalFunction *fcn = NULL;
	RListIter *bb_iter = NULL, *fcn_iter = NULL;
	ut64 func_addr = -1;

	const char *p = r_cmd_java_consumetok (input, ' ', -1);
	func_addr = p && *p && r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;

	if (!bin) return R_FALSE;

	const char *fmt = "0x%"PFMT64x" %s type: %s info: %s\n";


	r_list_foreach (anal->fcns, fcn_iter, fcn) {
		ut8 do_this_one = fcn->addr <= func_addr && func_addr <= fcn->addr + fcn->size;
		do_this_one = func_addr == -1 ? 1 : do_this_one;
		if (!do_this_one) continue;
		r_list_foreach (fcn->bbs, bb_iter, bb) {
			const char *operation = NULL, *type = NULL;
			ut64 addr = -1;
			ut16 cp_ref_idx = -1;
			char *full_bird = NULL;
			// if bb_type is a call
			if ( (bb->type2 &  R_ANAL_EX_CODEOP_CALL) == R_ANAL_EX_CODEOP_CALL) {
				ut8 op_byte = bb->op_bytes[0];
				// look at the bytes determine if it belongs to this class
				switch (op_byte) {
					case 0xb6: // invokevirtual
						operation = "call virtual";
						type = "FUNCTION";
						addr = bb->addr;
						break;
					case 0xb7: // invokespecial
						operation = "call special";
						type = "FUNCTION";
						addr = bb->addr;
						break;
					case 0xb8: // invokestatic
						operation = "call static";
						type = "FUNCTION";
						addr = bb->addr;
						break;
					case 0xb9: // invokeinterface
						operation = "call interface";
						type = "FUNCTION";
						addr = bb->addr;
						break;
					case 0xba: // invokedynamic
						operation = "call dynamic";
						type = "FUNCTION";
						addr = bb->addr;
						break;
					default:
						operation = NULL;
						addr = -1;
						break;
				}

			} else if ( (bb->type2 & R_ANAL_EX_LDST_LOAD_GET_STATIC) == R_ANAL_EX_LDST_LOAD_GET_STATIC) {
				operation = "read static";
				type = "FIELD";
				addr = bb->addr;
			} else if ( (bb->type2 & R_ANAL_EX_LDST_LOAD_GET_FIELD)  == R_ANAL_EX_LDST_LOAD_GET_FIELD) {
				operation = "read dynamic";
				type = "FIELD";
				addr = bb->addr;
			} else if ( (bb->type2 & R_ANAL_EX_LDST_STORE_PUT_STATIC) == R_ANAL_EX_LDST_STORE_PUT_STATIC) {
				operation = "write static";
				type = "FIELD";
				addr = bb->addr;
			} else if ( (bb->type2 & R_ANAL_EX_LDST_STORE_PUT_FIELD)  == R_ANAL_EX_LDST_STORE_PUT_FIELD) {
				operation = "write dynamic";
				type = "FIELD";
				addr = bb->addr;
			} else if (bb->op_bytes[0] == 0x12) {
				addr = bb->addr;
				full_bird = r_bin_java_resolve_without_space(bin, bb->op_bytes[1]);
				operation = "read constant";
				type = r_bin_java_resolve_cp_idx_type (bin, bb->op_bytes[1]);
				r_cons_printf (fmt, addr, operation, type, full_bird);
				free (full_bird);
				free (type);
				operation = NULL;
			}

			if (operation) {
				cp_ref_idx = R_BIN_JAVA_USHORT (bb->op_bytes, 1);
				full_bird = r_cmd_java_get_descriptor (core, bin, cp_ref_idx);
				r_cons_printf (fmt, addr, operation, type, full_bird);
				free (full_bird);
			}

		}

	}
	return R_TRUE;
}

/*

typedef struct r_bin_java_attr_exception_table_entry_t {
	ut64 file_offset;
	ut16 start_pc;
	ut16 end_pc;
	ut16 handler_pc;
	ut16 catch_type;
	ut64 size;
} RBinJavaExceptionEntry;

*/

static int r_cmd_java_handle_print_exceptions (RCore *core, const char *input) {
	RAnal *anal = get_anal (core);
	RBinJavaObj *bin = (RBinJavaObj *) r_cmd_java_get_bin_obj (anal);
	RListIter *exc_iter = NULL, *methods_iter=NULL;
	RBinJavaField *method;
	ut64 func_addr = -1;
	RBinJavaExceptionEntry *exc_entry;

	const char *p = r_cmd_java_consumetok (input, ' ', -1);
	func_addr = p && *p && r_cmd_java_is_valid_input_num_value(core, p) ? r_cmd_java_get_input_num_value (core, p) : -1;

	if (!bin) return R_FALSE;

	const char *fmt = "0x%"PFMT64x" %s type: %s info: %s\n";


	r_list_foreach (bin->methods_list, methods_iter, method) {
		ut64 start = r_bin_java_get_method_start(bin, method),
			end = r_bin_java_get_method_end(bin, method);
		ut8 do_this_one = start <= func_addr && func_addr <= end;	RList * exc_table = NULL;
		do_this_one = func_addr == -1 ? 1 : do_this_one;
		if (!do_this_one) continue;
		exc_table = r_bin_java_get_method_exception_table_with_addr (bin, start);

		if (r_list_length (exc_table) == 0){
			r_cons_printf (" Exception table for %s @ 0x%"PFMT64x":\n", method->name, start);
			r_cons_printf ("\t[ NONE ]\n");
		}
		else{
			r_cons_printf (" Exception table for %s (%d entries) @ 0x%"PFMT64x":\n", method->name,
				r_list_length (exc_table) , start);
		}
		r_list_foreach (exc_table, exc_iter, exc_entry) {
			char *class_info = r_bin_java_resolve_without_space (bin, exc_entry->catch_type);
			r_cons_printf ("\tCatch Type: %d, %s @ 0x%"PFMT64x"\n", exc_entry->catch_type, class_info, exc_entry->file_offset+6);
			r_cons_printf ("\t\tStart PC: (0x%"PFMT64x") 0x%"PFMT64x" @ 0x%"PFMT64x"\n", exc_entry->start_pc, exc_entry->start_pc+start, exc_entry->file_offset);
			r_cons_printf ("\t\tEnd PC: (0x%"PFMT64x") 0x%"PFMT64x" 0x%"PFMT64x"\n", exc_entry->end_pc, exc_entry->end_pc+start, exc_entry->file_offset + 2);
			r_cons_printf ("\t\tHandler PC: (0x%"PFMT64x") 0x%"PFMT64x" 0x%"PFMT64x"\n", exc_entry->handler_pc, exc_entry->handler_pc+start, exc_entry->file_offset+4);
			free (class_info);
		}
	}

	return R_TRUE;
}

// PLUGIN Definition Info
RCorePlugin r_core_plugin_java = {
	.name = "java",
	.desc = "Suite of java commands, java help for more info",
	.license = "Apache",
	.call = r_cmd_java_call,
};

#ifndef CORELIB
struct r_lib_struct_t radare_plugin = {
	.type = R_LIB_TYPE_CMD,
	.data = &r_core_plugin_java
};
#endif
