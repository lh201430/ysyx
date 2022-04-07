#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_ADD=257,TK_SUB=258,TK_MUX=259,TK_DIV=260,TK_EQ=261,TK_AP=262,TK_NUMBER=263,TK_LEFT_BRACKET=264,TK_RIGHT_BRACKET=265,TK_REG=266

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_ADD},           
  {"-", TK_SUB},                         // plus
  {"\\*", TK_MUX},           
  {"\\/", TK_DIV},
  {"==", TK_EQ},        // equal
  {"[a-z]+",TK_AP},//alphabet
  {"[0-9]+",TK_NUMBER},//number
  {"\\(", TK_LEFT_BRACKET},
  {"\\)", TK_RIGHT_BRACKET},
  {"\\$.[a-z0-9]*", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}



typedef struct token {
  int type;   //type成员用于记录token的类型.
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

//检查左右括号
bool check_parentheses(int p, int q){
int flag = 0;
  if (tokens[p].type != TK_LEFT_BRACKET ||
      tokens[q].type != TK_RIGHT_BRACKET)
    return false;
  for (int i = p + 1; i <= q - 1; i++) {
    if (tokens[i].type == TK_LEFT_BRACKET) flag++;
    if (tokens[i].type == TK_RIGHT_BRACKET) flag--;
    if (flag < 0) return false;
  }
  if (flag == 0) return true;
  return false;


}


word_t findPrimaryOperator(int p, int q){
 
    int oneStage = -1;
  int twoStage = -1;
  int flag = 0;
  for (int i = p; i <= q; i++) {
    switch (tokens[i].type) {
      case TK_LEFT_BRACKET:
        flag++;
        break;
      case TK_RIGHT_BRACKET:
        flag--;
        break;
      case TK_MUX:
      case TK_DIV:
        if (oneStage < 0 && flag == 0) oneStage = i;
        break;
      case TK_ADD:
      case TK_SUB:
        if (twoStage < 0 && flag == 0) {
          twoStage = i;
          return twoStage;
        }
        break;

      default:
        break;
    }
  }
  return oneStage;

}


word_t eval(int p,int q){
  if (p > q) {
    /* Bad expression */
     return 0;
 }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    //vaddr_t addr;
    word_t number, data;
    bool success; 
    switch (tokens[p].type) {
      case TK_NUMBER:
        sscanf(tokens[p].str, "%lu", &number);
        return number;
      case TK_REG:
        printf("tokens数组为%s\n",tokens[p].str);
        data = isa_reg_str2val(tokens[p].str, &success);
        if (success) {
          return data;
        } else {
          return 0;
        }
      default:
        return 0;
    }

    sscanf(tokens[p].str, "%lu", &number);
    return number;


  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    /* We should do more things here. */
      //char op = 
      int op = findPrimaryOperator(p,q);
     //printf("主表达式的值为 %d",op);
      word_t val1 = eval(p, op - 1);
      word_t val2 = eval(op + 1, q);
      switch (tokens[op].type) {
      case TK_ADD:
        return val1 + val2;
      case TK_SUB:
        return val1 - val2;
      case TK_MUX:
        return val1 * val2;
      case TK_DIV:
        if (val2 == 0) return -1;
        return val1 / val2;
      default:
        assert(0);
    }


  }


  return 0;
}


static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    if (nr_token == 32) {
      printf("表达式太长\n");
      return -1;
    }

    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++) {
      //             正则     目标文本 结构体长度  结构体数组
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
          pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i,
            rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
        // 去掉寄存器的$符号
        if (*substr_start == '$') {
          substr_start++;
          substr_len--;
        }

        // 跳过空格
        if (i == 0) break;

        tokens[nr_token].type = TK_NOTYPE + i;

        memcpy(tokens[nr_token].str, substr_start, substr_len);
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  //  for(int i=0;i<32;i++){
  //     printf("tokens类型为%d  ",tokens[i].type);
  //     printf("tokens数组为%s\n",tokens[i].str);
  //   }



  return true;
}







word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
 
   // eval(0,3);
    *success = false;
    return 0;
    
  }

  /* TODO: Insert codes to evaluate the expression. */
  //eval(0,nr_token);

  return eval(0,nr_token-1);
}


