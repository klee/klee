#include<klee/klee.h>
#ifdef DIRECT
#include "taint_direct.h"
#else
#ifdef CONTROLFLOW 
#include "taint_controlflow.h"
#else REGIONS 
#include "taint_regions.h"
#endif
#endif

#define TEST(x) { printf("testing " #x " \n"); klee_set_pc_taint(0); test_##x(); }

int NO_OPTIM;

void klee_clear_taint(void * buffer, size_t size){
  klee_set_taint(0,buffer,size);
}


int
funcAdd (int a, int b)
{
  return a + b;
}

void
test_CallRet ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = funcAdd (a, b);
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_Unwind ()
{
  // cant reproduce in C, no exceptions...
}

void
test_Br ()
{
  int a = 1;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  if (a==1)
    a=c;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 0);
}

void
test_Switch ()
{
  int a = 1;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  switch(a)
    {
    case 0: break;
    case 1: a=c; break;
    }

  klee_assert (klee_get_taint (&c, sizeof (c)) == 0 );
}

void
test_Unreachable ()
{
  // The 'unreachable' instruction has no defined semantics.
}

void
test_Invoke ()
{
  // how to test?
}

void
test_PHI ()
{
  // how to test?
}

void
test_Select ()
{
  // esto no compila a select ! :(
  int a = 1;
  int c = 0;
  int boolean=1;
  a = boolean ? c : a;
  klee_set_taint (1, &a, sizeof (a));
  
}

void
test_VAArg ()
{
  // ?
}

void
test_Add ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a + b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_Sub ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a - b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_Mul ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a * b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_UDiv ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a / b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_SDiv ()
{
  int a = -1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a / b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_URem ()
{
  unsigned int a = 120;
  unsigned int b = 100;
  unsigned int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a % b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_SRem ()
{
  int a = 120;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a % b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_And ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a & b;
  printf("C taint: %d\n", klee_get_taint (&c, sizeof (c)));
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_Or ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a | b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_Xor ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a ^ b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_Shl ()
{
  int a = 1;
  int b = 8;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a << b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_LShr ()
{
  int a = 1;
  int b = 8;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a >> b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_AShr ()
{
  int a = -1;
  int b = 1;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a >> b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_ICmp ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a == b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}


void test_gep()
{
  char a[256];

  char i=2;
  klee_set_taint (1, &i, sizeof (i));

  int r = *(a+i);

  klee_assert (klee_get_taint (&r, sizeof(r)) == 1 );


}


void test_idx()
{
  char a[256];

  int i=2;
  klee_set_taint (1, &i, sizeof (i));
  
  a[i]=1;

  klee_assert (klee_get_taint (&a[i], sizeof (a[i])) == 1 );

}



void
test_Malloc ()
{
  // mallo y free son funciones builtin de llvm llamadas desde adentro?
}


void
test_Free ()
{
}

void
test_Alloca ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  int *ptr = (int *) malloc(2 * sizeof (int));
  *ptr = a;
  *(ptr+1) = b;
  c = *ptr + *(ptr+1);
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1|2 );

  free(ptr);
}

/* this function test that taint is propagated through pointers
   here value_p points originally to value_w. when *value_p (that is value_w) 
   is changed to value_r, the taint of value_w should have the combined taints
   of value_p and value_r!
*/
void
test_Store ()
{
  int value_w = 0x12345678;
  int *value_p = &value_w;
  int value_r = 0x1111111;

  klee_set_taint(0, &value_w, sizeof(value_w));
  klee_set_taint(1, &value_p, sizeof(value_p));
  klee_set_taint(2, &value_r, sizeof(value_r));

  *value_p = value_r;

  //printf ("Value_w: 0x%08x\n", value_w);
  printf ("Value_p taint: %x\n", klee_get_taint(&value_p, sizeof(value_p)));
  printf ("Value_w: 0x%08x taint %x\n", value_w, klee_get_taint(&value_w, sizeof(value_w)));

klee_assert(klee_get_taint(&value_w,sizeof(value_w))==3);

  return ;

}

void
test_GetElementPtr ()
{
  // testeado por Load y Store
}

void
test_Trunc ()
{
  // testeado por test_CovertChannel
}

void
test_ZExt ()
{
  // testeado por test_CovertChannel
}

void
test_SExt ()
{
  // testeado por test_gep
}

void
test_IntToPtr ()
{
  // testeado por test_MemoryOffsetTaint
}

void
test_PtrToInt ()
{
  // testeado por test_MemoryOffsetTaint
}

void
test_BitCast ()
{
  // testeado por test_MemoryOffsetTaint
}

void
test_FAdd ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a + b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_FSub ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a - b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_FMul ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a * b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_FDiv ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a / b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_FRem ()
{
  // y esto?
}

void
test_FPTrunc ()
{
  double a = 1.0;
  float c = 0.0;
  klee_set_taint (4, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) ==  4 );
}

void
test_FPExt ()
{
  float a = 1.0;
  double c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_FPToUI ()
{
  float a = 1.0;
  unsigned int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_FPToSI ()
{
  float a = 1.0;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_UIToFP ()
{
  float c = 1.0;
  unsigned int a = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_SIToFP ()
{
  float c = 1.0;
  int a = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 ));
}

void
test_FCmp ()
{
  float a = 1.0;
  float b = 2.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  if (a>b)
    c=a;
  else
    c=b;
#ifdef DIRECT
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1);
#else
  klee_assert (klee_get_taint (&c, sizeof (c)) == 3);
#endif
}

void
test_InsertValue ()
{
  // como declarar arrays nativos de llvm?
}

void
test_ExtractValue ()
{
  // como llamar a 'llvm.sadd.with.overflow.*' ?
}

void
test_ExtractElement ()
{
  // idem arrays pero con vectores
}

void
test_InsertElement ()
{
  // idem arrays pero con vectores
}

void
test_ShuffleVector ()
{
  // idem arrays pero con vectores
}

/**
 * Constants should not contribute any taint
 */
void
test_AddConst ()
{
  int a = 10;
  int b = 10;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (1, &b, sizeof (b));

  int c ;

  c = b+1;

  klee_assert(klee_get_taint (&c, sizeof (c))==1);
}

/* Tests taint is mantained under internal function calls
 */
int
test_Nested_Fun()
{
  int result;
  int param=1;
  int inner_param=param+1;

  klee_set_taint(1, &param, sizeof(param));
  fun(param,inner_param);

  result = result / 2;

  klee_assert(klee_get_taint(&result,sizeof(result))==RESULT_ASSERT_NESTED_FUN);

  return result;
}


int fun(int param, int inner_param){
  int result;

  if (param==1) 
    {

      if (inner_param==2)
	result=1;
      else
	result=2;

      result = result*2;
    }
  else
    result = 0;

  return result;
}


void jmpfun(int *result, int param){
  *result=param;
}

/* Tests taint propagation in parameter passing to function called by pointer
 */
int
test_Jump(){
  void (*ptrFun)();
  int result;
  int param;
  klee_set_taint(1, &param, sizeof(param));

  ptrFun = &jmpfun;
  ptrFun(&result, param);

  klee_assert(klee_get_taint(&result,sizeof(result))==1);

}

/* like test_Conditional but nested 
 */
int
test_Nested_Conditional()
{
  int result;
  int param=0;
  int inner_param=param+1;

  klee_set_taint(1, &param, sizeof(param));

  if (param==1) 
    {

      if (inner_param==2)
	result=1;
      else
	result=2;

      result = result*2;
    }
  else
    result = 0;

  result = result / 2;

  klee_assert(klee_get_taint(&result,sizeof(result))==RESULT_ASSERT_NESTED_CONDITIONAL);

  return result;

}

/* taint propagation by indirect flow in the conditional (in the while)
 */ 
int
test_While()
{
  int result;
  int param=0;

  klee_set_taint(1,&param,sizeof(param));

  while (param<=10) {
    param++;
    result = result*2;
  }

  result = result / 2;

  klee_assert(klee_get_taint(&result,sizeof(result))==RESULT_TEST_WHILE);
  

  return result;
}

/* taint propagation by indirect flow in the conditional
 */ 
int
test_Conditional_Regions()
{
  int result;
  int param=1;
  klee_set_taint(1,&param,sizeof(param));
  if (param == 1)
    result = 1;
  else
    result = 2;
  
  result = result + 1;
  klee_assert(klee_get_taint(&result,sizeof(result))==RESULT_ASSERT_TEST_CONDITIONAL_REGIONS);
  return result;
}

/* taint propagation by indirect flow in the conditional
 */ 
int
test_Conditional()
{
  int result;
  int param=1;
  klee_set_taint(1,&param,sizeof(param));
  if (param == 1)
    result = 1;
  else
    result = 2;
  
  klee_assert(klee_get_taint(&result,sizeof(result))==RESULT_ASSERT_TEST_CONDITIONAL);
  return result;
}

void
test_Buffer_Star ()
{
  char * buffer = "AAAAAAAAAAAAAAAA";
  unsigned i;


  //    printf ("Testing tainting buffers\n");

  //printf ("Taint all 16 bytes of a buffer with TAINT_A\n");
  klee_set_taint (1, buffer, 16);

  klee_assert (1 == klee_get_taint (buffer, 16));
  for (i = 0; i < 16; i++)
    klee_set_taint (1 << i, buffer + i, 1);


  klee_assert (klee_get_taint (buffer, 16) == 0xffff);
  for (i = 0; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == (1 << i) | 1);

  //printf ("Clear taint of the second half of the buffer\n");
  klee_clear_taint (buffer + 8, 8);
  klee_assert (klee_get_taint (buffer, 16) == 0xff);
  for (i = 0; i < 8; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == (1 << i) | 1);
  for (i = 8; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == 0);


  //printf ("Clear taint of the buffer\n");
  klee_clear_taint (buffer, 16);
  klee_assert (klee_get_taint (buffer, 16) == 0);
  for (i = 0; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == 0);

}

void
test_Buffer ()
{
  char buffer[16];
  unsigned i;
  //printf ("Testing tainting buffers\n");

  //printf ("Taint all 16 bytes of a buffer with TAINT_A\n");
  klee_set_taint (1, buffer, 16);

  klee_assert (1 == klee_get_taint (buffer, 16));
  for (i = 0; i < 16; i++)
    klee_set_taint (1 << i, buffer + i, 1);


  klee_assert (klee_get_taint (buffer, 16) == 0xffff);
  for (i = 0; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == (1 << i) | 1);

  //printf ("Clear taint of the second half of the buffer\n");
  klee_clear_taint (buffer + 8, 8);
  klee_assert (klee_get_taint (buffer, 16) == 0xff);
  for (i = 0; i < 8; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == (1 << i) | 1);
  for (i = 8; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == 0);


  //printf ("Clear taint of the buffer\n");
  klee_clear_taint (buffer, 16);
  klee_assert (klee_get_taint (buffer, 16) == 0);
  for (i = 0; i < 16; i++)
    klee_assert (klee_get_taint (buffer + i, 1) == 0);

}
/*
  This shall propagate the taints from both the value and from
  the pointer used to set the result 
*/
int
test_Load()
{
  int value = 0x12345678;
  int *value_p = &value;
  int result;

  klee_set_taint(0,&result,sizeof(result));
  klee_set_taint(1,&value,sizeof(value));
  klee_set_taint(2,&value_p,sizeof(value_p));

  result = *value_p;

  //printf ("Result taint: 0x%08x\n", klee_get_taint(&result,sizeof(result)));
  klee_assert(klee_get_taint(&result,sizeof(result))==3);

  return result;
}

/* tests that values ( "input" below ) pass taintness when used as memory offsets*/
int
test_MemoryOffsetTaint()
{
  char input=2;
  char map[5] = { 1,2,3,4,5 };
  char output;
  klee_set_taint(0x1, &input, 1);
  klee_set_taint(0x2, map, 5);
  klee_set_taint(0, &output, 1);
  
  //output = map[input];
  output = *(char*) ((long int)map + (long int)input);
  
  //  printf ("output: <%c>\n",output);
  //  printf ("Output taint: 0x%08x\n", klee_get_taint(&output,1));
  
  //klee_assert(klee_get_taint(&output,sizeof(output))==klee_get_taint(&input,sizeof(input)) + klee_get_taint(&map,sizeof(map)));
  
}

/* assigning a constant to a variable erases its taintset (re: Executor.cpp:3079 rev.24) */
int 
test_ConstAssign()
{
  int value = 0x12345678;
  klee_set_taint(1,&value,sizeof(value));
  value = 10;
  klee_assert(klee_get_taint(&value,sizeof(value))==0);

}

/*ERROR: This works as a covert channel (The idea is that it wont)*/
char  covert_channel(unsigned char c){
  int i;
  char map[0x100] = {0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 , 20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 , 30 , 31 , 32 , 33 , 34 , 35 , 36 , 37 , 38 , 39 , 40 , 41 , 42 , 43 , 44 , 45 , 46 , 47 , 48 , 49 , 50 , 51 , 52 , 53 , 54 , 55 , 56 , 57 , 58 , 59 , 60 , 61 , 62 , 63 , 64 , 65 , 66 , 67 , 68 , 69 , 70 , 71 , 72 , 73 , 74 , 75 , 76 , 77 , 78 , 79 , 80 , 81 , 82 , 83 , 84 , 85 , 86 , 87 , 88 , 89 , 90 , 91 , 92 , 93 , 94 , 95 , 96 , 97 , 98 , 99 , 100 , 101 , 102 , 103 , 104 , 105 , 106 , 107 , 108 , 109 , 110 , 111 , 112 , 113 , 114 , 115 , 116 , 117 , 118 , 119 , 120 , 121 , 122 , 123 , 124 , 125 , 126 , 127 , 128 , 129 , 130 , 131 , 132 , 133 , 134 , 135 , 136 , 137 , 138 , 139 , 140 , 141 , 142 , 143 , 144 , 145 , 146 , 147 , 148 , 149 , 150 , 151 , 152 , 153 , 154 , 155 , 156 , 157 , 158 , 159 , 160 , 161 , 162 , 163 , 164 , 165 , 166 , 167 , 168 , 169 , 170 , 171 , 172 , 173 , 174 , 175 , 176 , 177 , 178 , 179 , 180 , 181 , 182 , 183 , 184 , 185 , 186 , 187 , 188 , 189 , 190 , 191 , 192 , 193 , 194 , 195 , 196 , 197 , 198 , 199 , 200 , 201 , 202 , 203 , 204 , 205 , 206 , 207 , 208 , 209 , 210 , 211 , 212 , 213 , 214 , 215 , 216 , 217 , 218 , 219 , 220 , 221 , 222 , 223 , 224 , 225 , 226 , 227 , 228 , 229 , 230 , 231 , 232 , 233 , 234 , 235 , 236 , 237 , 238 , 239 , 240 , 241 , 242 , 243 , 244 , 245 , 246 , 247 , 248 , 249 , 250 , 251 , 252 , 253 , 254 , 255};
  klee_set_taint(0, map, 256);
  char result = *(char*)((long)map + (long)c);
  return result;
}

int
test_CovertChannel()
{
  int i=0;
  char input[7]="SECRET";
  char output[7];


  klee_set_taint(0x8, output, 7);
  klee_set_taint(0x1, input, 7);

  for (i=0; i<7; i++)
    output[i] = covert_channel(input[i]);
  //    output[i] = input[i];

  klee_assert(klee_get_taint(output, sizeof(output))==0x1);

}



int
main (int argc, char *argv[])
{
  TEST(MemoryOffsetTaint);
  TEST(ConstAssign);
  TEST(CovertChannel);
  TEST(Jump);
  TEST(While);
  TEST(Conditional);
  TEST(Nested_Conditional);
  TEST(Nested_Fun);
  TEST(Conditional_Regions);
  TEST(AddConst );
  TEST(Buffer_Star );
  TEST(Buffer );
  TEST(Unwind );
  TEST(Br );
  TEST(Switch );
  TEST(Unreachable );
  TEST(Invoke );
  TEST(CallRet );
  TEST(PHI );
  TEST(Select );
  TEST(VAArg );
  TEST(Add );
  TEST(Sub );
  TEST(Mul );
  TEST(UDiv );
  TEST(SDiv );
  TEST(URem );
  TEST(SRem );
  TEST(And );
  TEST(Or );
  TEST(Xor );
  TEST(Shl );
  TEST(LShr );
  TEST(AShr );
  TEST(ICmp );
  TEST(Malloc );
  TEST(Alloca );
  TEST(Alloca );
  TEST(Free );
  TEST(Load );
  TEST(Store );
  TEST(GetElementPtr );
  TEST(Trunc );
  TEST(ZExt );
  TEST(SExt );
  TEST(IntToPtr );
  TEST(PtrToInt );
  TEST(BitCast );
  TEST(FAdd );
  TEST(FSub );
  TEST(FMul );
  TEST(FDiv );
  TEST(FRem );
  TEST(FPTrunc );
  TEST(FPExt );
  TEST(FPToUI );
  TEST(FPToSI );
  TEST(UIToFP );
  TEST(SIToFP );
  TEST(FCmp );
  TEST(InsertValue );
  TEST(ExtractValue );
  TEST(ExtractElement );
  TEST(InsertElement );
  TEST(ShuffleVector );
  return 0;
}
