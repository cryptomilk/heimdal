#include "asn1_locl.h"

RCSID("$Id$");

static void
decode_primitive (char *typename, char *name)
{
    fprintf (codefile,
	     "e = decode_%s(p, len, %s, &l);\n"
	     "FORW;\n",
	     typename,
	     name);
}

static void
decode_type (char *name, Type *t)
{
  switch (t->type) {
  case TType:
#if 0
    decode_type (name, t->symbol->type);
#endif
    fprintf (codefile,
	     "e = decode_%s(p, len, %s, &l);\n"
	     "FORW;\n",
	     t->symbol->gen_name, name);
    break;
  case TInteger:
    decode_primitive ("integer", name);
    break;
  case TOctetString:
    decode_primitive ("octet_string", name);
    break;
  case TBitString: {
    Member *m;
    int tag = -1;
    int pos;

    fprintf (codefile,
	     "e = der_match_tag_and_length (p, len, UNIV, PRIM, UT_BitString,"
	     "&reallen, &l);\n"
	     "FORW;\n"
	     "if(len < reallen)\n"
	     "return ASN1_OVERRUN;\n"
	     "p++;\n"
	     "len--;\n"
	     "reallen--;\n"
	     "ret++;\n");
    pos = 0;
    for (m = t->members; m && tag != m->val; m = m->next) {
      while (m->val / 8 > pos / 8) {
	fprintf (codefile,
		 "p++; len--; reallen--; ret++;\n");
	pos += 8;
      }
      fprintf (codefile,
	       "%s->%s = (*p >> %d) & 1;\n",
	       name, m->gen_name, 7 - m->val % 8);
      if (tag == -1)
	tag = m->val;
    }
    fprintf (codefile,
	     "p += reallen; len -= reallen; ret += reallen;\n");
    break;
  }
  case TSequence: {
    Member *m;
    int tag = -1;

    if (t->members == NULL)
      break;

    fprintf (codefile,
	     "e = der_match_tag_and_length (p, len, UNIV, CONS, UT_Sequence,"
	     "&reallen, &l);\n"
	     "FORW;\n"
	     "{\n"
	     "int dce_fix;\n"
	     "if((dce_fix = fix_dce(reallen, &len)) < 0)\n"
	     "return ASN1_BAD_FORMAT;\n");

    for (m = t->members; m && tag != m->val; m = m->next) {
      char *s = malloc(2 + strlen(name) + 1 + strlen(m->gen_name) + 3);

      sprintf (s, "%s(%s)->%s", m->optional ? "" : "&", name, m->gen_name);
      if (0 && m->type->type == TType){
	  if(m->optional)
	      fprintf (codefile,
		       "%s = malloc(sizeof(*%s));\n", s, s);
	  fprintf (codefile, 
		   "e = decode_seq_%s(p, len, %d, %d, %s, &l);\n",
		   m->type->symbol->gen_name,
		   m->val, 
		   m->optional,
		   s);
	  if(m->optional)
	      fprintf (codefile, 
		       "if (e == ASN1_MISSING_FIELD) {\n"
		       "free(%s);\n"
		       "%s = NULL;\n"
		       "e = l = 0;\n"
		       "}\n",
		       s, s);
	  
	  fprintf (codefile, "FORW;\n");
	  
      }else{
      fprintf (codefile, "{\n"
	       "size_t newlen, oldlen;\n\n"
	       "e = der_match_tag (p, len, CONTEXT, CONS, %d, &l);\n",
	       m->val);
      fprintf (codefile,
	       "if (e)\n");
      if(m->optional)
	  /* XXX should look at e */
	  fprintf (codefile,
		   "%s = NULL;\n", s);
      else
	  fprintf (codefile,
		   "return e;\n");
      fprintf (codefile, 
	       "else {\n");
      fprintf (codefile,
	       "p += l;\n"
	       "len -= l;\n"
	       "ret += l;\n"
	       "e = der_get_length (p, len, &newlen, &l);\n"
	       "FORW;\n"
	       "{\n"
	       
	       "int dce_fix;\n"
	       "oldlen = len;\n"
	       "if((dce_fix = fix_dce(newlen, &len)) < 0)"
	       "return ASN1_BAD_FORMAT;\n");
      if (m->optional)
	fprintf (codefile,
		 "%s = malloc(sizeof(*%s));\n",
		 s, s);
      decode_type (s, m->type);
      fprintf (codefile,
	       "if(dce_fix){\n"
	       "e = der_match_tag_and_length (p, len, 0, 0, 0, "
	       "&reallen, &l);\n"
	       "FORW;\n"
	       "}else \n"
	       "len = oldlen - newlen;\n"
	       "}\n"
	       "}\n");
      fprintf (codefile,
		 "}\n");
      }
      if (tag == -1)
	tag = m->val;
      free (s);
    }
    fprintf(codefile,
	    "if(dce_fix){\n"
	    "e = der_match_tag_and_length (p, len, 0, 0, 0, &reallen, &l);\n"
	    "FORW;\n"
	    "}\n"
	    "}\n");

    break;
  }
  case TSequenceOf: {
    char *n = malloc(2*strlen(name) + 20);

    fprintf (codefile,
	     "e = der_match_tag_and_length (p, len, UNIV, CONS, UT_Sequence,"
	     "&reallen, &l);\n"
	     "FORW;\n"
	     "if(len < reallen)\n"
	     "return ASN1_OVERRUN;\n"
	     "len = reallen;\n");

    fprintf (codefile,
	     "(%s)->len = 0;\n"
	     "(%s)->val = NULL;\n"
	     "while(len > 0) {\n"
	     "(%s)->len++;\n"
	     "(%s)->val = realloc((%s)->val, sizeof(*((%s)->val)) * (%s)->len);\n",
	     name, name, name, name, name, name, name);
    sprintf (n, "&(%s)->val[(%s)->len-1]", name, name);
    decode_type (n, t->subtype);
    fprintf (codefile, 
	     "}\n");
    free (n);
    break;
  }
  case TGeneralizedTime:
    decode_primitive ("generalized_time", name);
    break;
  case TGeneralString:
    decode_primitive ("general_string", name);
    break;
  case TApplication:
    fprintf (codefile,
	     "e = der_match_tag_and_length (p, len, APPL, CONS, %d, "
	     "&reallen, &l);\n"
	     "FORW;\n"
	     "{\n"
	     "int dce_fix;\n"
	     "if((dce_fix = fix_dce(reallen, &len)) < 0)\n"
	     "return ASN1_BAD_FORMAT;\n", 
	     t->application);
    decode_type (name, t->subtype);
    fprintf(codefile,
	    "if(dce_fix){\n"
	    "e = der_match_tag_and_length (p, len, 0, 0, 0, &reallen, &l);\n"
	    "FORW;\n"
	    "}\n"
	    "}\n");

    break;
  default :
    abort ();
  }
}

void
generate_type_decode (Symbol *s)
{
  fprintf (headerfile,
	   "int decode_%s(unsigned char *, size_t, %s *, size_t *);\n",
	   s->gen_name, s->gen_name);

  fprintf (codefile, "#define FORW "
	   "if(e) return e; "
	   "p += l; "
	   "len -= l; "
	   "ret += l\n\n");


  fprintf (codefile, "int\n"
	   "decode_%s(unsigned char *p, size_t len, %s *data, size_t *size)\n"
	   "{\n",
	   s->gen_name, s->gen_name);

  switch (s->type->type) {
  case TInteger:
    fprintf (codefile, "return decode_integer (p, len, data, size);\n");
    break;
  case TOctetString:
    fprintf (codefile, "return decode_octet_string (p, len, data, size);\n");
    break;
  case TGeneralizedTime:
    fprintf (codefile, "return decode_generalized_time (p, len, data, size);\n");
    break;
  case TGeneralString:
    fprintf (codefile, "return decode_general_string (p, len, data, size);\n");
    break;
  case TBitString:
  case TSequence:
  case TSequenceOf:
  case TApplication:
  case TType:
    fprintf (codefile,
	     "size_t ret = 0, reallen;\n"
	     "size_t l;\n"
	     "int e, i;\n\n");
    
    decode_type ("data", s->type);
    fprintf (codefile, 
	     "*size = ret;\n"
	     "return 0;\n");
    break;
  default:
    abort ();
  }
  fprintf (codefile, "}\n\n");
}

void
generate_seq_type_decode (Symbol *s)
{
    fprintf (headerfile,
	     "int decode_seq_%s(unsigned char *, size_t, int, int, "
	     "%s *, size_t *);\n",
	     s->gen_name, s->gen_name);

    fprintf (codefile, "int\n"
	     "decode_seq_%s(unsigned char *p, size_t len, int tag, "
	     "int optional, %s *data, size_t *size)\n"
	     "{\n",
	     s->gen_name, s->gen_name);

    fprintf (codefile,
	     "size_t newlen, oldlen;\n"
	     "size_t l, ret = 0;\n"
	     "int e;\n"
	     "int dce_fix;\n");
    
    fprintf (codefile,
	     "e = der_match_tag(p, len, CONTEXT, CONS, tag, &l);\n"
	     "if (e)\n"
	     "return e;\n");
    fprintf (codefile, 
	     "p += l;\n"
	     "len -= l;\n"
	     "ret += l;\n"
	     "e = der_get_length(p, len, &newlen, &l);\n"
	     "if (e)\n"
	     "return e;\n"
	     "p += l;\n"
	     "len -= l;\n"
	     "ret += l;\n"
	     "oldlen = len;\n"
	     "if ((dce_fix = fix_dce(newlen, &len)) < 0)\n"
	     "return ASN1_BAD_FORMAT;\n"
	     "e = decode_%s(p, len, data, &l);\n"
	     "if (e)\n"
	     "return e;\n"
	     "p += l;\n"
	     "len -= l;\n"
	     "ret += l;\n"
	     "if (dce_fix) {\n"
	     "size_t reallen;\n\n"
	     "e = der_match_tag_and_length(p, len, 0, 0, 0, &reallen, &l);\n"
	     "if (e)\n"
	     "return e;\n"
	     "ret += l;\n"
	     "}\n",
	     s->gen_name);
    fprintf (codefile, 
	     "*size = ret;\n"
	     "return 0;\n");

    fprintf (codefile, "}\n\n");
}


#if 0
static void
generate_type_decode (Symbol *s)
{
  fprintf (headerfile,
	   "int decode_%s(unsigned char *, int, %s *);\n",
	   s->gen_name, s->gen_name);

  fprintf (codefile, "int\n"
	   "decode_%s(unsigned char *p, int len, %s *data)\n"
	   "{\n"
	   "int ret = 0, reallen;\n"
	   "int l, i;\n\n",
	   s->gen_name, s->gen_name);

  decode_type ("data", s->type);
  fprintf (codefile, "return ret;\n"
	   "}\n\n");
}
#endif

