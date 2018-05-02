#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint64_t tagged_obj;

static int has_char = 0;
static char peek_char;

static char peek(void) {
  if (!has_char) {
    peek_char = getchar();
    has_char = 1;
  }
  return peek_char;
}

static char read_char(void) {
  if (has_char) {
    has_char = 0;
    return peek_char;
  } else {
    return getchar();
  }
}

static void unread_char(char ch) {
  has_char = 1;
  peek_char = ch;
}

struct symtab {
  struct symtab *left, *right;
  tagged_obj (*fn)(tagged_obj, tagged_obj);
  char sym[];
};

static struct symtab *symtab;

static tagged_obj intern(char *name) {
  struct symtab **ptr = &symtab;
  while (*ptr) {
    int direction = strcmp(name, (*ptr)->sym);
    if (direction == 0)
      return (tagged_obj) *ptr;
    else if (direction < 0)
      ptr = &(*ptr)->left;
    else
      ptr = &(*ptr)->right;
  }
  *ptr = malloc(sizeof(**ptr) + strlen(name) + 1);
  (*ptr)->left = (*ptr)->right = (*ptr)->fn = NULL;
  strcpy((*ptr)->sym, name);
  return (tagged_obj) *ptr;
}

struct cons {
  tagged_obj car, cdr;
};

static int atom(tagged_obj obj) {
  return !(obj & 0x1);
}

static tagged_obj cons(tagged_obj car, tagged_obj cdr) {
  struct cons *obj = malloc(sizeof(*obj));
  obj->car = car;
  obj->cdr = cdr;
  return (uintptr_t) obj | 0x1;
}

static tagged_obj car(tagged_obj cons) {
  if (atom(cons))
    return intern("nil");
  else
    return ((struct cons *) (cons & ~0x1))->car;
}
static tagged_obj cdr(tagged_obj cons) {
  if (atom(cons))
    return intern("nil");
  else
    return ((struct cons *) (cons & ~0x1))->cdr;
}

static tagged_obj read_obj(void);

static tagged_obj read_list(char end) {
  tagged_obj ret = intern("nil"), *ptr = &ret;
  char ch;
  while ((ch = read_char()) != end) {
    unread_char(ch);
    *ptr = cons(read_obj(), intern("nil"));
    ptr = &((struct cons *) ((uintptr_t) *ptr & ~0x1))->cdr;
  }
  return ret;
}

static tagged_obj read_obj(void) {
  char ch = read_char();
  if (ch == '(')
    return read_list(')');
  else if (ch == '\0' || strchr(" \t\n", ch))
    return read_obj();
  else if (ch == '\'')
    return cons(intern("quote"), cons(read_obj(), intern("nil")));
  else {
    size_t buf_len = 16, buf_i = 0;
    char *buf = malloc(buf_len+1);
    do {
      buf[buf_i++] = ch;
      if (buf_i >= buf_len)
	buf = realloc(buf, (buf_len *= 2) + 1);
      ch = read_char();
    } while (!(ch == '\0' || strchr(" \t\n()", ch)));
    unread_char(ch);
    buf[buf_i] = '\0';
    return intern(buf);
  }
}

static tagged_obj assoc(tagged_obj lst, tagged_obj key) {
  if (lst == intern("nil"))
    return intern("nil");
  else
    return car(car(lst)) == key ? cdr(car(lst)) : assoc(cdr(lst), key);
}

static tagged_obj eval_in_lexenv(tagged_obj expr, tagged_obj env) {
  if (atom(expr)) {
    return assoc(env, expr);
  } else {
    tagged_obj fn_name = car(expr);
    if (atom(fn_name) && ((struct symtab *) fn_name)->fn)
      return ((struct symtab *) fn_name)->fn(cdr(expr), env);

    tagged_obj fn = eval_in_lexenv(car(expr), env);
    if (!atom(fn)) {
      if (car(fn) == intern("lambda")) {
	tagged_obj params = car(cdr(fn));
	tagged_obj args = cdr(expr);
	tagged_obj new_env = env;
	while (params != intern("nil") && args != intern("nil")) {
	  new_env = cons(cons(car(params), eval_in_lexenv(car(args), env)), new_env);
	  params = cdr(params);
	  args = cdr(args);
	}
	tagged_obj form = cdr(cdr(fn)), ret = intern("nil");
	while (form != intern("nil")) {
	  ret = eval_in_lexenv(car(form), new_env);
	  form = cdr(form);
	}
	return ret;
      } else if (car(fn) == intern("macro")) {
	tagged_obj params = car(cdr(fn));
	tagged_obj args = cdr(expr);
	tagged_obj new_env = env;
	while (params != intern("nil") && args != intern("nil")) {
	  new_env = cons(cons(car(params), car(args)), new_env);
	  params = cdr(params);
	  args = cdr(args);
	}
	tagged_obj form = cdr(cdr(fn)), ret = intern("nil");
	while (form != intern("nil")) {
	  ret = eval_in_lexenv(car(form), new_env);
	  form = cdr(form);
	}
	return eval_in_lexenv(ret, env);
      }
    }
    return intern("nil");
  }
}

static void print_obj(tagged_obj obj) {
  if (atom(obj)) {
    printf("%s", ((struct symtab *) obj)->sym);
  } else {
    printf("(");
    int first = 1;
    while (!atom(obj)) {
      if (!first)
	printf(" ");
      else
	first = 0;
      print_obj(car(obj));
      obj = cdr(obj);
    }
    if (obj != intern("nil")) {
      printf(" . ");
      print_obj(obj);
    }
    printf(")");
  }
}

static tagged_obj eval_obj(tagged_obj expr) {
  return eval_in_lexenv(expr, intern("nil"));
}

static tagged_obj fquote(tagged_obj args, tagged_obj env) {
  return car(args);
}
static tagged_obj fcons(tagged_obj args, tagged_obj env) {
  return cons(eval_in_lexenv(car(args), env), eval_in_lexenv(car(cdr(args)), env));
}
static tagged_obj fcar(tagged_obj args, tagged_obj env) {
  return car(eval_in_lexenv(car(args), env));
}
static tagged_obj fcdr(tagged_obj args, tagged_obj env) {
  return cdr(eval_in_lexenv(car(args), env));
}
static tagged_obj let(tagged_obj args, tagged_obj env) {
  tagged_obj new_env = env;
  for (tagged_obj decls = car(args); decls != intern("nil"); decls = cdr(decls))
    new_env = cons(cons(car(car(decls)), eval_in_lexenv(car(cdr(car(decls))), env)), new_env);
  tagged_obj ret = intern("nil");
  for (args = cdr(args); args != intern("nil"); args = cdr(args))
    ret = eval_in_lexenv(car(args), new_env);
  return ret;
}

int main() {
  ((struct symtab *) intern("quote"))->fn = fquote;
  ((struct symtab *) intern("cons"))->fn = fcons;
  ((struct symtab *) intern("car"))->fn = fcar;
  ((struct symtab *) intern("cdr"))->fn = fcdr;
  ((struct symtab *) intern("let"))->fn = let;
  for (;;) {
    printf("pavlisp> ");
    print_obj(eval_obj(read_obj()));
    printf("\n");
  }
  return 0;
}
