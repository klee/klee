array set LANGS {
	sharp {
		TYPES {
			Agraph_t* SWIGTYPE_p_Agraph_t
                	Agnode_t* SWIGTYPE_p_Agnode_t
                	Agedge_t* SWIGTYPE_p_Agedge_t
                	Agsym_t* SWIGTYPE_p_Agsym_t
                	char* string
                	{const char*} string
                	char** outdata
                	FILE* SWIGTYPE_p_FILE
                	bool bool
			int int
                	void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
		}
		USAGE {
		}
	}
	go {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
		}
		USAGE {
		}
	}
	guile {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
                	void** data_handle
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
			{(load-extension "./libgv.so" "SWIG_init")}
		}
		USAGE {
		}
	}
	io {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
		}
		USAGE {
		}
	}
	java {
		TYPES {
			Agraph_t* SWIGTYPE_p_Agraph_t
			Agnode_t* SWIGTYPE_p_Agnode_t
			Agedge_t* SWIGTYPE_p_Agedge_t
			Agsym_t* SWIGTYPE_p_Agsym_t
			char* string
			{const char*} string
			char** outdata
			FILE* SWIGTYPE_p_FILE
			bool bool
			int int
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
			{System.loadLibrary("gv");}
		}
		USAGE {
		}
	}
	lua {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
			{#!/usr/bin/lua}
			{require('gv')}
		}
		USAGE {
		}
	}
	ocaml {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
		}
		USAGE {
		}
	}
	perl {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			gv:: (  {, }   {);}
		}
		SYNOPSIS {
			{#!/usr/bin/perl}
			{use gv;}
		}
		USAGE {
		}
	}
	php {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			gv:: (  {, }   {);}
		}
		SYNOPSIS {
			{#!/usr/bin/php}
			{<?}
			{include("gv.php")}
			{...}
			{?>}
		}
		USAGE {
		}
	}
	python {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
			{#!/usr/bin/python}
			{import sys}
			{import gv}
		}
		USAGE {
		}
	}
	R {
		TYPES {
			Agraph_t* SWIGTYPE_p_Agraph_t
			Agnode_t* SWIGTYPE_p_Agnode_t
			Agedge_t* SWIGTYPE_p_Agedge_t
			Agsym_t* SWIGTYPE_p_Agsym_t
			char* string
			{const char*} string
			char** outdata
			FILE* SWIGTYPE_p_FILE
			bool bool
			int int
			void {}
		}
		SYNTAX {
			gv.  (  {, }   {);}
		}
		SYNOPSIS {
			{System.loadLibrary("gv");}
		}
		USAGE {
		}
	}
	ruby {
		TYPES {
			{Agraph_t* g} graph_handle
			{Agraph_t* sg} subgraph_handle
			{Agnode_t* n} node_handle
			{Agnode_t* t} tail_node_handle
			{Agnode_t* h} head_node_handle
			{Agedge_t* e} edge_handle
			{Agsym_t* a} attr_handle
			{char* gne} type
			{char* name} name
			{char* tname} tail_name
			{char* hname} head_name
			{char* attr} attr_name
			{char* val} attr_value
			{const char* filename} filename
			{char* engine} engine
			{char* string} string
			{char** outdata} outdata
			{char* format} format
			{FILE* f} channel
			{void** data} data_handle
			Agraph_t* graph_handle
			Agnode_t* node_handle
			Agedge_t* edge_handle
			Agsym_t* attribute_handle
			char* string
			{const char*} string
			char** outdata
			FILE* channel
			bool bool
			int int
			void** data_handle
			void {}
		}
		SYNTAX {
			Gv.  (  {, }   {);}
		}
		SYNOPSIS {
			{#!/usr/bin/ruby}
			{require 'gv'}
		}
		USAGE {
		}
	}
	tcl {
		TYPES {
			{Agraph_t* g} <graph_handle>
			{Agraph_t* sg} <subgraph_handle>
			{Agnode_t* n} <node_handle>
			{Agnode_t* t} <tail_node_handle>
			{Agnode_t* h} <head_node_handle>
			{Agedge_t* e} <edge_handle>
			{Agsym_t* a} <attr_handle>
			{char* gne} <type>
			{char* name} <name>
			{char* tname} <tail_name>
			{char* hname} <head_name>
			{char* attr} <attr_name>
			{char* val} <attr_value>
			{const char* filename} <filename>
			{char* engine} <engine>
			{char* string} <string>
			{char** outdata} <outdata>
			{char* format} <format>
			{FILE* f} <channel>
			{void** data} <data_handle>
			Agraph_t* <graph_handle>
			Agnode_t* <node_handle>
			Agedge_t* <edge_handle>
			Agsym_t* <attr_handle>
			char* <string>
			{const char*} <string>
			char** <outdata>
			FILE* <channel>
			bool <boolean_string>
			int <integer_string>
			void** <data_handle>
			void {}
		}
		SYNTAX {
			gv:: {} { } {}
		}
		SYNOPSIS {
			{#!/usr/bin/tclsh}
			{package require gv}
		}
		USAGE {
			{Requires tcl8.3 or later.}
		}
	}
}
