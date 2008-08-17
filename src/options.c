
#include "base.h"
#include "options.h"

option* option_new_bool(gboolean val) {
	option *opt = g_slice_new0(option);
	opt->value.opt_bool = val;
	opt->type = OPTION_BOOLEAN;
	return opt;
}

option* option_new_int(gint val) {
	option *opt = g_slice_new0(option);
	opt->value.opt_int = val;
	opt->type = OPTION_INT;
	return opt;
}

option* option_new_string(GString *val) {
	option *opt = g_slice_new0(option);
	opt->value.opt_string = val;
	opt->type = OPTION_STRING;
	return opt;
}

option* option_new_list() {
	option *opt = g_slice_new0(option);
	opt->value.opt_list = g_array_new(FALSE, TRUE, sizeof(option*));
	opt->type = OPTION_LIST;
	return opt;
}

void _option_hash_free_key(gpointer data) {
	g_string_free((GString*) data, TRUE);
}

void _option_hash_free_value(gpointer data) {
	option_free((option*) data);
}

option* option_new_hash() {
	option *opt = g_slice_new0(option);
	opt->value.opt_hash = g_hash_table_new_full(
		(GHashFunc) g_string_hash, (GEqualFunc) g_string_equal,
		_option_hash_free_key, _option_hash_free_value);
	opt->type = OPTION_HASH;
	return opt;
}

option* option_new_action(server *srv, action *a) {
	option *opt = g_slice_new0(option);
	opt->value.opt_action.srv = srv;
	opt->value.opt_action.action = a;
	opt->type = OPTION_ACTION;
	return opt;
}

option* option_new_condition(server *srv, condition *c) {
	option *opt = g_slice_new0(option);
	opt->value.opt_cond.srv = srv;
	opt->value.opt_cond.cond = c;
	opt->type = OPTION_CONDITION;
	return opt;
}

option* option_copy(option* opt) {
	option *n;

	switch (opt->type) {
		case OPTION_NONE: n = option_new_bool(FALSE); n->type = OPTION_NONE; return n; /* hack */
		case OPTION_BOOLEAN: return option_new_bool(opt->value.opt_bool);
		case OPTION_INT: return option_new_int(opt->value.opt_int);
		case OPTION_STRING: return option_new_string(g_string_new_len(GSTR_LEN(opt->value.opt_string)));
		/* list: we have to copy every option in the list! */
		case OPTION_LIST:
			n = option_new_list();
			g_array_set_size(n->value.opt_list, opt->value.opt_list->len);
			for (guint i = 0; i < opt->value.opt_list->len; i++)
				//g_array_insert_val((*(n->value.opt_list)), i, option_copy(g_array_index(opt->value.opt_list, option*, i)));
				g_array_insert_vals(n->value.opt_list, i, option_copy(g_array_index(opt->value.opt_list, option*, i)), 1);
			return n;
		/* hash: iterate over hashtable, clone each option */
		case OPTION_HASH:
			n = option_new_hash();
			{
				GHashTableIter iter;
				gpointer k, v;
				g_hash_table_iter_init(&iter, opt->value.opt_hash);
				while (g_hash_table_iter_next(&iter, &k, &v))
					g_hash_table_insert(n->value.opt_hash, g_string_new_len(GSTR_LEN((GString*)k)), option_copy((option*)v));
			}
			return n;
		/* TODO: does it make sense to clone action and condition options? */
		case OPTION_ACTION:
		case OPTION_CONDITION:
			assert(NULL);
			return NULL;
	}
}

void option_free(option* opt) {
	if (!opt) return;

	switch (opt->type) {
	case OPTION_NONE:
	case OPTION_BOOLEAN:
	case OPTION_INT:
		/* Nothing to free */
		break;
	case OPTION_STRING:
		g_string_free(opt->value.opt_string, TRUE);
		break;
	case OPTION_LIST:
		option_list_free(opt->value.opt_list);
		break;
	case OPTION_HASH:
		g_hash_table_destroy((GHashTable*) opt->value.opt_hash);
		break;
	case OPTION_ACTION:
		action_release(opt->value.opt_action.srv, opt->value.opt_action.action);
		break;
	case OPTION_CONDITION:
		condition_release(opt->value.opt_cond.srv, opt->value.opt_cond.cond);
		break;
	}
	opt->type = OPTION_NONE;
	g_slice_free(option, opt);
}

const char* option_type_string(option_type type) {
	switch(type) {
	case OPTION_NONE:
		return "none";
	case OPTION_BOOLEAN:
		return "boolean";
	case OPTION_INT:
		return "int";
	case OPTION_STRING:
		return "string";
	case OPTION_LIST:
		return "list";
	case OPTION_HASH:
		return "hash";
	case OPTION_ACTION:
		return "action";
	case OPTION_CONDITION:
		return "condition";
	}
	return "<unknown>";
}

void option_list_free(GArray *optlist) {
	if (!optlist) return;
	for (gsize i = 0; i < optlist->len; i++) {
		option_free(g_array_index(optlist, option*, i));
	}
	g_array_free(optlist, TRUE);
}

/* Extract value from option, option set to none */
gpointer option_extract_value(option *opt) {
	gpointer val = NULL;
	if (!opt) return NULL;

	switch (opt->type) {
		case OPTION_NONE:
			break;
		case OPTION_BOOLEAN:
			val = GINT_TO_POINTER(opt->value.opt_bool);
			break;
		case OPTION_INT:
			val =  GINT_TO_POINTER(opt->value.opt_int);
			break;
		case OPTION_STRING:
			val =  opt->value.opt_string;
			break;
		case OPTION_LIST:
			val =  opt->value.opt_list;
			break;
		case OPTION_HASH:
			val =  opt->value.opt_hash;
			break;
		case OPTION_ACTION:
			val = opt->value.opt_action.action;
			break;
		case OPTION_CONDITION:
			val = opt->value.opt_action.action;
			break;
	}
	opt->type = OPTION_NONE;
	return val;
}
